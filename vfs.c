/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  vfs.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/19/12.
//

#ifndef __KERNEL__

// This file contains imported functions from Linux VFS.

#include "vfs.h"

// fs/super.c
/**
 *	alloc_super	-	create new superblock
 *	@type:	filesystem type superblock should belong to
 *
 *	Allocates and initializes a new &struct super_block.  alloc_super()
 *	returns a pointer new superblock or %NULL if allocation had failed.
 */
static struct super_block *alloc_super_(struct file_system_type *type)
{
	struct super_block *s = malloc(sizeof(struct super_block));
	static const struct super_operations default_op;
  
	if (s) {
  //		if (security_sb_alloc(s)) {
  //			kfree(s);
  //			s = NULL;
  //			goto out;
  //		}
  //#ifdef CONFIG_SMP
  //		s->s_files = alloc_percpu(struct list_head);
  //		if (!s->s_files) {
  //			security_sb_free(s);
  //			kfree(s);
  //			s = NULL;
  //			goto out;
  //		} else {
  //			int i;
  //      
  //			for_each_possible_cpu(i)
  //      INIT_LIST_HEAD(per_cpu_ptr(s->s_files, i));
  //		}
  //#else
  //		INIT_LIST_HEAD(&s->s_files);
  //#endif
  //		s->s_bdi = &default_backing_dev_info;
  //		INIT_LIST_HEAD(&s->s_instances);
  //		INIT_HLIST_BL_HEAD(&s->s_anon);
  //		INIT_LIST_HEAD(&s->s_inodes);
  //		INIT_LIST_HEAD(&s->s_dentry_lru);
  //		init_rwsem(&s->s_umount);
  //		mutex_init(&s->s_lock);
  //		lockdep_set_class(&s->s_umount, &type->s_umount_key);
  //		/*
  //		 * The locking rules for s_lock are up to the
  //		 * filesystem. For example ext3fs has different
  //		 * lock ordering than usbfs:
  //		 */
  //		lockdep_set_class(&s->s_lock, &type->s_lock_key);
  //		/*
  //		 * sget() can have s_umount recursion.
  //		 *
  //		 * When it cannot find a suitable sb, it allocates a new
  //		 * one (this one), and tries again to find a suitable old
  //		 * one.
  //		 *
  //		 * In case that succeeds, it will acquire the s_umount
  //		 * lock of the old one. Since these are clearly distrinct
  //		 * locks, and this object isn't exposed yet, there's no
  //		 * risk of deadlocks.
  //		 *
  //		 * Annotate this by putting this lock in a different
  //		 * subclass.
  //		 */
  //		down_write_nested(&s->s_umount, SINGLE_DEPTH_NESTING);
		s->s_count = 1;
  //		atomic_set(&s->s_active, 1);
  //		mutex_init(&s->s_vfs_rename_mutex);
  //		lockdep_set_class(&s->s_vfs_rename_mutex, &type->s_vfs_rename_key);
  //		mutex_init(&s->s_dquot.dqio_mutex);
  //		mutex_init(&s->s_dquot.dqonoff_mutex);
  //		init_rwsem(&s->s_dquot.dqptr_sem);
  //		init_waitqueue_head(&s->s_wait_unfrozen);
		s->s_maxbytes = ((1UL<<31) - 1); // MAX_NON_LFS;
		s->s_op = &default_op;
		s->s_time_gran = 1000000000;
	}
  //out:
	return s;
}


struct dentry *mount_nodev(struct file_system_type *fs_type,
                           int flags, void *data,
                           int (*fill_super)(struct super_block *, void *, int))
{
	int error;
	// struct super_block *s = sget(fs_type, NULL, set_anon_super, NULL);
  // expanded as following
  struct super_block *s = alloc_super_(fs_type);
  // set_anon_super(s, data);
  s->s_type = fs_type;
  strncpy(s->s_id, fs_type->name, sizeof(s->s_id));
  //  list_add_tail(&s->s_list, &super_blocks);
  //  list_add(&s->s_instances, &type->fs_supers);
  
	if (IS_ERR(s))
		return (void *)s;
  
	s->s_flags = flags;
  
	error = fill_super(s, data, flags); // & MS_SILENT ? 1 : 0);
	if (error) {
  //		deactivate_locked_super(s);
		return ERR_PTR(error);
	}
	s->s_flags |= MS_ACTIVE;
	return dget(s->s_root);
}


// fs/inode.c
/**
 * inode_init_always - perform inode structure intialisation
 * @sb: superblock inode belongs to
 * @inode: inode to initialise
 *
 * These are initializations that need to be done on every inode
 * allocation as the fields are not initialised by slab allocation.
 */
static inline int inode_init_always_(struct super_block *sb, struct inode *inode)
{
  static const struct inode_operations empty_iops;
  static const struct file_operations empty_fops;
  // struct address_space *const mapping = &inode->i_data;
  
  inode->i_sb = sb;
  inode->i_blkbits = sb->s_blocksize_bits;
  inode->i_flags = 0;
  atomic_set(&inode->i_count, 1);
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
  inode->i_rdev = 0;
  inode->dirtied_when = 0;
  
  // if (security_inode_alloc(inode))
  //   goto out;
  spin_lock_init(&inode->i_lock);
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
  //   bdi = sb->s_bdev->bd_inode->i_mapping->backing_dev_info;
  //   mapping->backing_dev_info = bdi;
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
  //out:
  //  return -ENOMEM;
}


// fs/inode.c
static inline struct inode *alloc_inode_(struct super_block *sb)
{
	struct inode *inode;
  
	if (sb->s_op->alloc_inode)
		inode = sb->s_op->alloc_inode(sb);
	else
		inode = (struct inode *)malloc(sizeof(struct inode));
  
	if (!inode)
		return NULL;
  
  // inode_init_once // expanded as following
  memset(inode, 0, sizeof(*inode));
  //  INIT_HLIST_NODE(&inode->i_hash);
  INIT_LIST_HEAD(&inode->i_dentry);
  //  INIT_LIST_HEAD(&inode->i_devices);
  //  INIT_LIST_HEAD(&inode->i_wb_list);
  //  INIT_LIST_HEAD(&inode->i_lru);
  //  address_space_init_once(&inode->i_data);
  //  i_size_ordered_init(inode);
  
	if (inode_init_always_(sb, inode)) {
		inode->i_sb->s_op->destroy_inode(inode); // adjusted
		return NULL;
	}
	return inode;
}

/** fs/inode.c
 *	new_inode 	- obtain an inode
 *	@sb: superblock
 *
 *	Allocates a new inode for given superblock.
 */
struct inode *new_inode(struct super_block *sb) {
	struct inode *inode;
  
	// spin_lock_prefetch(&inode_sb_list_lock);
  
	inode = alloc_inode_(sb);
	if (inode) {
		spin_lock(&inode->i_lock);
		inode->i_state = 0;
		spin_unlock(&inode->i_lock);
		// inode_sb_list_add(inode);
	}
	return inode;
}

/*
 * A helper for ->readlink().  This should be used *ONLY* for symlinks that
 * have ->follow_link() touching nd only in nd_set_link().  Using (or not
 * using) it for any given inode is up to filesystem.
 */
int generic_readlink(struct dentry *dentry, char *buffer, int buflen)
{
  struct nameidata nd;
  void *cookie;
  int res;

  nd.depth = 0;
  cookie = dentry->d_inode->i_op->follow_link(dentry, &nd);
  if (IS_ERR(cookie))
    return PTR_ERR(cookie);

  res = vfs_readlink_(dentry, buffer, buflen, nd_get_link(&nd));
  if (dentry->d_inode->i_op->put_link)
    dentry->d_inode->i_op->put_link(dentry, &nd, cookie);
  return res;
}

/*
 * get_write_access() gets write permission for a file.
 * put_write_access() releases this write permission.
 * This is used for regular files.
 * We cannot support write (and maybe mmap read-write shared) accesses and
 * MAP_DENYWRITE mmappings simultaneously. The i_writecount field of an inode
 * can have the following values:
 * 0: no writers, no VM_DENYWRITE mappings
 * < 0: (-i_writecount) vm_area_structs with VM_DENYWRITE set exist
 * > 0: (i_writecount) users are writing to the file.
 *
 * Normally we operate on that counter with atomic_{inc,dec} and it's safe
 * except for the cases where we don't hold i_writecount yet. Then we need to
 * use {get,deny}_write_access() - these functions check the sign and refuse
 * to do the change if sign is wrong. Exclusion between them is provided by
 * the inode->i_lock spinlock.
 */

int get_write_access(struct inode * inode)
{
	spin_lock(&inode->i_lock);
	if (atomic_read(&inode->i_writecount) < 0) {
		spin_unlock(&inode->i_lock);
		return -ETXTBSY;
	}
	atomic_inc(&inode->i_writecount);
	spin_unlock(&inode->i_lock);
  
	return 0;
}

int deny_write_access(struct file * file)
{
	struct inode *inode = file->f_path.dentry->d_inode;
  
	spin_lock(&inode->i_lock);
	if (atomic_read(&inode->i_writecount) > 0) {
		spin_unlock(&inode->i_lock);
		return -ETXTBSY;
	}
	atomic_dec(&inode->i_writecount);
	spin_unlock(&inode->i_lock);
  
	return 0;
}


static struct file *__dentry_open(struct dentry *dentry, struct vfsmount *mnt,
                                  struct file *f,
                                  int (*open)(struct inode *, struct file *),
                                  const struct cred *cred)
{
	static const struct file_operations empty_fops = {};
	struct inode *inode;
	int error;
  
	f->f_mode = OPEN_FMODE(f->f_flags) | FMODE_LSEEK |
      FMODE_PREAD | FMODE_PWRITE;
  
	if (unlikely(f->f_flags & O_PATH))
		f->f_mode = FMODE_PATH;
  
	inode = dentry->d_inode;
	if (f->f_mode & FMODE_WRITE) {
    //		error = __get_file_write_access(inode, mnt); // simplified as below
    error = get_write_access(inode);
		if (error)
			goto cleanup_file;
//		if (!special_file(inode->i_mode))
//			file_take_write(f);
	}
  
//	f->f_mapping = inode->i_mapping;
	f->f_path.dentry = dentry;
//	f->f_path.mnt = mnt;
	f->f_pos = 0;

//	file_sb_list_add(f, inode->i_sb);
  
	if (unlikely(f->f_mode & FMODE_PATH)) {
		f->f_op = &empty_fops;
		return f;
	}
  
	f->f_op = inode->i_fop; // fops_get(inode->i_fop);
  
//	error = security_dentry_open(f, cred);
//	if (error)
//		goto cleanup_all;
  
	if (!open && f->f_op)
		open = f->f_op->open;
	if (open) {
		error = open(inode, f);
		if (error)
			goto cleanup_all;
	}
//	if ((f->f_mode & (FMODE_READ | FMODE_WRITE)) == FMODE_READ)
//		i_readcount_inc(inode); // only ifdef CONFIG_IMA
  
	f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);
  
//	file_ra_state_init(&f->f_ra, f->f_mapping->host->i_mapping);
  
	/* NB: we're sure to have correct a_ops only after f_op->open */
//	if (f->f_flags & O_DIRECT) {
//		if (!f->f_mapping->a_ops ||
//		    ((!f->f_mapping->a_ops->direct_IO) &&
//         (!f->f_mapping->a_ops->get_xip_mem))) {
//          fput(f);
//          f = ERR_PTR(-EINVAL);
//        }
//	}
  
	return f;
  
cleanup_all:
  //	fops_put(f->f_op); // simplified as below
  
	if (f->f_mode & FMODE_WRITE) {
		put_write_access(inode);
//		if (!special_file(inode->i_mode)) {
//			file_reset_write(f);
//			mnt_drop_write(mnt);
//		}
	}
  
//	file_sb_list_del(f);
	f->f_path.dentry = NULL;
//	f->f_path.mnt = NULL;
cleanup_file:
	put_filp(f);
	dput(dentry);
//	mntput(mnt);
	return ERR_PTR(error);
}

/*
 * dentry_open() will have done dput(dentry) and mntput(mnt) if it returns an
 * error.
 */
struct file *dentry_open(struct dentry *dentry, struct vfsmount *mnt, int flags,
                         const struct cred *cred)
{
	int error;
	struct file *f;
  
//	validate_creds(cred);
//  BUG_ON(!mnt);
  
	error = -ENFILE;
	f = get_empty_filp();
	if (f == NULL) {
		dput(dentry);
//		mntput(mnt);
		return ERR_PTR(error);
	}
  
	f->f_flags = flags;
	return __dentry_open(dentry, mnt, f, NULL, cred);
}


static int __negative_fpos_check(struct file *file, loff_t pos, size_t count)
{
	/*
	 * pos or pos+count is negative here, check overflow.
	 * too big "count" will be caught in rw_verify_area().
	 */
	if ((pos < 0) && (pos + count < pos))
		return -EOVERFLOW;
	if (file->f_mode & FMODE_UNSIGNED_OFFSET)
		return 0;
	return -EINVAL;
}

/**
 * generic_file_llseek_unlocked - lockless generic llseek implementation
 * @file:	file structure to seek on
 * @offset:	file offset to seek to
 * @origin:	type of seek
 *
 * Updates the file offset to the value specified by @offset and @origin.
 * Locking must be provided by the caller.
 */
static inline loff_t generic_file_llseek_unlocked(struct file *file,
                                                  loff_t offset, int origin) {
	struct inode *inode = file->f_dentry->d_inode; // file->f_mapping->host;
  
	switch (origin) {
    case SEEK_END:
      offset += inode->i_size;
      break;
    case SEEK_CUR:
      /*
       * Here we special-case the lseek(fd, 0, SEEK_CUR)
       * position-querying operation.  Avoid rewriting the "same"
       * f_pos value back to the file because a concurrent read(),
       * write() or lseek() might have altered it
       */
      if (offset == 0)
        return file->f_pos;
      offset += file->f_pos;
      break;
	}
  
	if (offset < 0 && __negative_fpos_check(file, offset, 0))
		return -EINVAL;
	if (offset > inode->i_sb->s_maxbytes)
		return -EINVAL;
  
	/* Special lock needed here? */
	if (offset != file->f_pos) {
		file->f_pos = offset;
		file->f_version = 0;
	}
  
	return offset;
}

/**
 * generic_file_llseek - generic llseek implementation for regular files
 * @file:	file structure to seek on
 * @offset:	file offset to seek to
 * @origin:	type of seek
 *
 * This is a generic implemenation of ->llseek useable for all normal local
 * filesystems.  It just updates the file offset to the value specified by
 * @offset and @origin under i_mutex.
 */
loff_t generic_file_llseek(struct file *file, loff_t offset, int origin)
{
	loff_t rval;
  
	mutex_lock(&file->f_dentry->d_inode->i_mutex);
	rval = generic_file_llseek_unlocked(file, offset, origin);
	mutex_unlock(&file->f_dentry->d_inode->i_mutex);
  
	return rval;
}

static inline void generic_fillattr(struct inode *inode, struct kstat *stat)
{
	stat->dev = inode->i_sb->s_dev;
	stat->ino = inode->i_ino;
	stat->mode = inode->i_mode;
	stat->nlink = inode->i_nlink;
	stat->uid = inode->i_uid;
	stat->gid = inode->i_gid;
	stat->rdev = inode->i_rdev;
	stat->atime = inode->i_atime;
	stat->mtime = inode->i_mtime;
	stat->ctime = inode->i_ctime;
	stat->size = inode->i_size; // i_size_read(inode);
	stat->blocks = inode->i_blocks;
	stat->blksize = (1 << inode->i_blkbits);
}

int simple_getattr(struct vfsmount *mnt, struct dentry *dentry,
                   struct kstat *stat) {
  struct inode *inode = dentry->d_inode;
  generic_fillattr(inode, stat);
//  stat->blocks = inode->i_mapping->nrpages << (PAGE_CACHE_SHIFT - 9);
  return 0;
}

#endif // __KERNEL__
