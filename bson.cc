#include "bson.h"

#include<nan.h>
bson::BSONValue::BSONValue(v8::Handle<v8::Value> value) {
}

bson::BSONValue::~BSONValue() {
	if(length > sizeof(cache)) { // new cache is allocated
		delete[] pointer;
	}	
}
