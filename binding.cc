#include<nan.h>

#include<unistd.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
#include "memcache.h"
#include "bson.h"

#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
#define NanReturnEmpty(TYPE)    return
#else
#define NanReturnEmpty(TYPE)    return Handle<TYPE>()
#endif

using namespace v8;

#define FATALIF(expr, n, method)    if((expr) == n) {\
    char sbuf[64];\
    sprintf(sbuf, "%s failed with code %d at " __FILE__ ":%d", #method, errno, __LINE__);\
    return NanThrowError(sbuf);\
}

static NAN_METHOD(create) {
    if(!args.IsConstructCall()) {
        return NanThrowError("Illegal constructor");
    }

    NanScope();

    uint32_t size = args[1]->Uint32Value();
    uint32_t block_size_shift = args[2]->Uint32Value();
    if(!block_size_shift) block_size_shift = 6;

    uint32_t blocks = size >> (5 + block_size_shift) << 5; // 32 aligned
    size = blocks << block_size_shift;

    if(block_size_shift < 6) {
        return NanThrowError("block size should not be smaller than 64 bytes");
    } else if(block_size_shift > 11) {
        return NanThrowError("block size should not be greater than 2 KB");
    } else if(blocks < (HEADER_SIZE >> block_size_shift)) {
        return NanThrowError("total size should be larger than 512 KB");
    } else if(blocks > 2097152) {
        return NanThrowError("block count should be smaller than 2097152");
    }

    // fprintf(stderr, "allocating %d bytes memory\n", len);

    int fd;

    FATALIF(fd = shm_open(*NanUtf8String(args[0]), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR), -1, shm_open);
    FATALIF(ftruncate(fd, size), -1, ftruncate);

    void* ptr;

    FATALIF(ptr = mmap(NULL, blocks << block_size_shift, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0), MAP_FAILED, mmap);

    cache::init(ptr, blocks, block_size_shift);

    NanSetInternalFieldPointer(args.Holder(), 0, ptr);
    NanReturnUndefined();
}

#define PROPERTY_SCOPE(ptr, keyLen, keyBuf) size_t keyLen = property->Length();\
    void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);\
    if(keyLen > (1 << (*reinterpret_cast<uint16_t*>(ptr) - 1)) - 16) {\
        return NanThrowError("length of property name should not be greater than (block size - 32) / 2");\
    }\
    uint16_t keyBuf[256];\
    property->Write(keyBuf);\


static NAN_PROPERTY_GETTER(getter) {
    NanScope();
    PROPERTY_SCOPE(ptr, keyLen, keyBuf);
    
    uint8_t* val;
    size_t valLen;

    cache::get(ptr, keyBuf, keyLen, val, valLen);

    if(val) {
        // TODO bson decode
        Handle<Value> ret = bson::parse(val);
        delete[] val;
        NanReturnValue(ret);
    } else {
        NanReturnUndefined();
    }
}

static NAN_PROPERTY_SETTER(setter) {
    NanScope();
    PROPERTY_SCOPE(ptr, keyLen, keyBuf);

    bson::BSONValue bsonValue(value);

    FATALIF(cache::set(ptr, keyBuf, keyLen, bsonValue.Data(), bsonValue.Length()), -1, cache::set);
    NanReturnValue(value);
}

class KeysEnumerator: public cache::EnumerateCallback {
public:
    uint32_t length;
    Local<Array> keys;

    void next(uint16_t* key, size_t keyLen) {
        keys->Set(length++, NanNew<String>(key, keyLen));
    }

    KeysEnumerator() : length(0), keys(NanNew<Array>()) {}
};

static NAN_PROPERTY_ENUMERATOR(enumerator) {
    NanScope();
    void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);
    // fprintf(stderr, "enumerating properties %x\n", ptr);

    KeysEnumerator enumerator;
    cache::enumerate(ptr, enumerator);

    NanReturnValue(enumerator.keys);
}

static NAN_PROPERTY_DELETER(deleter) {
    NanScope();
    size_t keyLen = property->Length();
    if(keyLen > 255) {
        NanThrowError("length of property name should not be greater than 255");
        NanReturnEmpty(Boolean);
    }
    uint16_t keyBuf[256];
    property->Write(keyBuf);
    void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);

    NanReturnValue(cache::unset(ptr, keyBuf, keyLen) ? NanTrue() : NanFalse());
}

static NAN_PROPERTY_QUERY(querier) {
    NanScope();
    size_t keyLen = property->Length();
    if(keyLen > 255) {
        NanThrowError("length of property name should not be greater than 255");
        NanReturnEmpty(Integer);
    }
    uint16_t keyBuf[256];
    property->Write(keyBuf);
    void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);

    NanReturnValue(cache::contains(ptr, keyBuf, keyLen) ? NanNew<Integer>(0) : Handle<Integer>());
}

void init(Handle<Object> exports) {
    NanScope();

    Local<FunctionTemplate> constructor = NanNew<FunctionTemplate>(create);
    Local<ObjectTemplate> inst = constructor->InstanceTemplate();
    inst->SetInternalFieldCount(1); // ptr
    inst->SetNamedPropertyHandler(getter, setter, querier, deleter, enumerator);
    
    exports->Set(NanNew<String>("Cache"), constructor->GetFunction());
}


NODE_MODULE(binding, init)
