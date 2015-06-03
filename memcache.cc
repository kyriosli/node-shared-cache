#include<uv.h>
#include<stdio.h>
#include<string.h>

#define	MAGIC_NUM	0xdeadbeef

namespace cache {

typedef struct node_slave_s {
	uint32_t	slave_next;
	char		data[0];
} node_slave_t;

typedef struct node_s {
	uint32_t    slave_next;
	uint32_t	hash;
	uint32_t	next;
	uint32_t	hash_next;

	uint16_t	keyLen;
	uint16_t	valLen;
	char		data[0];

} node_t;


typedef	struct cache_s {
	uint8_t		bitmap[262144];
	uint32_t	hash_map[65536];
	struct {
		uint32_t	magic;
		size_t		size;
		uint32_t	blocks_used;
		uint32_t	blocks_total;

		uv_rwlock_t	lock;
	} info;

} cache_t;

void init(void* ptr, size_t size) {
	cache_t& cache = *static_cast<cache_t*>(ptr);
	if(cache.info.magic == MAGIC_NUM && cache.info.size == size) { // already initialized
		fprintf(stderr, "init cache: already initialized\n");
		return;
	}
	memset(&cache, 0, sizeof(cache_t));


	uv_rwlock_init(&cache.info.lock);
	uv_rwlock_rdlock(&cache.info.lock);
	
	cache.info.magic = MAGIC_NUM;
	cache.info.size = size;
	
	uv_rwlock_rdunlock(&cache.info.lock);
	
	uint32_t blocks = size >> 10;
	cache.info.blocks_total = blocks - 512;
	cache.info.blocks_used = 1;
	cache.bitmap[0] = 1; // first block is used by cache.info

	fprintf(stderr, "init cache: size %d, blocks %d, usage %d/%d\n", size, blocks, cache.info.blocks_used, cache.info.blocks_total);
}

}
