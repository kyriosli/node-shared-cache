#include<nan.h>

#include<unistd.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
#include "memcache.h"
#include "bson.h"


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

    size_t len = args[1]->Uint32Value();
    if(len < 557056) {
        return NanThrowError("cache size should be greater than 544KB");
    } else if(len > 2147483647) {
        return NanThrowError("cache size should be less than 2GB");
    }
    len &= ~32767; // 32KB aligned

    // fprintf(stderr, "allocating %d bytes memory\n", len);

    int fd;

    FATALIF(fd = shm_open(*NanUtf8String(args[0]), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR), -1, shm_open);
    FATALIF(ftruncate(fd, len), -1, ftruncate);

    void* ptr;

    FATALIF(ptr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0), MAP_FAILED, mmap);

    cache::init(ptr, len);

    NanSetInternalFieldPointer(args.Holder(), 0, ptr);
    NanReturnUndefined();
}

#define PROPERTY_SCOPE(ptr, keyLen, keyBuf) void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);\
    size_t keyLen = property->Length();\
    if(keyLen > 255) {\
        return NanThrowError("length of property name should not be greater than 255");\
    }\
    uint16_t keyBuf[256];\
    property->Write(keyBuf)


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

//  fputs("      | 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f | 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f", stderr);
//  for(uint32_t i=0,L=bsonValue.Length(); i<L;i++) {
//      uint8_t n = bsonValue.Data()[i];
//      if(!(i&511)) {
//          fputs("\n------|-------------------------------------------------|------------------------------------------------", stderr);
//      }
//      if(!(i&31)) {
//          fprintf(stderr, "\n%06x| ", i);
//      } else if(!(i&15)) {
//          fputs("| ", stderr);
//      }
//      fprintf(stderr, n > 15 ? "%x " : "0%x ", n);
//  }
    // fprintf(stderr, "length=%d\n", bsonValue.Length());


    FATALIF(cache::set(ptr, keyBuf, keyLen, bsonValue.Data(), bsonValue.Length()), -1, cache::set);
    NanReturnUndefined();
}

class KeysEnumerator: public cache::EnumerateCallback {
public:
    uint32_t length;
    Local<Array> keys;

    void next(uint16_t* key, size_t keyLen) {
        keys->Set(length++, NanNew<String>(key, keyLen));
    }

    KeysEnumerator() :  length(0), keys(NanNew<Array>()) {}
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
    void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);
    size_t keyLen = property->Length();
    if(keyLen > 255) {
        NanThrowError("length of property name should not be greater than 255");
        return Handle<Boolean>();
    }
    uint16_t keyBuf[256];
    property->Write(keyBuf);

    NanReturnValue(cache::unset(ptr, keyBuf, keyLen) ? NanTrue() : NanFalse());
}

static NAN_PROPERTY_QUERY(querier) {
    NanScope();
    void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);
    size_t keyLen = property->Length();
    if(keyLen > 255) {
        NanThrowError("length of property name should not be greater than 255");
        return Handle<Integer>();
    }
    uint16_t keyBuf[256];
    property->Write(keyBuf);

    NanReturnValue(cache::contains(ptr, keyBuf, keyLen) ? NanNew<Integer>(0) : Handle<Integer>());
}

void init(Handle<Object> exports) {
    NanScope();

    Local<FunctionTemplate> constructor = NanNew<FunctionTemplate>(create);
    Local<ObjectTemplate> inst = constructor->InstanceTemplate();
    inst->SetInternalFieldCount(1);
    inst->SetNamedPropertyHandler(getter, setter, querier, deleter, enumerator);
    
    exports->Set(NanNew<String>("Cache"), constructor->GetFunction());
}


NODE_MODULE(binding, init)
