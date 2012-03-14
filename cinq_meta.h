/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  cinq_meta.h
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/9/12.
//

#ifndef CINQUAIN_META_CINQ_META_H_
#define CINQUAIN_META_CINQ_META_H_

#include "util.h"

#ifndef __KERNEL__
// supplement of kernel structure to user space

struct super_block {
  void *s_fs_info;
  
  // Omits all other members.
  // This is only for interface compatibility.
};

struct dentry {
  inode_t *d_inode;
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
  inode_t *inode; /* path.dentry.d_inode */
  unsigned int flags;
  unsigned int seq;
  int last_type;
  unsigned depth;
  char *saved_names[MAX_NESTED_LINKS + 1];

  // Omits intent data
};

#endif // __KERNEL__


struct cinq_fsnode {
  unsigned int fs_id;
  char fs_name[MAX_NAME_LEN + 1];
  struct cinq_fsnode *fs_parent;
  
  // Using hash table to store children
  struct cinq_fsnode *fs_children;
  UT_hash_handle fs_child; // used for parent's children
  UT_hash_handle fs_tag; // used for cinq_inode's tags
  UT_hash_handle fs_member; // used for global file system list
};

static inline int fsnode_is_root(struct cinq_fsnode *fsnode) {
  return fsnode->fs_parent == fsnode;
}

/* fsnode.c */
// Note that these operations are NOT thread safe.
// However, the parent can be safely read from multiple threads.
//
// This implies that only a single thread should be used to manipulate fsnodes,
// while multiple threads are allowed to retrieve the parent of a fsnode.

// Creates a fsnode.
// @parent the fsnode's parent, while NULL indicates a root fsnode.
extern struct cinq_fsnode *fsnode_new(const char *name,
                                      struct cinq_fsnode *parent);

// Destroys a single fsnode without children
extern void fsnode_free(struct cinq_fsnode *fsnode);

// Destroys recursively a single fsnode and all its descendants
extern void fsnode_free_all(struct cinq_fsnode *fsnode);

extern void fsnode_move(struct cinq_fsnode *child,
                        struct cinq_fsnode *new_parent);


struct cinq_inode {
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
  
  // Cinquain-specific
  char in_file_name[MAX_NAME_LEN + 1];
  char in_file_handle[FILE_HASH_WIDTH];
  
  struct cinq_fsnode *in_tags; // hash table of tags
  rwlock_t in_tags_lock;
  
  struct cinq_inode *in_parent;
  struct cinq_inode *in_children; // hash table of children
  rwlock_t in_children_lock;
  UT_hash_handle in_child;
};

// No inode cache is necessary since cinq_inodes are in memory.
// Therefore no public alloc/free-like functions are provided.
/* inode.c */
extern struct dentry *cinq_lookup(inode_t *dir, struct dentry *dentry,
                                  struct nameidata *nameidata);
extern int cinq_mkdir(inode_t *dir, struct dentry *dentry, int mode);

#endif // CINQUAIN_META_CINQ_META_H_
