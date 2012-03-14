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

struct super_block {
  void *s_fs_info;
  
  // Omits all other members.
  // This is only for interface compatibility.
};

struct inode {
  umode_t i_mode;
  uid_t i_uid; 
  gid_t i_gid;
  unsigned int i_flags;
  
  /* Stat data, not accessed from path walking */
  unsigned long i_no;
  unsigned int i_nlink;
  
  long i_atime;
  long i_mtime;
  long i_ctime;
  uint32_t i_dtime;
  loff_t i_size;
  
  uint32_t i_file_acl;
  uint32_t i_dir_acl;
  __u32 i_generation;
  
  // Omits other members.
};

struct dentry {
  struct inode *d_inode;
  struct dentry *parent;
  struct qstr d_name;
  void *d_fsdata;
  
  // Omits all dcache-related members.
  // User-space implementation should provide independent dentry cache.
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


/**
 * d_add - add dentry to hash queues
 * @entry: dentry to add
 * @inode: the inode to attach to this dentry
 *
 * This adds the entry to the hash queues and initializes @inode.
 * The entry was actually filled in earlier during d_alloc().
*/
static inline void d_add(struct dentry *entry, struct inode *inode) {
  // User-space implementation
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
  dentry->d_inode = inode;
  return dentry;
}

#endif // __KERNEL__

#endif // CINQUAIN_META_STUB_H_
