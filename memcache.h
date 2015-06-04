#ifndef	MEMCACHE_H_
#define	MEMCACHE_H_

namespace cache {
	void init(void* ptr, size_t size);
	int set(void* ptr, const uint16_t* key, size_t keyLen, const uint8_t* val, size_t valLen);

	class EnumerateCallback {
	public:
		virtual void next(uint16_t* key, size_t keyLen) = 0;
	};

	void enumerate(void* ptr, EnumerateCallback& enumerator);

	void get(void* ptr, const uint16_t* key, size_t keyLen, uint8_t*& val, size_t& valLen);

	bool contains(void* ptr, const uint16_t* key, size_t keyLen);

	bool unset(void* ptr, const uint16_t* key, size_t keyLen);
}

#endif
