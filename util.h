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

#ifdef __KERNEL__ // intended for Linux
/* Kernel (exchangable) */

#define CURRENT_SEC (current_kernel_time().tv_sec)

#else
/* User space (exchangable) */
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>

#define log(...) (fprintf(stderr, __VA_ARGS__))

#define fsnode_malloc() \
    ((struct cinq_fsnode *)malloc(sizeof(struct cinq_fsnode)))
#define fsnode_mfree(p) (free(p))

#define inode_malloc() \
    ((inode_t *)malloc(sizeof(inode_t)))
#define inode_mfree(p) (free(p))

#define CURRENT_SEC (time(NULL))

#endif // __KERNEL__


#ifndef __KERNEL__
// Private: those in kernel but not user-space

typedef unsigned int umode_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef uint32_t __u32;
typedef long long loff_t;

typedef pthread_rwlock_t rwlock_t;
#define rwlock_init(lock_p) (pthread_rwlock_init(lock_p, NULL))
#define read_lock(lock_p) (pthread_rwlock_rdlock(lock_p))
#define read_unlock(lock_p) (pthread_rwlock_unlock(lock_p))
#define write_lock(lock_p) (pthread_rwlock_wrlock(lock_p))
#define write_unlock(lock_p) (pthread_rwlock_unlock(lock_p))

struct qstr { // include/linux/dcache.h
  unsigned int hash;
  unsigned int len;
  const unsigned char *name;
};

typedef struct cinq_inode inode_t;

static inline void * ERR_PTR(long error) { // include/linux/err.h
  return (void *) error;
}

// include/asm-generic/errno-base.h
#define ENOENT 2 /* No such file or directory */
// include/asm-generic/errno.h
#define ENAMETOOLONG 36 /* File name too long */
#define EADDRNOTAVAIL 99 /* Cannot assign requested address */

#endif // __KERNEL__



/* Non-portability utilities */

#include "uthash.h"

#define FILE_HASH_WIDTH 16 // bytes
#define MAX_NAME_LEN 255 // max value
#define MAX_NESTED_LINKS 6

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
#define HASH_REMOVE(hh, head, delptr) \
    HASH_DELETE(hh, head, delptr)

#endif // CINQUAIN_META_UTIL_H_
