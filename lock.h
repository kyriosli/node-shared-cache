#ifndef	_LOCK_H
#define	_LOCK_H

#include <stdint.h>

typedef	int32_t	mutex_t;

#ifdef	__GNUC__

#if GCC_VERSION > 46300

template<typename t>
inline t cmpxchg(t& var, t oldval, t newval) {
	__atomic_compare_exchange_n(&var, &oldval, newval, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
	return oldval;
}

#define xchg(var, newval) __atomic_exchange_n(&var, newval, __ATOMIC_ACQ_REL);	
#define	atomic_dec(var) __atomic_fetch_sub(&var, 1, __ATOMIC_ACQ_REL)

#else

#define cmpxchg(var, oldval, newval) __sync_val_compare_and_swap(&var, oldval, newval)

#define xchg(var, newval) __sync_lock_test_and_set(&var, newval)
#define	atomic_dec(var) __sync_fetch_and_sub (&var, 1)

#endif
#endif


#define TSL(mutex) xchg(mutex, 1)
#define SPIN(mutex) while(TSL(mutex))

#ifdef	linux
/* Use futex to implement mutex in linux */
#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>

#define	futex_wait(addr, val) syscall(SYS_futex, addr, FUTEX_WAIT, val, 0, 0, 0)
#define futex_wake(addr, count) syscall(SYS_futex, addr, FUTEX_WAKE, count, 0, 0, 0)

inline void LOCK(mutex_t& mutex) {
	mutex_t c;
	if((c = cmpxchg(mutex, 0, 1))) {
		// locked, c=2 if there is someone waiting
		if(c != 2) c = xchg(mutex, 2);
		while(c) {
			futex_wait(&mutex, 2);
			c = xchg(mutex, 2);
		}
	}
}

inline void UNLOCK(mutex_t& mutex) {
	if(atomic_dec(mutex) != 1) {
		mutex = 0;
		futex_wake(&mutex, 1);
	}
}

#else
#define LOCK(mutex) SPIN(mutex) while(mutex)
#define UNLOCK(mutex) __sync_lock_release(&mutex)
#endif

typedef struct {
	mutex_t count_lock;
	mutex_t mutex;
	uint32_t readers;
} rw_lock_t;

typedef struct read_lock_s {
	rw_lock_t& lock;
    inline read_lock_s(rw_lock_t& lock) : lock(lock) {
		LOCK(lock.count_lock);
		if(++lock.readers == 1) {
			LOCK(lock.mutex);
		}
		UNLOCK(lock.count_lock);
    }
    inline ~read_lock_s() {
		LOCK(lock.count_lock);
		if(--lock.readers == 0) {
			UNLOCK(lock.mutex);
		}
		UNLOCK(lock.count_lock);
	}
} read_lock_t;

typedef struct write_lock_s {
	rw_lock_t& lock;
    inline write_lock_s(rw_lock_t& lock) : lock(lock) {
		LOCK(lock.mutex);
    }
    inline ~write_lock_s() {
		UNLOCK(lock.mutex);
    }
} write_lock_t;

#endif
