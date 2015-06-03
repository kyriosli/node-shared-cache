#ifndef	MEMCACHE_H_
#define	MEMCACHE_H_

#include<errno.h>

namespace cache {
	void init(void* ptr, size_t size);
	void set(uint8_t* key, size_t keyLen, uint8_t* val, size_t valLen);
}

#endif
