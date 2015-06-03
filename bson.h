#ifndef	BSON_H_
#define BSON_H_

#include<v8.h>

namespace bson {
	typedef enum {
		Null,
		Undefined,
		Boolean,
		Number,
		String,
		Array,
		Object
	} TYPES;

	class BSONValue {
	private:
		uint8_t	cache[32];
		size_t	length;
		uint8_t*pointer;
	public:
		BSONValue(v8::Handle<v8::Value> value);
		~BSONValue();
		inline const uint8_t* Data() { return pointer; }
		inline size_t Length () { return length; }
	};

}

#endif
