#ifndef BSON_H_
#define BSON_H_

#include<v8.h>

namespace bson {
    typedef enum {
        Null,
        Undefined,
        True,
        False,
        Int32,
        Number,
        String,
        Array,
        Object,
        ObjectRef
    } TYPES;

    class BSONValue {
    private:
        size_t  length;
        uint8_t*pointer;
    public:
        uint8_t cache[32];
        BSONValue(v8::Handle<v8::Value> value);
        ~BSONValue();
        inline const uint8_t* Data() { return pointer; }
        inline size_t Length () { return length; }
    };

    v8::Local<v8::Value> parse(const uint8_t* data);

}

#endif
