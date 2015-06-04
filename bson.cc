#include "bson.h"

#include<nan.h>
#include<string.h>

typedef struct object_wrapper_s {
    v8::Handle<v8::Object> object;
    uint32_t index;
    object_wrapper_s* next;

    object_wrapper_s(v8::Handle<v8::Object> obj, object_wrapper_s* curr) :
        object(obj), index(curr ? curr->index + 1 : 0), next(curr) {}
} object_wrapper_t;

typedef struct writer_s {
    bool deleteOld;
    size_t capacity;
    size_t used;
    uint8_t* current;

    object_wrapper_t* objects;


    inline writer_s(bson::BSONValue& value) :
        deleteOld(false), capacity(sizeof(value.cache)), used(0), current(value.cache), objects(NULL) {}

    inline void ensureCapacity(size_t required) {
        required += used;
        if(capacity >= required) {
        	used = required;
        	return;
        }

        if(!deleteOld) {
        	capacity = 4096;
        }
        while(capacity < required) {
            capacity <<= 1;
        }

        uint8_t* new_pointer = new uint8_t[capacity];
        fprintf(stderr, "writer::ensureCapacity: new buffer allocated:%x (len:%d, required:%d, used:%d)\n", new_pointer, capacity, required, used);
        uint8_t* old_pointer = current - used;
        memcpy(new_pointer, old_pointer, used);

        if(deleteOld) delete[] old_pointer;
        else deleteOld = true;

        current = new_pointer + used;

        // set used to required + used
        used = required;
    }

    inline ~writer_s() {
        object_wrapper_t* curr = objects;
        while(curr) {
            object_wrapper_t* next = curr->next;
            delete curr;
            curr = next;
        }
    }

    void write(v8::Handle<v8::Value> value) {
	    using namespace v8;
	    ensureCapacity(1);
	    if(value->IsNull()) {
	        *(current++) = bson::Null;
	    } else if(value->IsBoolean()) {
	        *(current++) = value->IsTrue() ? bson::True : bson::False;
	    } else if(value->IsNumber()) {
	        *(current++) = bson::Number;
	        ensureCapacity(sizeof(double));
	        *reinterpret_cast<double*>(current) = value->NumberValue();
	        current += sizeof(double);
	    } else if(value->IsString()) {
	        *(current++) = bson::String;
	        NanUcs2String str(value);
	        size_t len = str.length() << 1;
	        ensureCapacity(sizeof(uint32_t) + len);

            *reinterpret_cast<uint32_t*>(current) = len;
            current += sizeof(uint32_t);
	        memcpy(current, *str, len);
	        current += len;
	    } else if(value->IsObject()) {
	        Handle<Object> obj = value.As<Object>();
	        // check if object has already been serialized
	        object_wrapper_t* curr = objects;
	        while(curr) {
	            if(curr->object->StrictEquals(value)) { // found
	                *(current++) = bson::ObjectRef;
	                ensureCapacity(sizeof(uint32_t));
	                *reinterpret_cast<uint32_t*>(current) = curr->index;
	                current += sizeof(uint32_t);
	                return;
	            }
	            curr = curr->next;
	        }
	        // curr is null
	        objects = new object_wrapper_t(obj, objects);
	        if(value->IsArray()) {
	            *(current++) = bson::Array;
	            Handle<Array> arr = obj.As<Array>();
	            uint32_t len = arr->Length();
	            ensureCapacity(sizeof(uint32_t));
	            *reinterpret_cast<uint32_t*>(current) = len;
	            current += sizeof(uint32_t);
	            for(uint32_t i = 0; i < len; i++) {
	            	fprintf(stderr, "write array[%d] (len=%d)\n", i, len);
	                write(arr->Get(i));
	            }
	        } else { // TODO: support for other object types
	            *(current++) = bson::Object;
	            Local<Array> names = obj->GetOwnPropertyNames();
	            uint32_t len = names->Length();
	            ensureCapacity(sizeof(uint32_t));
	            *reinterpret_cast<uint32_t*>(current) = len;
	            current += sizeof(uint32_t);
	            for(uint32_t i = 0; i < len; i++) {
	                Local<Value> name = names->Get(i);
	                write(name);
	                write(obj->Get(name));
	            }
	        }
	    } else {
	        *(current++) = bson::Undefined;
	    }
	}

} writer_t;



bson::BSONValue::BSONValue(v8::Handle<v8::Value> value) {
    NanScope();
    writer_t writer(*this);
    writer.write(value);
    fprintf(stderr, "%d bytes used writing %s\n", writer.used, *NanUtf8String(value));

    pointer = writer.current - writer.used;
    length = writer.used;
}

bson::BSONValue::~BSONValue() {
    if(length > sizeof(cache)) { // new cache is allocated
    	fprintf(stderr, "BSONValue: freeing buffer %x (len:%d)\n", pointer, length);
        delete[] pointer;
    }    
}
