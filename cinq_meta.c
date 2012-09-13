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
#include "util.h"

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/init.h>
#endif

struct file_system_type cinqfs = {
  .name = "cinqfs",
#ifdef __KERNEL__
  .owner = THIS_MODULE,
#endif
  .mount = cinq_mount,
  .kill_sb = cinq_kill_sb
};

const struct super_operations cinq_super_operations = {
  .alloc_inode = cinq_alloc_inode,
  .destroy_inode = cinq_destroy_inode,
  .evict_inode = cinq_evict_inode,
#ifdef __KERNEL__
  .statfs = simple_statfs,
  .show_options = generic_show_options,
#endif
};

const struct inode_operations cinq_dir_inode_operations = {
	.create		= cinq_create,
	.lookup		= cinq_lookup,
	.link   	= cinq_link,
	.unlink		= cinq_unlink,
	.symlink	= cinq_symlink,
	.mkdir		= cinq_mkdir,
	.mknod    	= cinq_mknod,
	.rmdir		= cinq_rmdir,
	.rename		= cinq_rename,
	.setattr	= cinq_setattr
};

const struct inode_operations cinq_file_inode_operations = {
	.setattr	= cinq_setattr,
  .getattr  = simple_getattr
};

const struct inode_operations cinq_symlink_inode_operations = {
  .readlink       = generic_readlink,
  .follow_link    = cinq_follow_link,
  .setattr        = cinq_setattr
};

const struct file_operations cinq_file_operations = {
	.read     = cinq_file_read,
	.write		= cinq_file_write,
  .fsync    = noop_fsync,
  .llseek   = generic_file_llseek
};

// Refers to fs/libfs.c. Also used by ramfs.
const struct file_operations cinq_dir_operations = {
  .open     = cinq_dir_open,
  .release  = cinq_dir_release,
  //  .llseek   = cinq_dir_llseek,
  .readdir  = cinq_readdir,
  
  .read     = generic_read_dir,
  .fsync    = noop_fsync
};

const struct export_operations cinq_export_operations = {
#ifdef __KERNEL__
	.get_parent     = cinq_get_parent,
	.encode_fh      = cinq_encode_fh,
	.fh_to_dentry   = cinq_fh_to_dentry,
#endif
};

#ifdef __KERNEL__

struct kmem_cache *UT_hash_table_cachep;
struct kmem_cache *cinq_jentry_cachep;

static int __init init_cinq_fs(void) {
  int err;
  
  err = bdi_init(&cinq_backing_dev_info);
	if (err) goto destroy_bdi;

  err = init_cnode_cache();
  if (err) goto free_cnode;

  err = init_tag_cache();
  if (err) goto free_tag;

  err = init_inode_cache();
  if (err) goto free_inode;

  err = init_fsnode_cache();
  if (err) goto free_fsnode;

  err = init_jentry_cache();
  if (err) goto free_jentry;

  err = init_UT_hash_table_cache();
  if (err) goto free_hash;

  err = register_filesystem(&cinqfs);
  if (err) goto unregister;

  DEBUG_("sinqfs: loaded successfully.");
  return 0;

free_hash:
  destroy_UT_hash_table_cache();
free_jentry:
  destroy_jentry_cache();
free_fsnode:
  destroy_fsnode_cache();
free_inode:
  destroy_inode_cache();
free_tag:
  destroy_tag_cache();
free_cnode:
  destroy_cnode_cache();
unregister:
  unregister_filesystem(&cinqfs);
destroy_bdi:
  bdi_destroy(&cinq_backing_dev_info);
  return err;
}

static void __exit exit_cinq_fs(void) {
  bdi_destroy(&cinq_backing_dev_info);
  destroy_cnode_cache();
  destroy_fsnode_cache();
  destroy_jentry_cache();
  destroy_UT_hash_table_cache();
  unregister_filesystem(&cinqfs);
}

module_init(init_cinq_fs);
module_exit(exit_cinq_fs);
MODULE_LICENSE("GPL");

#endif

