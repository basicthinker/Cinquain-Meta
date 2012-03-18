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

#define cnode_malloc() \
    ((struct cinq_inode *)malloc(sizeof(struct cinq_inode)))
#define cnode_mfree(p) (free(p))

#endif // __KERNEL__

static inline struct timespec current_time_() {
#ifdef __KERNEL__
  return current_kernel_time();
#else
  struct timespec ts;
  ts.tv_sec = time(NULL);
  return ts;
#endif
}

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
#define spin_lock(lock_p) (pthread_rwlock_wrlock(lock_p))
#define spin_unlock(lock_p) (pthread_rwlock_unlock(lock_p))

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

#define MAX_NESTED_LINKS 6

struct super_block;
struct writeback_control;
struct vfsmount;
struct inode;
struct iattr;
struct dentry;
struct nameidata;
struct file;

struct super_operations {
  struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *);
  
  void (*dirty_inode) (struct inode *);
	int (*write_inode) (struct inode *, struct writeback_control *wbc);
	int (*drop_inode) (struct inode *);
	void (*evict_inode) (struct inode *);
	void (*put_super) (struct super_block *);
	void (*write_super) (struct super_block *);
	int (*sync_fs)(struct super_block *sb, int wait);
	int (*freeze_fs) (struct super_block *);
	int (*unfreeze_fs) (struct super_block *);
  // int (*statfs) (struct dentry *, struct kstatfs *);
	int (*remount_fs) (struct super_block *, int *, char *);
	void (*umount_begin) (struct super_block *);
  
  // int (*show_options)(struct seq_file *, struct vfsmount *);
	// int (*show_devname)(struct seq_file *, struct vfsmount *);
	// int (*show_path)(struct seq_file *, struct vfsmount *);
	// int (*show_stats)(struct seq_file *, struct vfsmount *);
#ifdef CONFIG_QUOTA
	ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
	ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
#endif
	// int (*bdev_try_to_free_page)(struct super_block*, struct page*, gfp_t);
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
	struct dentry * (*lookup) (struct inode *,struct dentry *, struct nameidata *);
	void * (*follow_link) (struct dentry *, struct nameidata *);
	int (*permission) (struct inode *, int, unsigned int);
	int (*check_acl)(struct inode *, int, unsigned int);
  
	int (*readlink) (struct dentry *, char *,int);
	void (*put_link) (struct dentry *, struct nameidata *, void *);
  
	int (*create) (struct inode *,struct dentry *,int, struct nameidata *);
	int (*link) (struct dentry *,struct inode *,struct dentry *);
	int (*unlink) (struct inode *,struct dentry *);
	int (*symlink) (struct inode *,struct dentry *,const char *);
	int (*mkdir) (struct inode *,struct dentry *,int);
	int (*rmdir) (struct inode *,struct dentry *);
	int (*mknod) (struct inode *,struct dentry *,int,dev_t);
	int (*rename) (struct inode *, struct dentry *,
                 struct inode *, struct dentry *);
	void (*truncate) (struct inode *);
	int (*setattr) (struct dentry *, struct iattr *);
	int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
	int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
	ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
	ssize_t (*listxattr) (struct dentry *, char *, size_t);
	int (*removexattr) (struct dentry *, const char *);
	void (*truncate_range)(struct inode *, loff_t, loff_t);
	// int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start, 
  //               u64 len);
};

typedef int (*filldir_t)(void *, const char *, int, loff_t, u64, unsigned);

/*
 * NOTE:
 * all file operations except setlease can be called without
 * the big kernel lock held in all filesystems.
 */
struct file_operations {
	// struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char *, size_t, loff_t *);
  // ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	// ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	int (*readdir) (struct file *, void *, filldir_t);
	// unsigned int (*poll) (struct file *, struct poll_table_struct *);
	// long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	// long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	// int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
	// int (*flush) (struct file *, fl_owner_t id);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, int datasync);
	// int (*aio_fsync) (struct kiocb *, int datasync);
	// int (*fasync) (int, struct file *, int);
	// int (*lock) (struct file *, int, struct file_lock *);
	// ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	// int (*flock) (struct file *, int, struct file_lock *);
	// ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
	// ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
	// int (*setlease)(struct file *, long, struct file_lock **);
	long (*fallocate)(struct file *file, int mode, loff_t offset,
                    loff_t len);
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
