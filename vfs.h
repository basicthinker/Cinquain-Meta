/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  vfs.h
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/18/12.
//

#ifndef CINQUAIN_META_VFS_H_
#define CINQUAIN_META_VFS_H_

// This is a partial header file depending on util.h.
// util.h should be included instead of this file in most cases.

#ifndef __KERNEL__

// include/linux/fs.h
/*
 * Inode state bits.  Protected by inode->i_lock
 *
 * Three bits determine the dirty state of the inode, I_DIRTY_SYNC,
 * I_DIRTY_DATASYNC and I_DIRTY_PAGES.
 *
 * Four bits define the lifetime of an inode.  Initially, inodes are I_NEW,
 * until that flag is cleared.  I_WILL_FREE, I_FREEING and I_CLEAR are set at
 * various stages of removing an inode.
 *
 * Two bits are used for locking and completion notification, I_NEW and I_SYNC.
 *
 * I_DIRTY_SYNC		Inode is dirty, but doesn't have to be written on
 *			fdatasync().  i_atime is the usual cause.
 * I_DIRTY_DATASYNC	Data-related inode changes pending. We keep track of
 *			these changes separately from I_DIRTY_SYNC so that we
 *			don't have to write inode on fdatasync() when only
 *			mtime has changed in it.
 * I_DIRTY_PAGES	Inode has dirty pages.  Inode itself may be clean.
 * I_NEW		Serves as both a mutex and completion notification.
 *			New inodes set I_NEW.  If two processes both create
 *			the same inode, one of them will release its inode and
 *			wait for I_NEW to be released before returning.
 *			Inodes in I_WILL_FREE, I_FREEING or I_CLEAR state can
 *			also cause waiting on I_NEW, without I_NEW actually
 *			being set.  find_inode() uses this to prevent returning
 *			nearly-dead inodes.
 * I_WILL_FREE		Must be set when calling write_inode_now() if i_count
 *			is zero.  I_FREEING must be set when I_WILL_FREE is
 *			cleared.
 * I_FREEING		Set when inode is about to be freed but still has dirty
 *			pages or buffers attached or the inode itself is still
 *			dirty.
 * I_CLEAR		Added by end_writeback().  In this state the inode is clean
 *			and can be destroyed.  Inode keeps I_FREEING.
 *
 *			Inodes that are I_WILL_FREE, I_FREEING or I_CLEAR are
 *			prohibited for many purposes.  iget() must wait for
 *			the inode to be completely released, then create it
 *			anew.  Other functions will just ignore such inodes,
 *			if appropriate.  I_NEW is used for waiting.
 *
 * I_SYNC		Synchonized write of dirty inode data.  The bits is
 *			set during data writeback, and cleared with a wakeup
 *			on the bit address once it is done.
 */
#define I_DIRTY_SYNC		(1 << 0)
#define I_DIRTY_DATASYNC	(1 << 1)
#define I_DIRTY_PAGES		(1 << 2)
#define __I_NEW			3
#define I_NEW			(1 << __I_NEW)
#define I_WILL_FREE		(1 << 4)
#define I_FREEING		(1 << 5)
#define I_CLEAR			(1 << 6)
#define __I_SYNC		7
#define I_SYNC			(1 << __I_SYNC)
#define I_REFERENCED		(1 << 8)

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)

struct qstr { // include/linux/dcache.h
  unsigned int hash;
  unsigned int len;
  const unsigned char *name;
};

struct writeback_control {
  // only for interface compatibility 
};

struct vfsmount {
  // Only for interface compatibility
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

struct super_block;

struct file_system_type {
	const char *name;
	int fs_flags;
	struct dentry *(*mount) (struct file_system_type *, int,
                           const char *, void *);
	void (*kill_sb) (struct super_block *);
	//  struct module *owner;
	//  struct file_system_type * next;
	//  struct list_head fs_supers;
  
  //	struct lock_class_key s_lock_key;
  //	struct lock_class_key s_umount_key;
  //	struct lock_class_key s_vfs_rename_key;
  //  
  //	struct lock_class_key i_lock_key;
  //	struct lock_class_key i_mutex_key;
  //	struct lock_class_key i_mutex_dir_key;
  //	struct lock_class_key i_alloc_sem_key;
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
  //    struct rcu_head	i_rcu;
  //  };
  struct list_head	i_dentry; // replaces the above
  
	unsigned long		i_ino;
  atomic_t        i_count;
	unsigned int		i_nlink;
	dev_t           i_rdev;
	unsigned int		i_blkbits;
	u64             i_version;
	loff_t          i_size;
  // #ifdef __NEED_I_SIZE_ORDERED
  // 	seqcount_t		i_size_seqcount;
  // #endif
	struct timespec		i_atime;
	struct timespec		i_mtime;
	struct timespec		i_ctime;
	blkcnt_t          i_blocks;
	unsigned short    i_bytes;
  // struct rw_semaphore	i_alloc_sem;
	const struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
	//  struct file_lock	*i_flock;
  //  struct address_space	*i_mapping;
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
	/* RCU lookup touched fields */
	unsigned int d_flags;		/* protected by d_lock */
	// seqcount_t d_seq;		/* per dentry seqlock */
	// struct hlist_bl_node d_hash;	/* lookup hash list */
	struct dentry *d_parent;	/* parent directory */
	struct qstr d_name;
	struct inode *d_inode;		/* Where the name belongs to - NULL is
                             * negative */
  
  // Cinquain: just use in-memory inode names
	// unsigned char d_iname[DNAME_INLINE_LEN];	/* small names */
  
	/* Ref lookup also touches following */
	unsigned int d_count;		/* protected by d_lock */
	spinlock_t d_lock;		/* per dentry lock */
  //	const struct dentry_operations *d_op;
	struct super_block *d_sb;	/* The root of the dentry tree */
	unsigned long d_time;		/* used by d_revalidate */
	void *d_fsdata;			/* fs-specific data */
  
	// struct list_head d_lru;		/* LRU list */
	/*
	 * d_child and d_rcu can share memory
	 */
  union {
    struct list_head d_child;	/* child of parent list */
  	// struct rcu_head d_rcu;
  } d_u;
  struct list_head d_subdirs;	/* our children */
  struct list_head d_alias;	/* inode alias list */
};

struct path {
  struct dentry *dentry;
  
  // Omits a vfsmount-related member.
  // User-space implementation does not handle vfsmount.
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
  //#ifdef CONFIG_QUOTA
  //	ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
  //	ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
  //#endif
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


// stub.c
/* Stub functions with user-space implementation */
extern struct dentry *d_alloc(struct dentry * parent, const struct qstr *name);

extern void d_instantiate(struct dentry *dentry, struct inode * inode);

extern struct dentry *dget(struct dentry *dentry);

extern struct dentry *d_splice_alias(struct inode *inode,
                                     struct dentry *dentry);
extern int d_invalidate(struct dentry * dentry);

extern void d_genocide(struct dentry *root);

extern unsigned int current_fsuid(void);

extern unsigned int current_fsgid(void);


// vfs.c

extern struct dentry *mount_nodev(struct file_system_type *fs_type,
                                  int flags, void *data,
                                  int (*fill_super)(struct super_block *,
                                                    void *, int));
/**
 *	new_inode 	- obtain an inode
 *	@sb: superblock
 *
 *	Allocates a new inode for given superblock.
 */
extern struct inode *new_inode(struct super_block *sb);

extern void mark_inode_dirty(struct inode *inode);


// include/linux/fs.h
static inline void inc_nlink(struct inode *inode) {
  inode->i_nlink++;
}

static inline void inode_inc_link_count(struct inode *inode) {
  inc_nlink(inode);
  mark_inode_dirty(inode);
}

/**
 * inode_init_owner - Init uid,gid,mode for new inode according to posix standards
 * @inode: New inode
 * @dir: Directory inode
 * @mode: mode of the new inode
 */
static inline void inode_init_owner(struct inode *inode, const struct inode *dir,
                                    mode_t mode) {
	inode->i_uid = current_fsuid();
	if (dir && dir->i_mode & S_ISGID) {
		inode->i_gid = dir->i_gid;
		if (S_ISDIR(mode))
			mode |= S_ISGID;
	} else
		inode->i_gid = current_fsgid();
	inode->i_mode = mode;
}

// fs/inode.c
static inline void destroy_inode(struct inode *inode) {
  // BUG_ON(!list_empty(&inode->i_lru));
  // __destroy_inode(inode);
  if (inode->i_sb->s_op->destroy_inode)
    inode->i_sb->s_op->destroy_inode(inode);
  else
    // call_rcu(&inode->i_rcu, i_callback);
    free(inode);
}

/**
 *      iput    - put an inode
 *      @inode: inode to put
 *
 *      Puts an inode, dropping its usage count. If the inode use count hits
 *      zero, the inode is then freed and may also be destroyed.
*/
static inline void iput(struct inode *inode) {
  if (inode) {
    DEBUG_ON_(inode->i_state & I_CLEAR,
              "[Bug@iput] violating kernel specification.\n");
    // if (atomic_dec_and_lock(&inode->i_count, &inode->i_lock))
    if (atomic_dec_and_test(&inode->i_count)) {
      // spin_lock(&inode->i_lock);
      // iput_final(inode);
      destroy_inode(inode);
    }
  }
}
  
#endif // __KERNEL__
#endif // CINQUAIN_META_VFS_H_
