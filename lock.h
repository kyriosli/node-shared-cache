#ifndef	_LOCK_H
#define	_LOCK_H


typedef struct read_preferred_rw_lock_s {
    uint32_t    mutex; // mutex for read/write
    uint32_t    rdmutex; // mutex for readers count
    uint32_t    readers; // readers count
} rw_lock_t;


// use a spin lock to lock the mutex
#define LOCK(mutex) while(__sync_lock_test_and_set(&mutex, 1))
#define UNLOCK(mutex) __sync_lock_release(&mutex)

typedef struct read_lock_s {
    rw_lock_t* lock;
    inline read_lock_s(rw_lock_t& rwlock) : lock(&rwlock) {
        LOCK(lock->rdmutex);
        if(++lock->readers == 1) {
            LOCK(lock->mutex);
        }
        UNLOCK(lock->rdmutex);
    }
    inline ~read_lock_s() {
        LOCK(lock->rdmutex);
        if(--lock->readers == 0) {
            UNLOCK(lock->mutex);
        }
        UNLOCK(lock->rdmutex);
    }
} read_lock_t;

typedef struct write_lock_s {
    rw_lock_t* lock;
    inline write_lock_s(rw_lock_t& rwlock) : lock(&rwlock) {
        LOCK(lock->mutex);
    }
    inline ~write_lock_s() {
        UNLOCK(lock->mutex);
    }
} write_lock_t;

#undef LOCK
#undef UNLOCK

#endif