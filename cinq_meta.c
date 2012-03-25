/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  cinq_meta.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/18/12.
//

#include "cinq_meta.h"

const struct file_system_type cinqfs = {
  .name = "cinqfs",
#ifdef __KERNEL__
  .owner = THIS_MODULE,
#endif
  .mount = cinq_mount,
  .kill_sb = cinq_kill_sb
};

const struct super_operations cinq_super_operations = {
  .dirty_inode = cinq_dirty_inode,
  .write_inode = cinq_write_inode
};

const struct inode_operations cinq_dir_inode_operations = {
	.create		= cinq_create,
	.lookup		= cinq_lookup,
	.link     = cinq_link,
	.unlink		= cinq_unlink,
	.symlink	= cinq_symlink,
	.mkdir		= cinq_mkdir,
	.rmdir		= cinq_rmdir,
	.rename		= cinq_rename,
	.setattr	= cinq_setattr
};

const struct inode_operations cinq_file_inode_operations = {
	.setattr	= cinq_setattr
};

const struct file_operations cinq_file_operations = {
	.read     = cinq_read,
	.write		= cinq_write,
	.open     = cinq_open,
	.release	= cinq_release_file
};
