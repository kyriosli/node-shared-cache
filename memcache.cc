#include<stdio.h>
#include<string.h>
#include<errno.h>
#include "lock.h"
#include "memcache.h"

#define static_assert(pred) switch(0){case 0:case pred:;}

namespace cache {

typedef struct node_slave_s {
    uint32_t    slave_next;
    uint8_t        data[0];
} node_slave_t;

typedef struct node_s {
    uint32_t    slave_next;
    uint32_t    prev;
    uint32_t    next;
    uint32_t    hash_next;
    uint32_t    blocks;
    uint32_t    valLen;
    uint32_t    hash;
    uint16_t    keyLen;
    uint16_t    key[0];
} node_t;

typedef struct cache_s {
    union {
        struct {
            uint16_t    block_size_shift;
            uint16_t    nextBitPos; // next bitmap position to look for when allocating block
            uint32_t    blocks_total;
            uint32_t    blocks_used;
            uint32_t    head;
            uint32_t    tail;

			rw_lock_t   lock;
        } info; // assert(sizeof(cache.info) * 8 <= HEADER_SIZE >> block_size_shift)

        // bitmap[0~15] is not used
        uint32_t bitmap[65536]; // maximum memory size available is 65536*32*1K = 2G
    };
    uint32_t hashmap[65536];

    template<typename T>
    inline T* address(uint32_t block) {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + (block << info.block_size_shift));
    }

    inline uint32_t find(const uint16_t* key, size_t keyLen, uint32_t hash) {
    
        uint32_t curr = hashmap[hash & 0xffff];
        while(curr) {
            node_t& node = *address<node_t>(curr);
            // fprintf(stderr, "cache::find: tests match block %d (keyLen=%d hash=%d)\n", curr, node.keyLen, node.hash);
            if(node.keyLen == keyLen && node.hash == hash && !memcmp(node.key, key, keyLen << 1)) {
                return curr;
            }
            curr = node.hash_next;
        }

        return 0;
    }

    inline uint32_t selectOne() {
        uint16_t& curr = info.nextBitPos;
        uint32_t bits = bitmap[curr];

        while(bits == 0xffffffff) {
            curr++;
            if(curr == (info.blocks_total + (HEADER_SIZE >> info.block_size_shift)) >> 5) {
                curr = 1 << (14 - info.block_size_shift);
            }
            bits = bitmap[curr];
        }
        // assert(bits != 0xffffffff)
        uint32_t bitSelected = 31 - __builtin_clz(~bits);
        bitmap[curr] = bits | 1 << bitSelected;
        return curr << 5 | bitSelected;
    }

    inline void release(uint32_t block) {
        uint32_t count = 0;
        for(uint32_t next = block; next; next = address<node_slave_t>(next)->slave_next) {
            // fprintf(stderr, "free block %d assert(bit:%d) %x ^ %x = %x\n", next, (bitmap[next >> 5] >> (next & 31)) & 1, bitmap[next >> 5], 1 << (next & 31), bitmap[next >> 5] ^ 1 << (next & 31));
            bitmap[next >> 5] ^= 1 << (next & 31);
            // fprintf(stderr, "freed 1 bit,  bitmap[%d] = %x\n", next>>5, bitmap[next>>5]);
            count++;
        }
        info.blocks_used -= count;
    }

    inline void dropNode(uint32_t firstBlock) {
        node_t& node = *address<node_t>(firstBlock);
        // fprintf(stderr, "dropping node %d (prev:%d next:%d)\n", firstBlock, node.prev, node.next);
        // remove from lru list
        uint32_t& $prev = node.prev ? address<node_t>(node.prev)->next : info.head;
        uint32_t& $next = node.next ? address<node_t>(node.next)->prev : info.tail;
        $prev = node.next;
        $next = node.prev;

        // remove from hash list
        uint32_t* toModify = &hashmap[node.hash & 0xffff];
        while(*toModify != firstBlock) {
            node_t* pcurr = address<node_t>(*toModify);
            toModify = &pcurr->hash_next;
        }
        *toModify = node.hash_next;
        // release blocks
        release(firstBlock);
    }

    inline uint32_t allocate(uint32_t count) {
        uint32_t target = info.blocks_total - count;
        // fprintf(stderr, "allocate: total=%d used=%d, count=%d, target=%d\n", info.blocks_total, info.blocks_used, count, target);
        if(info.blocks_used > target) { // not enough
            do {
                // fprintf(stderr, "allocate: will drop head key %s blocks freed: %d\n", keyBuf, headBlock->blocks);
                dropNode(info.head);
            } while (info.blocks_used > target);
        }
        // TODO
        uint32_t firstBlock = selectOne();
        // fprintf(stderr, "select %d blocks (first: %d)\n", count, firstBlock);
        info.blocks_used += count;

        node_slave_t* node = address<node_slave_t>(firstBlock);
        while(--count) {
            uint32_t nextBlock = selectOne();
            // fprintf(stderr, "selected block  %d\n", nextBlock);
            node->slave_next = nextBlock;
            node = address<node_slave_t>(nextBlock);
        }
        // close last slave
        node->slave_next = 0;
        return firstBlock;
    }

    inline void touch(uint32_t curr) {
        // fprintf(stderr, "cache::touch %d (current: %d)\n", curr, info.tail);
        if(info.tail == curr) {
            return;
        }
         // bring to tail
        node_t& node = *address<node_t>(curr);
        // assert: node.next != 0
        address<node_t>(node.next)->prev = node.prev;

        if(node.prev) {
            address<node_t>(node.prev)->next = node.next;
        } else { // node is head
            info.head = node.next;
        }
        node.next = 0;
        node.prev = info.tail;
        address<node_t>(info.tail)->next = curr;
        info.tail = curr;
        // fprintf(stderr, "touch: head=%d tail=%d curr=%d prev=%d next=%d\n", info.head, info.tail, curr, node.prev, node.next);
    }


    inline uint32_t& next(node_t& node, uint32_t count) {
        uint32_t* ret = &node.slave_next;
        for(uint32_t i = 1; i < count; i++) {
            ret = &address<node_slave_t>(*ret)->slave_next;
        }
        return *ret;
    }
} cache_t;

void init(void* ptr, uint32_t blocks, uint32_t block_size_shift) {
    uint32_t blocks_total = blocks - (HEADER_SIZE >> block_size_shift);
    cache_t& cache = *static_cast<cache_t*>(ptr);
    static_assert(sizeof(cache.info) <= 32)

    if(cache.info.block_size_shift == block_size_shift &&
       cache.info.blocks_total == blocks_total) { // already initialized
        // fprintf(stderr, "init cache: already initialized\n");
        return;
    }
    memset(&cache, 0, sizeof(cache_t));
    // fprintf(stderr, "sizeof(cache_t):%d==524288 sizeof(cache.info):%d<64\n", sizeof(cache_t), sizeof(cache.info));

    // Use forced write lock to prevent deadlock caused by a crashed thread
    write_lock_t lock(cache.info.lock);
    
    cache.info.block_size_shift = block_size_shift;
    
    cache.info.blocks_total = blocks_total;
    cache.info.blocks_used = 0;
    cache.info.nextBitPos = 1 << (14 - block_size_shift);
    // fprintf(stderr, "init cache: size %d, blocks %d, usage %d/%d\n", size, blocks, cache.info.blocks_used, cache.info.blocks_total);
}

static void dump(cache_t& cache) {
    fprintf(stderr, "== DUMP START: head: %d tail:%d", cache.info.head, cache.info.tail);
    uint32_t prev = 0;

    for(uint32_t curr = cache.info.head; curr;) {
        node_t& node = *cache.address<node_t>(curr);
        if(node.prev != prev) {
            fprintf(stderr, "ERROR: %d->prev=%d != %d\n", curr, node.prev, prev);
        }
        fprintf(stderr, "\n%d(hash:%d prev:%d next:%d)", curr, node.hash, node.prev, node.next);
        for(uint32_t slave_next = node.slave_next; slave_next; ) {
            node_slave_t& slave = *cache.address<node_slave_t>(slave_next);
            fprintf(stderr, "-->%d", slave_next);
            slave_next = slave.slave_next;
        }
        prev = curr;
        curr = node.next;
    }
    if(prev != cache.info.tail) {
        fprintf(stderr, "\nERROR: ended at %d != tail(%d)\n", prev, cache.info.tail);
    } else {
        fprintf(stderr, "\nDUMP END ==\n");
    }

}


inline uint32_t hashsum(const uint16_t* key, size_t keyLen) {
    uint32_t hash = 0xffffffff;
    for(size_t i = 0; i < keyLen; i++) {
        hash = hash * 31 + key[i];
    }
    return hash;
}

void get(void* ptr, const uint16_t* key, size_t keyLen, uint8_t*& retval, size_t& retvalLen) {
    // fprintf(stderr, "cache::get: key len %d\n", keyLen);
    cache_t& cache = *static_cast<cache_t*>(ptr);

    uint32_t hash = hashsum(key, keyLen);

    read_lock_t lock(cache.info.lock);
    uint32_t found = cache.find(key, keyLen, hash);
    // fprintf(stderr, "cache::get hash=%d found=%d\n", hash, found);
    if(found) {
        cache.touch(found);
        node_t* pnode = cache.address<node_t>(found);
        size_t  valLen = retvalLen = pnode->valLen;
        uint8_t* val = retval = new uint8_t[valLen];

        node_slave_t* currentBlock = reinterpret_cast<node_slave_t*>(pnode);
        uint32_t offset = sizeof(node_t) + (keyLen << 1);
        const uint32_t BLK_SIZE = 1 << cache.info.block_size_shift;
        uint32_t capacity = BLK_SIZE - offset;

        while(capacity < valLen) {
            // fprintf(stderr, "copying val (%x+%d) %d bytes\n", currentBlock, offset, capacity);
            memcpy(val, reinterpret_cast<uint8_t*>(currentBlock) + offset, BLK_SIZE - offset);
            val += capacity;
            valLen -= capacity;
            currentBlock = cache.address<node_slave_t>(currentBlock->slave_next);
            offset = sizeof(node_slave_t);
            capacity = BLK_SIZE - sizeof(node_slave_t);
        }
        if(valLen) { // capacity >= valLen
            // fprintf(stderr, "copying remaining val (%x+%d) %d bytes\n", currentBlock, offset, valLen);
            memcpy(val, reinterpret_cast<uint8_t*>(currentBlock) + offset, valLen);
        }
    } else {
        retval = NULL;
    }

    // dump(cache);
}

int set(void* ptr, const uint16_t* key, size_t keyLen, const uint8_t* val, size_t valLen) {
    cache_t& cache = *static_cast<cache_t*>(ptr);
    size_t totalLen = (keyLen << 1) + valLen + sizeof(node_s) - 1;
    const uint32_t BLK_SIZE = 1 << cache.info.block_size_shift;
    uint32_t blocksRequired = totalLen / (BLK_SIZE - sizeof(node_slave_t)) + (totalLen % (BLK_SIZE - sizeof(node_slave_t)) ? 1 : 0);
    // fprintf(stderr, "cache::set: total len %d (%d blocks required)\n", totalLen, blocksRequired);


    if(blocksRequired > cache.info.blocks_total) {
        errno = E2BIG;
        return -1;
    }

    uint32_t hash = hashsum(key, keyLen);

    write_lock_t lock(cache.info.lock);

    // find if key is already exists
    uint32_t found = cache.find(key, keyLen, hash);
    node_t* selectedBlock;

    // fprintf(stderr, "cache::set hash=%d found=%d\n", hash, found);
    if(found) { // update
        cache.touch(found);
        selectedBlock = cache.address<node_t>(found);
        node_t& node = *selectedBlock;
        if(node.blocks > blocksRequired) { // free extra blocks
            uint32_t& slave_next = cache.next(node, blocksRequired);
            // drop remaining blocks
            cache.release(slave_next);
            // fprintf(stderr, "freeing %d blocks (%d used)\n", node.blocks - blocksRequired, cache.info.blocks_used);
            slave_next = 0;
        } else if(node.blocks < blocksRequired) {
            cache.next(node, node.blocks) = cache.allocate(blocksRequired - node.blocks);
        }
        node.blocks = blocksRequired;
    } else { // insert
        // insert into hash table
        uint32_t firstBlock = cache.allocate(blocksRequired);
        // fprintf(stderr, "cache::set allocated new block %d\n", firstBlock);
        selectedBlock = cache.address<node_t>(firstBlock);
        node_t& node = *selectedBlock;
        node.blocks = blocksRequired;

        node.hash = hash;
        uint32_t& hash_head = cache.hashmap[hash & 0xffff];
        node.hash_next = hash_head; // insert into linked list
        hash_head = firstBlock;
        node.keyLen = keyLen;

        // fprintf(stderr, "offering block %d to list\n", firstBlock);
        if(!cache.info.tail) {
            cache.info.head = cache.info.tail = firstBlock;
            node.prev = node.next = 0;
        } else {
            cache.address<node_t>(cache.info.tail)->next = firstBlock;
            node.prev = cache.info.tail;
            node.next = 0;
            cache.info.tail = firstBlock;
        }
        // copy key
        // fprintf(stderr, "copying key (len:%d capacity:%d)\n", keyLen, BLK_SIZE - sizeof(node_t));
        memcpy(selectedBlock->key, key, keyLen << 1);
    }
    selectedBlock->valLen = valLen;

    // keyLen <= 256 < capacity
    // while(capacity < keyLen) {
    //     if(!found) memcpy(reinterpret_cast<uint8_t*>(currentBlock) + offset, key, capacity << 1);
    //     key += capacity;
    //     keyLen -= capacity;
    //     currentBlock = cache.address<node_slave_t>(currentBlock->slave_next);
    //     offset = sizeof(node_slave_t);
    //     capacity = (BLK_SIZE - sizeof(node_slave_t)) >> 1;
    // }
    // if(keyLen) { // capacity >= keyLen
    // }

    // copy values

    node_slave_t* currentBlock = reinterpret_cast<node_slave_t*>(selectedBlock);
    uint32_t offset = sizeof(node_t) + (keyLen << 1);
    uint32_t capacity = BLK_SIZE - offset;

    while(capacity < valLen) {
        // fprintf(stderr, "copying val (%x+%d) %d bytes. next=%d\n", currentBlock, offset, capacity, currentBlock->slave_next);
        memcpy(reinterpret_cast<uint8_t*>(currentBlock) + offset, val, capacity);
        val += capacity;
        valLen -= capacity;
        currentBlock = cache.address<node_slave_t>(currentBlock->slave_next);
        offset = sizeof(node_slave_t);
        capacity = BLK_SIZE - sizeof(node_slave_t);
    }

    if(valLen) { // capacity >= valLen
        // fprintf(stderr, "copying remaining val (%x+%d) %d bytes\n", currentBlock, offset, valLen);
        memcpy(reinterpret_cast<uint8_t*>(currentBlock) + offset, val, valLen);
    }
    return 0;
}


void enumerate(void* ptr, EnumerateCallback& enumerator) {
    cache_t& cache = *static_cast<cache_t*>(ptr);
    read_lock_t lock(cache.info.lock);
    uint32_t curr = cache.info.head;

    while(curr) {
        node_t& node = *cache.address<node_t>(curr);
        enumerator.next(node.key, node.keyLen);
        curr = node.next;
    }
}

bool contains(void* ptr, const uint16_t* key, size_t keyLen) {
    cache_t& cache = *static_cast<cache_t*>(ptr);

    uint32_t hash = hashsum(key, keyLen);

    read_lock_t lock(cache.info.lock);
    return cache.find(key, keyLen, hash);
}

bool unset(void* ptr, const uint16_t* key, size_t keyLen) {
    cache_t& cache = *static_cast<cache_t*>(ptr);

    uint32_t hash = hashsum(key, keyLen);

    write_lock_t lock(cache.info.lock);
    uint32_t found = cache.find(key, keyLen, hash);
    if(found) {
        cache.dropNode(found);
    }
    return found;
}

}
