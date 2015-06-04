#ifndef	MEMCACHE_H_
#define	MEMCACHE_H_

namespace cache {
	void init(void* ptr, size_t size);
	int set(void*ptr, const uint16_t* key, size_t keyLen, const uint8_t* val, size_t valLen);
}

#endif
