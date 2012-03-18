/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  stub.h
//  cinquain-meta
//
//  Created by Jinglei Ren  <jinglei.ren@gmail.com> on 3/11/12.
//

#ifndef CINQUAIN_META_STUB_H_
#define CINQUAIN_META_STUB_H_

#ifndef __KERNEL__
// Supplement of kernel structure to user space.
// Subject to user-space adjustment.

#include "util.h"

struct writeback_control {
  // only for interface compatibility 
};

struct vfsmount {
  // Only for interface compatibility
};

struct path {
  struct dentry *dentry;
  
  // Omits a vfsmount-related member.
  // User-space implementation does not handle vfsmount.
};

struct iattr {
	unsigned int	ia_valid;
	umode_t		ia_mode;
	uid_t		ia_uid;
	gid_t		ia_gid;
	loff_t		ia_size;
	struct timespec	ia_atime;
	struct timespec	ia_mtime;
	struct timespec	ia_ctime;
};

struct super_block {
  //	struct list_head	s_list;		/* Keep this first */
	dev_t			s_dev;		/* search index; _not_ kdev_t */
	unsigned char		s_dirt;
	unsigned char		s_blocksize_bits;
	unsigned long		s_blocksize;
	loff_t			s_maxbytes;	/* Max file size */
	struct file_system_type	*s_type;
	const struct super_operations	*s_op;
  //	const struct dquot_operations	*dq_op;
  //	const struct quotactl_ops	*s_qcop;
	const struct export_operations *s_export_op;
	unsigned long		s_flags;
	unsigned long		s_magic;
	struct dentry		*s_root;
  //	struct rw_semaphore	s_umount;
  //	struct mutex		s_lock;
	int			s_count;
  //	atomic_t		s_active;
  //#ifdef CONFIG_SECURITY
  //	void                    *s_security;
  //#endif
  //	const struct xattr_handler **s_xattr;
  //  
  //	struct list_head	s_inodes;	/* all inodes */
  //	struct hlist_bl_head	s_anon;		/* anonymous dentries for (nfs) exporting */
  //#ifdef CONFIG_SMP
  //	struct list_head __percpu *s_files;
  //#else
  //	struct list_head	s_files;
  //#endif
  //	/* s_dentry_lru, s_nr_dentry_unused protected by dcache.c lru locks */
  //	struct list_head	s_dentry_lru;	/* unused dentry lru */
  //	int			s_nr_dentry_unused;	/* # of dentry on lru */
  //  
  //	struct block_device	*s_bdev;
  //	struct backing_dev_info *s_bdi;
  //	struct mtd_info		*s_mtd;
  //	struct list_head	s_instances;
  //	struct quota_info	s_dquot;	/* Diskquota specific options */
  //  
  //	int			s_frozen;
  //	wait_queue_head_t	s_wait_unfrozen;
  
	char s_id[32];				/* Informational name */
	u8 s_uuid[16];				/* UUID */
  
	void 			*s_fs_info;	/* Filesystem private info */
	fmode_t		s_mode;
  
	/* Granularity of c/m/atime in ns.
   Cannot be worse than a second */
	u32		   s_time_gran;
  
	/*
	 * The next field is for VFS *only*. No filesystems have any business
	 * even looking at it. You had been warned.
	 */
  //	struct mutex s_vfs_rename_mutex;	/* Kludge */
  
	/*
	 * Filesystem subtype.  If non-empty the filesystem type field
	 * in /proc/mounts will be "type.subtype"
	 */
	char *s_subtype;
  
	/*
	 * Saved mount options for lazy filesystems using
	 * generic_show_options()
	 */
	char *s_options;
  //	const struct dentry_operations *s_d_op; /* default d_op for dentries */
};

struct nameidata {
  struct path path;
  struct qstr last;
  struct path root;
  struct inode *inode; /* path.dentry.d_inode */
  unsigned int flags;
  unsigned int seq;
  int last_type;
  unsigned depth;
  char *saved_names[MAX_NESTED_LINKS + 1];
  
  // Omits intent data
};

struct inode {
	/* RCU path lookup touches following: */
	umode_t		i_mode;
	uid_t			i_uid;
	gid_t			i_gid;
	const struct inode_operations	*i_op;
	struct super_block	*i_sb;
  
	spinlock_t		  i_lock;	/* i_blocks, i_bytes, maybe i_size */
	unsigned int		i_flags;
  // struct mutex		i_mutex;
  
	unsigned long		i_state;
	unsigned long		dirtied_when;	/* jiffies of first dirtying */
  
  //  struct hlist_node	i_hash;
  //  struct list_head	i_wb_list;	/* backing dev IO list */
  //  struct list_head	i_lru;		/* inode LRU list */
  //  struct list_head	i_sb_list;
  //  union {
  //    struct list_head	i_dentry;
  //    struct rcu_head		i_rcu;
  //  };
	unsigned long		i_ino;
	//atomic_t		i_count;
	unsigned int		i_nlink;
	dev_t			i_rdev;
	unsigned int		i_blkbits;
	u64			i_version;
	loff_t			i_size;
  // #ifdef __NEED_I_SIZE_ORDERED
  // 	seqcount_t		i_size_seqcount;
  // #endif
	struct timespec		i_atime;
	struct timespec		i_mtime;
	struct timespec		i_ctime;
	blkcnt_t		i_blocks;
	unsigned short          i_bytes;
  // struct rw_semaphore	i_alloc_sem;
	const struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
	struct file_lock	*i_flock;
  struct address_space	*i_mapping;
  //  struct address_space	i_data;
  //#ifdef CONFIG_QUOTA
  //	struct dquot		*i_dquot[MAXQUOTAS];
  //#endif
  //	struct list_head	i_devices;
  //	union {
  //		struct pipe_inode_info	*i_pipe;
  //		struct block_device	*i_bdev;
  //		struct cdev		*i_cdev;
  //	};
  
	__u32			i_generation;
  
  //#ifdef CONFIG_FSNOTIFY
  //	__u32			i_fsnotify_mask; /* all events this inode cares about */
  //	struct hlist_head	i_fsnotify_marks;
  //#endif
  //  
  //#ifdef CONFIG_IMA
  //	atomic_t		i_readcount; /* struct files open RO */
  //#endif
  //	atomic_t		i_writecount;
  //#ifdef CONFIG_SECURITY
  //	void			*i_security;
  //#endif
  //#ifdef CONFIG_FS_POSIX_ACL
  //	struct posix_acl	*i_acl;
  //	struct posix_acl	*i_default_acl;
  //#endif
	void			*i_private; /* fs or device private pointer */
};

struct dentry {
  struct inode *d_inode;
  struct dentry *parent;
  struct qstr d_name;
  void *d_fsdata;
  
  // Omits all dcache-related members.
  // User-space implementation should provide independent dentry cache.
};

struct file {
	/*
	 * fu_list becomes invalid after file_free is called and queued via
	 * fu_rcuhead for RCU freeing
	 */
  //	union {
  //		struct list_head	fu_list;
  //		struct rcu_head 	fu_rcuhead;
  //	} f_u;
	struct path	f_path;
#define f_dentry	f_path.dentry
#define f_vfsmnt	f_path.mnt
	const struct file_operations	*f_op;
	spinlock_t		f_lock;  /* f_ep_links, f_flags, no IRQ */
  //#ifdef CONFIG_SMP
  //	int			f_sb_list_cpu;
  //#endif
  //	atomic_long_t		f_count;
	unsigned int 		f_flags;
	fmode_t			f_mode;
	loff_t			f_pos;
  //	struct fown_struct	f_owner;
	const struct cred	*f_cred;
  //	struct file_ra_state	f_ra;
  
	u64			f_version;
  //#ifdef CONFIG_SECURITY
  //	void			*f_security;
  //#endif
	/* needed for tty driver, and maybe others */
	void			*private_data;
  
  //#ifdef CONFIG_EPOLL
  //	/* Used by fs/eventpoll.c to link all the hooks to this file */
  //	struct list_head	f_ep_links;
  //#endif /* #ifdef CONFIG_EPOLL */
  //	struct address_space	*f_mapping;
  //#ifdef CONFIG_DEBUG_WRITECOUNT
  //	unsigned long f_mnt_write_state;
  //#endif
};

/**
 * inode_init_always - perform inode structure intialisation
 * @sb: superblock inode belongs to
 * @inode: inode to initialise
 *
 * These are initializations that need to be done on every inode
 * allocation as the fields are not initialised by slab allocation.
 */
static int inode_init_always(struct super_block *sb, struct inode *inode)
{
  static const struct inode_operations empty_iops;
  static const struct file_operations empty_fops;
  // struct address_space *const mapping = &inode->i_data;
  
  inode->i_sb = sb;
  // inode->i_blkbits = sb->s_blocksize_bits;
  inode->i_flags = 0;
  // atomic_set(&inode->i_count, 1);
  inode->i_op = &empty_iops;
  inode->i_fop = &empty_fops;
  inode->i_nlink = 1;
  inode->i_uid = 0;
  inode->i_gid = 0;
  // atomic_set(&inode->i_writecount, 0);
  inode->i_size = 0;
  inode->i_blocks = 0;
  inode->i_bytes = 0;
  inode->i_generation = 0;
  // #ifdef CONFIG_QUOTA
  //   memset(&inode->i_dquot, 0, sizeof(inode->i_dquot));
  // #endif
  // inode->i_pipe = NULL;
  // inode->i_bdev = NULL;
  // inode->i_cdev = NULL;
  // inode->i_rdev = 0;
  inode->dirtied_when = 0;
 
  // if (security_inode_alloc(inode))
  //   goto out;
  // spin_lock_init(&inode->i_lock);
  // lockdep_set_class(&inode->i_lock, &sb->s_type->i_lock_key);
  
  // mutex_init(&inode->i_mutex);
  // lockdep_set_class(&inode->i_mutex, &sb->s_type->i_mutex_key);
  
  // init_rwsem(&inode->i_alloc_sem);
  // lockdep_set_class(&inode->i_alloc_sem, &sb->s_type->i_alloc_sem_key);
  
  // mapping->a_ops = &empty_aops;
  // mapping->host = inode;
  // mapping->flags = 0;
  // mapping_set_gfp_mask(mapping, GFP_HIGHUSER_MOVABLE);
  // mapping->assoc_mapping = NULL;
  // mapping->backing_dev_info = &default_backing_dev_info;
  // mapping->writeback_index = 0;
  
  /*
   * If the block_device provides a backing_dev_info for client
   * inodes then use that.  Otherwise the inode share the bdev's
   * backing_dev_info.
   */
  // if (sb->s_bdev) {
  //   struct backing_dev_info *bdi;
  //
  // bdi = sb->s_bdev->bd_inode->i_mapping->backing_dev_info;
  // mapping->backing_dev_info = bdi;
  // }
  // inode->i_private = NULL;
  // inode->i_mapping = mapping;
  // #ifdef CONFIG_FS_POSIX_ACL
  //   inode->i_acl = inode->i_default_acl = ACL_NOT_CACHED;
  // #endif
  
  // #ifdef CONFIG_FSNOTIFY
  //   inode->i_fsnotify_mask = 0;
  // #endif

  // this_cpu_inc(nr_inodes);

  return 0;
out:
  return -ENOMEM;
}

/**
 * d_add - add dentry to hash queues
 * @entry: dentry to add
 * @inode: the inode to attach to this dentry
 *
 * This adds the entry to the hash queues and initializes @inode.
 * The entry was actually filled in earlier during d_alloc().
*/
static inline void d_add(struct dentry *entry, struct inode *inode) {
  // User-space implementation for dentry cache.
  // This function attaches the inode to the dentry.
  entry->d_inode = inode;
}

/**
 * d_splice_alias - splice a disconnected dentry into the tree if one exists
 * @inode:  the inode which may have a disconnected dentry
 * @dentry: a negative dentry which we want to point to the inode.
 *
 * If inode is a directory and has a 'disconnected' dentry (i.e. IS_ROOT and
 * DCACHE_DISCONNECTED), then d_move that in place of the given dentry
 * and return it, else simply d_add the inode to the dentry and return NULL.
 *
 * This is needed in the lookup routine of any filesystem that is exportable
 * (via knfsd) so that we can build dcache paths to directories effectively.
 *
 * If a dentry was found and moved, then it is returned.  Otherwise NULL
 * is returned.  This matches the expected return value of ->lookup.
 *
*/
static inline struct dentry *d_splice_alias(struct inode *inode,
                                            struct dentry *dentry) {
  // Needs complete user-space implementation
  return dentry;
}

/**
 * inode_sb_list_add - add inode to the superblock list of inodes
 * @inode: inode to add
 */
static inline void inode_sb_list_add(struct inode *inode) {
  // This function can be left empty in user space for convenience.
  // Tailor it as user-space implementation needs.
  return;
}

#endif // __KERNEL__

#endif // CINQUAIN_META_STUB_H_
