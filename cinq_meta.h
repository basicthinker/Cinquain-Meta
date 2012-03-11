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
#include "uthash.h"

#define FILE_HASH_WIDTH 16 // bytes
#define MAX_FILE_NAME_LEN 256 // max value
#define MAX_NESTED_LINKS 6

#ifndef __KERNEL__
// supplement of kernel structure to user space

struct dentry {
  inode_t *d_inode;
  struct dentry *parent;
  struct qstr d_name;
  
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
  struct cinq_fsnode *parent;
  
  // Using hash table to store children
  struct cinq_fsnode *children;
  UT_hash_handle hh;
};

static inline int fsnode_is_root(struct cinq_fsnode *fsnode) {
  return fsnode->parent == fsnode;
}

/* fsnode.c */
// Note that these operations are NOT thread safe.
// However, the parent can be safely read from multiple threads.
//
// This implies that only a single thread should be used to manipulate fsnodes,
// while multiple threads are allowed to retrieve the parent of a fsnode.

// Creates a fsnode.
// @parent the fsnode's parent, while NULL indicates a root fsnode.
extern struct cinq_fsnode *fsnode_new(struct cinq_fsnode *parent);

// Destroys a single fsnode without children
extern void fsnode_free(struct cinq_fsnode *fsnode);

// Destroys recursively a single fsnode and all its descendants
extern void fsnode_free_all(struct cinq_fsnode *fsnode);

extern void fsnode_move(struct cinq_fsnode *child,
                        struct cinq_fsnode *new_parent);


struct __cinq_tag {
  unsigned int fs_id;
  UT_hash_handle hh;
};

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
  char file_name[MAX_FILE_NAME_LEN];
  char file_handle[FILE_HASH_WIDTH];
  struct __cinq_tag *tags; // hash table of tags
  struct cinq_inode *children; // hash table of children
  
};

/* inode.c */
extern struct dentry *cinq_lookup(inode_t *dir, struct dentry *dentry,
                                  struct nameidata *nameidata);


#endif // CINQUAIN_META_CINQ_META_H_
