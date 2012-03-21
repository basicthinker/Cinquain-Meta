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

#else
/* User space (exchangable) */
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define LOG(...) (fprintf(stdout, __VA_ARGS__))
#ifdef CINQ_DEBUG
#define DEBUG_ON_(cond, ...) if (cond) { fprintf(stderr, __VA_ARGS__); }
#define DEBUG_(...) (fprintf(stderr, __VA_ARGS__))
#else
#define DEBUG_ON_(cond, ...) (cond)
#define DEBUG_(...)
#endif // CINQ_DEBUG

#define fsnode_malloc() \
    ((struct cinq_fsnode *)malloc(sizeof(struct cinq_fsnode)))
#define fsnode_mfree(p) (free(p))

#define tag_malloc() \
    ((struct cinq_tag *)malloc(sizeof(struct cinq_tag)))
#define tag_mfree(p) (free(p))

#define inode_malloc() \
    ((struct inode *)malloc(sizeof(struct inode)))
#define inode_mfree(p) (free(p))

#define dentry_malloc() \
    ((struct dentry *)malloc(sizeof(struct dentry)))
#define dentry_mfree(p) (free(p))

#define cnode_malloc() \
    ((struct cinq_inode *)malloc(sizeof(struct cinq_inode)))
#define cnode_mfree(p) (free(p))

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
typedef long long loff_t;
typedef unsigned fmode_t;
typedef uint64_t blkcnt_t;

typedef pthread_rwlock_t rwlock_t;
#define rwlock_init(lock_p) (pthread_rwlock_init(lock_p, NULL))
#define read_lock(lock_p) (pthread_rwlock_rdlock(lock_p))
#define read_unlock(lock_p) (pthread_rwlock_unlock(lock_p))
#define write_lock(lock_p) (pthread_rwlock_wrlock(lock_p))
#define write_unlock(lock_p) (pthread_rwlock_unlock(lock_p))

typedef pthread_rwlock_t spinlock_t;
#define spin_lock_init(lock_p) (pthread_rwlock_init(lock_p, NULL))
#define spin_lock(lock_p) (pthread_rwlock_wrlock(lock_p))
#define spin_unlock(lock_p) (pthread_rwlock_unlock(lock_p))

struct list_head {
  struct list_head *next, *prev;
};
#include "list.h"

#define unlikely(cond) (cond)

#define CURRENT_TIME_SEC ((struct timespec) { time(NULL), 0 })

static inline void * ERR_PTR(long error) { // include/linux/err.h
  return (void *) error;
}

// include/asm-generic/errno-base.h
#define	EPERM     1   /* Operation not permitted */
#define	ENOENT    2   /* No such file or directory */
#define	ESRCH     3   /* No such process */
#define	EINTR     4   /* Interrupted system call */
#define	EIO       5   /* I/O error */
#define	ENXIO     6   /* No such device or address */
#define	E2BIG     7   /* Argument list too long */
#define	ENOEXEC		8   /* Exec format error */
#define	EBADF     9   /* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY     16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV     18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG     27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS     30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE     32	/* Broken pipe */
#define	EDOM      33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */

// include/asm-generic/errno.h
#define ENAMETOOLONG 36 /* File name too long */
#define EADDRNOTAVAIL 99 /* Cannot assign requested address */

// linux/stat.h
#define S_IFMT   00170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)

#define MAX_NESTED_LINKS 6

#include "vfs.h"

#endif // __KERNEL__


/* Non-portability utilities */

#include "uthash.h"

#define FILE_HASH_WIDTH 16 // bytes
#define MAX_NAME_LEN 255 // max value

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
