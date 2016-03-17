#ifndef MEMCACHE_H_
#define MEMCACHE_H_


#define HEADER_SIZE 262144

namespace cache {
    bool init(void* ptr, uint32_t blocks, uint32_t block_size_shift, bool forced);

    int set(void* ptr, int fd, const uint16_t* key, size_t keyLen, const uint8_t* val, size_t valLen);

    void _enumerate(void* ptr, int fd, void* enumerator, void(* callback)(void*,uint16_t*,size_t));

    void _dump(void* ptr, int fd, void* dumper, void(* callback)(void*,uint16_t*,size_t,uint8_t*));

	template<typename T>
    inline void enumerate(void* ptr, int fd, T* enumerator, void(* callback)(T*,uint16_t*,size_t)) {
    	_enumerate(ptr, fd, enumerator, (void(*)(void*,uint16_t*,size_t)) callback);
    }

    template<typename T>
    inline void dump(void* ptr, int fd, T* dumper, void(* callback)(T*,uint16_t*,size_t,uint8_t*)) {
    	_dump(ptr, fd, dumper, (void(*)(void*,uint16_t*,size_t,uint8_t*)) callback);
    }

    void get(void* ptr, int fd, const uint16_t* key, size_t keyLen, uint8_t*& val, size_t& valLen);

    bool contains(void* ptr, int fd, const uint16_t* key, size_t keyLen);

    bool unset(void* ptr, int fd, const uint16_t* key, size_t keyLen);

    void clear(void* ptr, int fd);

    int32_t increase(void* ptr, int fd, const uint16_t* key, size_t keyLen, int32_t increase_by);
}

#endif
