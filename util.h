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

#define tag_malloc() \
    ((struct cinq_tag *)malloc(sizeof(struct cinq_tag)))
#define tag_mfree(p) (free(p))

#define inode_malloc() \
    ((struct inode *)malloc(sizeof(struct inode)))
#define inode_mfree(p) (free(p))

#define cnode_malloc() \
    ((struct cinq_inode *)malloc(sizeof(struct cinq_inode)))
#define cnode_mfree(p) (free(p))

#define CURRENT_SEC (time(NULL))

#endif // __KERNEL__


#ifndef __KERNEL__
// Private: those in kernel but not user-space

typedef unsigned int umode_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef uint32_t __u32;
typedef uint64_t u64;
typedef long long loff_t;

typedef pthread_rwlock_t rwlock_t;
#define rwlock_init(lock_p) (pthread_rwlock_init(lock_p, NULL))
#define read_lock(lock_p) (pthread_rwlock_rdlock(lock_p))
#define read_unlock(lock_p) (pthread_rwlock_unlock(lock_p))
#define write_lock(lock_p) (pthread_rwlock_wrlock(lock_p))
#define write_unlock(lock_p) (pthread_rwlock_unlock(lock_p))

#define unlikely(cond) (cond)

struct qstr { // include/linux/dcache.h
  unsigned int hash;
  unsigned int len;
  const unsigned char *name;
};

static inline void * ERR_PTR(long error) { // include/linux/err.h
  return (void *) error;
}

// include/asm-generic/errno-base.h
#define ENOENT 2 /* No such file or directory */
// include/asm-generic/errno.h
#define ENAMETOOLONG 36 /* File name too long */
#define EADDRNOTAVAIL 99 /* Cannot assign requested address */

#define MAX_NESTED_LINKS 6

#include "stub.h"

struct super_operations {
  struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *);
  void (*dirty_inode) (struct inode *, int flags);
	int (*write_inode) (struct inode *, struct writeback_control *wbc);
	int (*drop_inode) (struct inode *);
	void (*evict_inode) (struct inode *);
	void (*put_super) (struct super_block *);
	void (*write_super) (struct super_block *);
	int (*sync_fs)(struct super_block *sb, int wait);
};

struct kstat { // include/linux/stat.h
  u64 ino;
  dev_t dev;
  umode_t mode;
  unsigned int nlink;
  uid_t uid;
  gid_t gid;
  dev_t rdev;
  loff_t size;
  struct timespec  atime;
  struct timespec mtime;
  struct timespec ctime;
  unsigned long blksize;
  unsigned long long blocks;
};

struct inode_operations {
	struct dentry * (*lookup) (struct inode *, struct dentry *, struct nameidata *);
	void *(*follow_link) (struct dentry *, struct nameidata *);
	int (*permission) (struct inode *, int);
	struct posix_acl * (*get_acl)(struct inode *, int);
  
	int (*readlink) (struct dentry *, char *, int);
	void (*put_link) (struct dentry *, struct nameidata *, void *);
  
	int (*create) (struct inode *, struct dentry *, int, struct nameidata *);
	int (*link) (struct dentry *, struct inode *, struct dentry *);
	int (*unlink) (struct inode *, struct dentry *);
	int (*symlink) (struct inode *, struct dentry *, const char *);
	int (*mkdir) (struct inode *, struct dentry *, int);
	int (*rmdir) (struct inode *, struct dentry *);
	int (*mknod) (struct inode *, struct dentry *, int, dev_t);
	int (*rename) (struct inode *, struct dentry *,
                 struct inode *, struct dentry *);
	void (*truncate) (struct inode *);
	int (*setattr) (struct dentry *, struct iattr *);
	int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
	int (*setxattr) (struct dentry *, const char *, const void *, size_t, int);
	ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
	ssize_t (*listxattr) (struct dentry *, char *, size_t);
	int (*removexattr) (struct dentry *, const char *);
	void (*truncate_range)(struct inode *, loff_t, loff_t);
};

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
