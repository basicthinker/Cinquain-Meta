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
	// return dget(s->s_root); // expanded
  spin_lock(&s->s_root->d_lock);
  s->s_root->d_count++; // to prevent being destroyed
  spin_unlock(&s->s_root->d_lock);
  return s->s_root;
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
		inode = sb->s_op->alloc_inode(sb); // no specific alloc function
	else
		inode = (struct inode *)malloc(sizeof(struct inode));
  
	if (!inode)
		return NULL;
  
#ifdef CINQ_DEBUG
  atomic_inc(&num_inodes_);
#endif // CINQ_DEBUG
  
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
		if (inode->i_sb->s_op->destroy_inode)
			inode->i_sb->s_op->destroy_inode(inode);
		else
			free(inode); // adjusted for user space

#ifdef CINQ_DEBUG
    atomic_dec(&num_inodes_);
#endif // CINQ_DEBUG
    
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

#endif // __KERNEL__
