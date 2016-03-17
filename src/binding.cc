#include<nan.h>

#include<unistd.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include "memcache.h"
#include "bson.h"

#define CACHE_HEADER_IN_WORDS   131080

using namespace v8;

#define FATALIF(expr, n, method)    if((expr) == n) {\
    char sbuf[64];\
    sprintf(sbuf, __FILE__ ":%d: `%s' failed with code %d", __LINE__, #method, errno);\
    return Nan::ThrowError(sbuf);\
}

static NAN_METHOD(release) {
    FATALIF(shm_unlink(*String::Utf8Value(info[0])), -1, shm_unlink);
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

    // fprintf(stderr, "allocating %d bytes memory\n", size);

    int fd;
    Nan::Utf8String name(info[0]);

    FATALIF(fd = shm_open(*name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR), -1, shm_open);
    struct stat stat;
    FATALIF(fstat(fd, &stat), -1, fstat);

    if(stat.st_size == 0) {
        FATALIF(ftruncate(fd, size), -1, ftruncate);        
    } else if(stat.st_size != size) {
        return Nan::ThrowError("cache initialized with different size");
    }


    void* ptr;

    FATALIF(ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0), MAP_FAILED, mmap);

    if(cache::init(ptr, blocks, block_size_shift, stat.st_size == 0)) {
        Nan::SetInternalFieldPointer(info.Holder(), 0, ptr);
#ifdef __MACH__
	char sbuf[64];
	sprintf(sbuf, "/tmp/shared_cache_%s", *name);
	fd = open(sbuf, O_CREAT | O_RDONLY);
#endif

        info.Holder()->SetInternalField(1, Nan::New(fd));
    } else {
        Nan::ThrowError("cache initialization failed, maybe it has been initialized with different block size");
    }

}

#define METHOD_SCOPE(holder, ptr, fd) void* ptr = Nan::GetInternalFieldPointer(holder, 0);\
    int fd = holder->GetInternalField(1)->Int32Value()

#define PROPERTY_SCOPE(property, holder, ptr, fd, keyLen, keyBuf) int keyLen = property->Length();\
    if(keyLen > 256) {\
        return Nan::ThrowError("length of property name should not be greater than 256");\
    }\
    METHOD_SCOPE(holder, ptr, fd);\
    if((keyLen << 1) + 32 > 1 << static_cast<uint16_t*>(ptr)[CACHE_HEADER_IN_WORDS]) {\
        return Nan::ThrowError("length of property name should not be greater than (block size - 32) / 2");\
    }\
    uint16_t keyBuf[256];\
    property->Write(keyBuf)


static NAN_PROPERTY_GETTER(getter) {
    PROPERTY_SCOPE(property, info.Holder(), ptr, fd, keyLen, keyBuf);
    
    uint8_t tmp[1024];

    uint8_t* val = tmp;
    size_t valLen = sizeof(tmp);

    cache::get(ptr, fd, keyBuf, keyLen, val, valLen);

    if(!val) return; // not found, returns undefined
    Local<Value> ret = bson::parse(val);

    if(valLen > sizeof(tmp)) {
        delete[] val;
    }
    info.GetReturnValue().Set(ret);
}

static NAN_PROPERTY_SETTER(setter) {
    PROPERTY_SCOPE(property, info.Holder(), ptr, fd, keyLen, keyBuf);

    bson::BSONValue bsonValue(value);

    FATALIF(cache::set(ptr, fd, keyBuf, keyLen, bsonValue.Data(), bsonValue.Length()), -1, cache::set);
    info.GetReturnValue().Set(value);
}

class KeysEnumerator {
public:
    uint32_t length;
    Local<Array> keys;

    static void next(KeysEnumerator* self, uint16_t* key, size_t keyLen) {
        self->keys->Set(self->length++, Nan::New<String>(key, keyLen).ToLocalChecked());
    }

    inline KeysEnumerator() : length(0), keys(Nan::New<Array>()) {}
};

static NAN_PROPERTY_ENUMERATOR(enumerator) {
    METHOD_SCOPE(info.Holder(), ptr, fd);
    // fprintf(stderr, "enumerating properties %x\n", ptr);

    KeysEnumerator enumerator;
    cache::enumerate(ptr, fd, &enumerator, KeysEnumerator::next);

    info.GetReturnValue().Set(enumerator.keys);
}

static NAN_PROPERTY_DELETER(deleter) {
    PROPERTY_SCOPE(property, info.Holder(), ptr, fd, keyLen, keyBuf);

    info.GetReturnValue().Set(cache::unset(ptr, fd, keyBuf, keyLen));
}

static NAN_PROPERTY_QUERY(querier) {
    PROPERTY_SCOPE(property, info.Holder(), ptr, fd, keyLen, keyBuf);
    if(cache::contains(ptr, fd, keyBuf, keyLen)) {
        info.GetReturnValue().Set(0);
    }
}

// increase(holder, key, [by])
static NAN_METHOD(increase) {
    Local<Object> holder = Local<Object>::Cast(info[0]);
    PROPERTY_SCOPE(info[1]->ToString(), holder, ptr, fd, keyLen, keyBuf);
    uint32_t increase_by = info.Length() > 2 ? info[2]->Uint32Value() : 1;
    info.GetReturnValue().Set(cache::increase(ptr, fd, keyBuf, keyLen, increase_by));
}

class EntriesDumper {
public:
    Local<Object> entries;
    uint16_t key[256];
    size_t keyLen;

    static void next(EntriesDumper* self, uint16_t* key, size_t keyLen, uint8_t* val) {
        if(self->keyLen) {
            if(self->keyLen > keyLen || memcmp(self->key, key, self->keyLen << 1)) return;
        }
        self->entries->Set(Nan::New<String>(key, keyLen).ToLocalChecked(), bson::parse(val));
    }

    inline  EntriesDumper() : entries(Nan::New<Object>()), keyLen(0) {}
};

static NAN_METHOD(dump) {
    Local<Object> holder = Local<Object>::Cast(info[0]);
    METHOD_SCOPE(holder, ptr, fd);
    EntriesDumper dumper;

    if(info.Length() > 1 && info[1]->BooleanValue()) {
        Local<String> prefix = info[1]->ToString();
        int keyLen = prefix->Length();
        if(keyLen > 256) {
            info.GetReturnValue().Set(dumper.entries);
            return;
        }

        prefix->Write(dumper.key);
        dumper.keyLen = keyLen;
    }


    cache::dump(ptr, fd, &dumper, EntriesDumper::next);
    info.GetReturnValue().Set(dumper.entries);
}

static NAN_METHOD(clear) {
    Local<Object> holder = Local<Object>::Cast(info[0]);
    METHOD_SCOPE(holder, ptr, fd);
    cache::clear(ptr, fd);
}

void init(Handle<Object> exports) {

    Local<FunctionTemplate> constructor = Nan::New<FunctionTemplate>(create);
    Local<ObjectTemplate> inst = constructor->InstanceTemplate();
    inst->SetInternalFieldCount(2); // ptr, fd
    Nan::SetNamedPropertyHandler(inst, getter, setter, querier, deleter, enumerator);
    
    Nan::Set(exports, Nan::New("Cache").ToLocalChecked(), constructor->GetFunction());
    Nan::SetMethod(exports, "release", release);
    Nan::SetMethod(exports, "increase", increase);
    Nan::SetMethod(exports, "clear", clear);
    Nan::SetMethod(exports, "dump", dump);
}


NODE_MODULE(binding, init)
