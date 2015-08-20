#ifndef MEMCACHE_H_
#define MEMCACHE_H_


#define HEADER_SIZE 262144

namespace cache {
    bool init(void* ptr, uint32_t blocks, uint32_t block_size_shift, bool forced);

    int set(void* ptr, int fd, const uint16_t* key, size_t keyLen, const uint8_t* val, size_t valLen);

    void enumerate(void* ptr, int fd, void* enumerator, void(* callback)(void*,uint16_t*,size_t));

    void get(void* ptr, int fd, const uint16_t* key, size_t keyLen, uint8_t*& val, size_t& valLen);

    bool contains(void* ptr, int fd, const uint16_t* key, size_t keyLen);

    bool unset(void* ptr, int fd, const uint16_t* key, size_t keyLen);
}

#endif
