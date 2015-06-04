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

#define	FATALIF(expr, n, method)	if((expr) == n) {\
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
	if(len < 1048576) {
		return NanThrowError("cache size should be greater than 1MB");
	}

	int fd;

	FATALIF(fd = shm_open(*NanUtf8String(args[0]), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR), -1, shm_open);
	FATALIF(ftruncate(fd, len), -1, ftruncate);

	void* ptr;

	FATALIF(ptr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0), MAP_FAILED, mmap);

	cache::init(ptr, len);

	NanSetInternalFieldPointer(args.Holder(), 0, ptr);
	NanReturnUndefined();
}

static NAN_PROPERTY_GETTER(getter) {
	NanScope();
	void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);
	fprintf(stderr, "getting property %x %s\n", ptr, *NanUtf8String(property));

	NanReturnUndefined();
}

static NAN_PROPERTY_SETTER(setter) {
	NanScope();
	void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);
	fprintf(stderr, "setting property %x %s = %s\n", ptr, *NanUtf8String(property), *NanUtf8String(value));
	bson::BSONValue bsonValue(value);


	for(uint32_t i=0,L=bsonValue.Length(); i<L;i++) {
		uint8_t n = bsonValue.Data()[i];
		fprintf(stderr, n > 15 ? "%x " : "0%x ", n);
	}
	fprintf(stderr, "value.length %d\n", bsonValue.Length());
	NanUcs2String sKey(property);

	FATALIF(cache::set(ptr, *sKey, sKey.length(), bsonValue.Data(), bsonValue.Length()), -1, cache::set);

	NanReturnUndefined();
}

static NAN_PROPERTY_ENUMERATOR(enumerator) {
	NanScope();
	Local<Array> ret = NanNew<Array>();

	NanReturnValue(ret);
}

static NAN_PROPERTY_DELETER(deleter) {
	NanScope();
	void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);
	fprintf(stderr, "deleting property %x %s\n", ptr, *NanUtf8String(property));
	NanReturnValue(NanTrue());
}

static NAN_PROPERTY_QUERY(querier) {
	NanScope();
	void* ptr = NanGetInternalFieldPointer(args.Holder(), 0);
	fprintf(stderr, "query property %x %s\n", ptr, *NanUtf8String(property));
	NanReturnValue(NanNew<Integer>(0));
}

void init(Handle<Object> exports) {
	NanScope();

	Local<FunctionTemplate> constructor = NanNew<FunctionTemplate>(create);
	Local<ObjectTemplate> inst = constructor->InstanceTemplate();
	inst->SetInternalFieldCount(1);

	Local<ObjectTemplate> proto = constructor->PrototypeTemplate();
	inst->SetNamedPropertyHandler(getter, setter, querier, deleter, enumerator);
	
	exports->Set(NanNew<String>("Cache"), constructor->GetFunction());
}


NODE_MODULE(binding, init)
