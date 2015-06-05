#ifndef MEMCACHE_H_
#define MEMCACHE_H_

namespace cache {
    void init(void* ptr, int fd, size_t size);
    int set(void* ptr, int fd, const uint16_t* key, size_t keyLen, const uint8_t* val, size_t valLen);

    class EnumerateCallback {
    public:
        virtual void next(uint16_t* key, size_t keyLen) = 0;
    };

    void enumerate(void* ptr, int fd, EnumerateCallback& enumerator);

    void get(void* ptr, int fd, const uint16_t* key, size_t keyLen, uint8_t*& val, size_t& valLen);

    bool contains(void* ptr, int fd, const uint16_t* key, size_t keyLen);

    bool unset(void* ptr, int fd, const uint16_t* key, size_t keyLen);
}

#endif
