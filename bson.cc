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

        do {
            capacity <<= 1;
        } while(capacity < required);

        uint8_t* new_pointer = new uint8_t[capacity];
        uint8_t* old_pointer = current - used;
        memcpy(old_pointer, new_pointer, used);

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
	        ensureCapacity(8);
	        *(current++) = bson::Number;
	        *reinterpret_cast<double*>(current) = value->NumberValue();
	        current += 8;
	    } else if(value->IsString()) {
	        NanUcs2String str(value);
	        size_t len = str.length() << 1;
	        ensureCapacity(4 + len);

	        *(current++) = bson::String;
            *reinterpret_cast<uint32_t*>(current) = len;
            current += 4;
	        memcpy(current, *str, len);
	        current += len;
	    } else if(value->IsObject()) {
	        Handle<Object> obj = value.As<Object>();
	        // check if object has already been serialized
	        object_wrapper_t* curr = objects;
	        while(curr) {
	            if(curr->object->StrictEquals(value)) { // found
	                ensureCapacity(4);
	                *(current++) = bson::ObjectRef;
	                *reinterpret_cast<uint32_t*>(current) = curr->index;
	                return;
	            }
	        }
	        // curr is null
	        objects = new object_wrapper_t(obj, objects);
	        if(value->IsArray()) {
	            Handle<Array> arr = obj.As<Array>();
	            uint32_t len = arr->Length();
	            ensureCapacity(4);
	            *(current++) = bson::Array;
	            *reinterpret_cast<uint32_t*>(current) = len;
	            for(uint32_t i = 0; i < len; i++) {
	                write(arr->Get(i));
	            }
	        } else { // TODO: support for other object types
	            Local<Array> names = obj->GetOwnPropertyNames();
	            uint32_t len = names->Length();
	            ensureCapacity(4);
	            *(current++) = bson::Object;
	            *reinterpret_cast<uint32_t*>(current) = len;
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
    // fprintf(stderr, "%d bytes used writing %s\n", writer.used, *NanUtf8String(value));

    pointer = writer.current - writer.used;
    length = writer.used;
}

bson::BSONValue::~BSONValue() {
    if(length > sizeof(cache)) { // new cache is allocated
        delete[] pointer;
    }    
}
