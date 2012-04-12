/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  util.h
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/10/12.
//

#ifndef CINQUAIN_META_UTIL_H_
#define CINQUAIN_META_UTIL_H_

/* All kernel portability issues are addressed in this header. */

#ifdef __KERNEL__

#include <linux/list.h>

#else

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "atomic.h"
#include "list.h"

#endif

#ifdef CINQ_DEBUG
atomic_t num_inodes_;
#endif // CINQ_DEBUG

#ifndef CINQ_DEBUG
#define DEBUG_ON_(cond, ...) ((void)(cond))
#define DEBUG_(...)
#endif // CINQ_DEBUG

#ifdef __KERNEL__ // intended for Linux
/* Kernel (exchangable) */


#else
/* User space (exchangable) */

// include/linux/pagemap.h
#define PAGE_CACHE_SHIFT        13 // 8KB
#define PAGE_CACHE_SIZE         ((uint64_t)1 << PAGE_CACHE_SHIFT)
#define PAGE_CACHE_MASK         (~(PAGE_SIZE - 1))

#ifdef CINQ_DEBUG
#define DEBUG_ON_(cond, ...) if (cond) { fprintf(stderr, __VA_ARGS__); }
#define DEBUG_(...) (fprintf(stderr, __VA_ARGS__))
#endif // CINQ_DEBUG

#define inode_free_(p) (free(p))

#define fsnode_malloc_() \
    ((struct cinq_fsnode *)malloc(sizeof(struct cinq_fsnode)))
#define fsnode_free_(p) (free(p))

#define tag_malloc_() \
    ((struct cinq_tag *)malloc(sizeof(struct cinq_tag)))
#define tag_free_(p) (free(p))

#define cnode_malloc_() \
    ((struct cinq_inode *)malloc(sizeof(struct cinq_inode)))
#define cnode_free_(p) (free(p))

#define journal_malloc_() \
    ((struct cinq_journal *)malloc(sizeof(struct cinq_journal)))
#define journal_free_(p) (free(p))

#define journal_entry_malloc_() \
    ((struct journal_entry *)malloc(sizeof(struct journal_entry)))
#define journal_entry_free_(p) (free(p))

#define kmalloc(n) malloc(n)

#define CURRENT_TIME ((struct timespec) { time(NULL), 0 })

#endif // __KERNEL__


#ifndef __KERNEL__
// Private: those in kernel but not user-space

typedef unsigned int umode_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef uint32_t __u32;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned fmode_t;

#ifndef _SYS_TYPES_H // linux sys/types.h has defined loff_t
typedef long long loff_t;
#endif

typedef pthread_rwlock_t rwlock_t;
#define RW_LOCK_UNLOCKED PTHREAD_RWLOCK_INITIALIZER
#define rwlock_init(lock_p) (pthread_rwlock_init(lock_p, NULL))
#define read_lock(lock_p) (pthread_rwlock_rdlock(lock_p))
#define read_unlock(lock_p) (pthread_rwlock_unlock(lock_p))
#define write_lock(lock_p) (pthread_rwlock_wrlock(lock_p))
#define write_unlock(lock_p) (pthread_rwlock_unlock(lock_p))

typedef pthread_mutex_t spinlock_t;
#define SPIN_LOCK_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define spin_lock_init(lock_p) (pthread_mutex_init(lock_p, NULL))
#define spin_lock(lock_p) (pthread_mutex_lock(lock_p))
#define spin_trylock(lock_p) (pthread_mutex_trylock(lock_p))
#define spin_unlock(lock_p) (pthread_mutex_unlock(lock_p))

#define rd_release_return(lock_p, err) return (read_unlock(lock_p), err)
#define wr_release_return(lock_p, err) return (write_unlock(lock_p), err)
#define sp_release_return(lock_p, err) return (spin_unlock(lock_p), err)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define CURRENT_TIME_SEC ((struct timespec) { time(NULL), 0 })

static inline void * ERR_PTR(long error) { // include/linux/err.h
  return (void *) error;
}

static inline long PTR_ERR(const void *ptr) { // include/linux/err.h
  return (long) ptr;
}

#include "include-asm-generic-errno.h"
#include "include-linux-stat.h"

// linux/err.h
#define MAX_ERRNO 4095
#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)
#define IS_ERR(ptr) (IS_ERR_VALUE((unsigned long)ptr))

// linux/sched.h
#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2

// linux/fs.h: the fs-independent mount-flags
#define MS_ACTIVE       (1<<30)

#define MAX_LFS_FILESIZE 0x7fffffffffffffffUL

#define MAX_NESTED_LINKS 6

#endif // __KERNEL__

/* Non-portability utilities */

#include "uthash/uthash.h"

#define CINQ_MAGIC 0x3122
#define FILE_HASH_WIDTH 16 // bytes
#define MAX_NAME_LEN 255 // max value

#define META_FS ((void *)-EPERM)

#define HASH_FIND_BY_STR(hh, head, findstr, out) \
    HASH_FIND(hh, head, findstr, strlen(findstr), out)
#define HASH_ADD_BY_STR(hh, head, strfield, add) \
    HASH_ADD(hh, head, strfield, strlen(add->strfield), add)
#define HASH_FIND_BY_INT(hh, head, findint, out) \
    HASH_FIND(hh, head, findint, sizeof(int), out)
#define HASH_ADD_BY_INT(hh, head, intfield, add) \
    HASH_ADD(hh, head, intfield, sizeof(int), add)
#define HASH_FIND_BY_PTR(hh, head, findptr, out) \
    HASH_FIND(hh, head, findptr, sizeof(void *), out)
#define HASH_ADD_BY_PTR(hh, head, ptrfield, add) \
    HASH_ADD(hh, head, ptrfield, sizeof(void *), add)

#endif // CINQUAIN_META_UTIL_H_
