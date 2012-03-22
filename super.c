//
//  super.c
//  cinquain-meta
//
//  Created by Jinglei Ren on 3/22/12.
//  Copyright (c) 2012 Tsinghua University. All rights reserved.
//

#include "cinq_meta.h"

static int cinq_fill_super_(struct super_block *sb, void *data, int silent) {
  struct inode *inode = NULL;
	struct dentry *root;
	int err;
  
	save_mount_options(sb, data);
  
	err = ramfs_parse_options(data, &fsi->mount_opts);
	if (err)
		goto fail;
  
	sb->s_maxbytes		= MAX_LFS_FILESIZE;
  //	sb->s_blocksize		= PAGE_CACHE_SIZE;
  //	sb->s_blocksize_bits	= PAGE_CACHE_SHIFT;
	sb->s_magic	= RAMFS_MAGIC;
	sb->s_op = &cinq_super_operations;
	sb->s_time_gran	= 1;
  
	inode = cinq_get_inode(sb, NULL, S_IFDIR, 0);
	if (!inode) {
		err = -ENOMEM;
		goto fail;
	}
  
	root = d_alloc_root(inode);
	sb->s_root = root;
	if (!root) {
		err = -ENOMEM;
		goto fail;
	}
  
	return 0;
}

struct dentry *cinq_mount (struct file_system_type *fs_type, int flags,
                           const char *dev_name, void *data) {
  return mount_nodev(fs_type, flags, data, cinq_fill_super_);
}

void cinq_kill_sb (struct super_block *sb) {
  
}

void cinq_dirty_inode(struct inode *inode) {
  
}

int cinq_write_inode(struct inode *inode, struct writeback_control *wbc) {
  return 0;
}
