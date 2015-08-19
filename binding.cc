#include<nan.h>

#include<unistd.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
#include "memcache.h"
#include "bson.h"

#define CACHE_HEADER_IN_WORDS   131078

using namespace v8;

#define FATALIF(expr, n, method)    if((expr) == n) {\
    char sbuf[64];\
    sprintf(sbuf, "%s failed with code %d at " __FILE__ ":%d", #method, errno, __LINE__);\
    return Nan::ThrowError(sbuf);\
}

static NAN_METHOD(create) {
    if(!info.IsConstructCall()) {
        return Nan::ThrowError("Illegal constructor");
    }

    uint32_t size = info[1]->Uint32Value();
    uint32_t block_size_shift = info[2]->Uint32Value();
    if(!block_size_shift || block_size_shift > 31) block_size_shift = 6;

    uint32_t blocks = size >> (5 + block_size_shift) << 5; // 32 aligned
    size = blocks << block_size_shift;

    if(block_size_shift < 6) {
        return Nan::ThrowError("block size should not be smaller than 64 bytes");
    } else if(block_size_shift > 14) {
        return Nan::ThrowError("block size should not be larger than 16 KB");
    }else if(size < 524288) {
        return Nan::ThrowError("total_size should be larger than 512 KB");
    }

    // fprintf(stderr, "allocating %d bytes memory\n", len);

    int fd;

    FATALIF(fd = shm_open(*Nan::Utf8String(info[0]), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR), -1, shm_open);
    FATALIF(ftruncate(fd, size), -1, ftruncate);

    void* ptr;

    FATALIF(ptr = mmap(NULL, blocks << block_size_shift, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0), MAP_FAILED, mmap);

    cache::init(ptr, blocks, block_size_shift);

    Nan::SetInternalFieldPointer(info.Holder(), 0, ptr);
}

#define PROPERTY_SCOPE(ptr, keyLen, keyBuf) size_t keyLen = property->Length();\
    if(keyLen > 256) {\
        return Nan::ThrowError("length of property name should not be greater than 256");\
    }\
    void* ptr = Nan::GetInternalFieldPointer(info.Holder(), 0);\
    if((keyLen << 1) + 32 > 1 << static_cast<uint16_t*>(ptr)[CACHE_HEADER_IN_WORDS]) {\
        return Nan::ThrowError("length of property name should not be greater than (block size - 32) / 2");\
    }\
    uint16_t keyBuf[256];\
    property->Write(keyBuf);\


static NAN_PROPERTY_GETTER(getter) {
    PROPERTY_SCOPE(ptr, keyLen, keyBuf);
    
    uint8_t tmp[1024];

    uint8_t* val = tmp;
    size_t valLen = sizeof(tmp);

    cache::get(ptr, keyBuf, keyLen, val, valLen);

    if(!val) return; // not found, returns undefined
    Local<Value> ret = bson::parse(val);

    if(valLen > sizeof(tmp)) {
        delete[] val;
    }
    info.GetReturnValue().Set(ret);
}

static NAN_PROPERTY_SETTER(setter) {
    PROPERTY_SCOPE(ptr, keyLen, keyBuf);

    bson::BSONValue bsonValue(value);

    FATALIF(cache::set(ptr, keyBuf, keyLen, bsonValue.Data(), bsonValue.Length()), -1, cache::set);
    info.GetReturnValue().Set(value);
}

class KeysEnumerator {
public:
    uint32_t length;
    Local<Array> keys;

    static void next(void* enumerator, uint16_t* key, size_t keyLen) {
        KeysEnumerator* self = static_cast<KeysEnumerator*>(enumerator);
#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
        self->keys->Set(self->length++, v8::String::NewFromTwoByte(Isolate::GetCurrent(), key, v8::String::kNormalString, keyLen));
#else
        self->keys->Set(self->length++, v8::String::New(key, keyLen));
#endif
    }

    inline KeysEnumerator() : length(0), keys(Nan::New<Array>()) {}
};

static NAN_PROPERTY_ENUMERATOR(enumerator) {
    void* ptr = Nan::GetInternalFieldPointer(info.Holder(), 0);
    // fprintf(stderr, "enumerating properties %x\n", ptr);

    KeysEnumerator enumerator;
    cache::enumerate(ptr, &enumerator, KeysEnumerator::next);

    info.GetReturnValue().Set(enumerator.keys);
}

static NAN_PROPERTY_DELETER(deleter) {
    size_t keyLen = property->Length();
    if(keyLen > 256) {
        return Nan::ThrowError("length of property name should not be greater than 256");
    }
    void* ptr = Nan::GetInternalFieldPointer(info.Holder(), 0);
    if((keyLen << 1) + 32 > 1 << static_cast<uint16_t*>(ptr)[CACHE_HEADER_IN_WORDS]) {
        return Nan::ThrowError("length of property name should not be greater than (block size - 32) / 2");
    }
    uint16_t keyBuf[256];
    property->Write(keyBuf);


    info.GetReturnValue().Set(cache::unset(ptr, keyBuf, keyLen));
}

static NAN_PROPERTY_QUERY(querier) {
    size_t keyLen = property->Length();
    if(keyLen > 256) {
        return Nan::ThrowError("length of property name should not be greater than 256");
    }
    void* ptr = Nan::GetInternalFieldPointer(info.Holder(), 0);
    if((keyLen << 1) + 32 > 1 << static_cast<uint16_t*>(ptr)[CACHE_HEADER_IN_WORDS]) {
        return Nan::ThrowError("length of property name should not be greater than (block size - 32) / 2");
    }
    uint16_t keyBuf[256];
    property->Write(keyBuf);
    if(cache::contains(ptr, keyBuf, keyLen)) {
        info.GetReturnValue().Set(0);
    }
}

void init(Handle<Object> exports) {

    Local<FunctionTemplate> constructor = Nan::New<FunctionTemplate>(create);
    Local<ObjectTemplate> inst = constructor->InstanceTemplate();
    inst->SetInternalFieldCount(1); // ptr
    Nan::SetNamedPropertyHandler(inst, getter, setter, querier, deleter, enumerator);
    
    Nan::Set(exports, Nan::New("Cache").ToLocalChecked(), constructor->GetFunction());
}


NODE_MODULE(binding, init)
