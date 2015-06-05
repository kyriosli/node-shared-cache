#ifndef	_LOCK_H
#define	_LOCK_H

#include<stdint.h>
#include<sys/file.h>

typedef struct read_lock_s {
    int fd;
    inline read_lock_s(int fd) : fd(fd) {
        flock(fd, LOCK_SH);
    }
    inline ~read_lock_s() {
        flock(fd, LOCK_UN);
    }
} read_lock_t;

typedef struct write_lock_s {
    int fd;
    inline write_lock_s(int fd) : fd(fd) {
        flock(fd, LOCK_EX);
    }
    inline ~write_lock_s() {
        flock(fd, LOCK_UN);
    }
} write_lock_t;

#endif