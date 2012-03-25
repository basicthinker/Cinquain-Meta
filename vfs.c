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

#include "util.h"

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
  strlcpy(s->s_id, fs_type->name, sizeof(s->s_id));
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

// fs/inode.c
void destroy_inode(struct inode *inode) {
  // BUG_ON(!list_empty(&inode->i_lru));
  // __destroy_inode(inode);
  if (inode->i_sb->s_op->destroy_inode)
    inode->i_sb->s_op->destroy_inode(inode);
  else
    // call_rcu(&inode->i_rcu, i_callback);
    free(inode);
}


// fs/fs-writeback.c
/**
 *	__mark_inode_dirty -	internal function
 *	@inode: inode to mark
 *	@flags: what kind of dirty (i.e. I_DIRTY_SYNC)
 *	Mark an inode as dirty. Callers should use mark_inode_dirty or
 *  	mark_inode_dirty_sync.
 *
 * Put the inode on the super block's dirty list.
 *
 * CAREFUL! We mark it dirty unconditionally, but move it onto the
 * dirty list only if it is hashed or if it refers to a blockdev.
 * If it was not hashed, it will never be added to the dirty list
 * even if it is later hashed, as it will have been marked dirty already.
 *
 * In short, make sure you hash any inodes _before_ you start marking
 * them dirty.
 *
 * This function *must* be atomic for the I_DIRTY_PAGES case -
 * set_page_dirty() is called under spinlock in several places.
 *
 * Note that for blockdevs, inode->dirtied_when represents the dirtying time of
 * the block-special inode (/dev/hda1) itself.  And the ->dirtied_when field of
 * the kernel-internal blockdev inode represents the dirtying time of the
 * blockdev's pages.  This is why for I_DIRTY_PAGES we always use
 * page->mapping->host, so the page-dirtying time is recorded in the internal
 * blockdev inode.
 */
static inline void mark_inode_dirty_(struct inode *inode, int flags)
{
	struct super_block *sb = inode->i_sb;
  //	struct backing_dev_info *bdi = NULL;
  
	/*
	 * Don't do this for I_DIRTY_PAGES - that doesn't actually
	 * dirty the inode itself
	 */
	if (flags & (I_DIRTY_SYNC | I_DIRTY_DATASYNC)) {
		if (sb->s_op->dirty_inode)
			sb->s_op->dirty_inode(inode);
	}
  
  return; // truncate the following
          //	/*
          //	 * make sure that changes are seen by all cpus before we test i_state
          //	 * -- mikulas
          //	 */
          //	smp_mb();
          //  
          //	/* avoid the locking if we can */
          //	if ((inode->i_state & flags) == flags)
          //		return;
          //  
          //	if (unlikely(block_dump))
          //		block_dump___mark_inode_dirty(inode);
          //  
          //	spin_lock(&inode->i_lock);
          //	if ((inode->i_state & flags) != flags) {
          //		const int was_dirty = inode->i_state & I_DIRTY;
          //    
          //		inode->i_state |= flags;
          //    
          //		/*
          //		 * If the inode is being synced, just update its dirty state.
          //		 * The unlocker will place the inode on the appropriate
          //		 * superblock list, based upon its state.
          //		 */
          //		if (inode->i_state & I_SYNC)
          //			goto out_unlock_inode;
          //    
          //		/*
          //		 * Only add valid (hashed) inodes to the superblock's
          //		 * dirty list.  Add blockdev inodes as well.
          //		 */
          //		if (!S_ISBLK(inode->i_mode)) {
          //			if (inode_unhashed(inode))
          //				goto out_unlock_inode;
          //		}
          //		if (inode->i_state & I_FREEING)
          //			goto out_unlock_inode;
          //    
          //		/*
          //		 * If the inode was already on b_dirty/b_io/b_more_io, don't
          //		 * reposition it (that would break b_dirty time-ordering).
          //		 */
          //		if (!was_dirty) {
          //			bool wakeup_bdi = false;
          //			bdi = inode_to_bdi(inode);
          //      
          //			if (bdi_cap_writeback_dirty(bdi)) {
          //				WARN(!test_bit(BDI_registered, &bdi->state),
          //				     "bdi-%s not registered\n", bdi->name);
          //        
          //				/*
          //				 * If this is the first dirty inode for this
          //				 * bdi, we have to wake-up the corresponding
          //				 * bdi thread to make sure background
          //				 * write-back happens later.
          //				 */
          //				if (!wb_has_dirty_io(&bdi->wb))
          //					wakeup_bdi = true;
          //			}
          //      
          //			spin_unlock(&inode->i_lock);
          //			spin_lock(&inode_wb_list_lock);
          //			inode->dirtied_when = jiffies;
          //			list_move(&inode->i_wb_list, &bdi->wb.b_dirty);
          //			spin_unlock(&inode_wb_list_lock);
          //      
          //			if (wakeup_bdi)
          //				bdi_wakeup_thread_delayed(bdi);
          //			return;
          //		}
          //	}
          //out_unlock_inode:
          //	spin_unlock(&inode->i_lock);
}

void mark_inode_dirty(struct inode *inode) {
  mark_inode_dirty_(inode, I_DIRTY);
}

void inode_inc_link_count(struct inode *inode) {
  inc_nlink(inode);
  mark_inode_dirty(inode);
}

/**
 * inode_init_owner - Init uid,gid,mode for new inode according to posix standards
 * @inode: New inode
 * @dir: Directory inode
 * @mode: mode of the new inode
 */
void inode_init_owner(struct inode *inode, const struct inode *dir,
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

#endif // __KERNEL__
