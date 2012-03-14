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
#include "stub.h"

/* Cinquain File System Data Structures and Operations */

struct cinq_fsnode {
  unsigned long fs_id;
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

enum cinq_inherit_type {
  CINQ_OVERWR = 1,
  CINQ_MERGE = 2
};
#define CINQ_MODE_SHIFT 30
#define I_MODE_FILTER 01111111111

struct cinq_tag {
  struct cinq_fsnode *t_fs; // key for hh
  struct inode *t_inode;

  enum cinq_inherit_type t_mode;
  char in_file_handle[FILE_HASH_WIDTH];

  UT_hash_handle hh; // default handle name
};

struct cinq_inode {
  unsigned long ci_id;
  char ci_name[MAX_NAME_LEN + 1];

  struct cinq_tag *ci_tags; // hash table of tags
  rwlock_t ci_tags_lock;
  
  struct cinq_inode *ci_parent;
  struct cinq_inode *ci_children; // hash table of children
  UT_hash_handle ci_child;
  rwlock_t ci_children_lock;
};

// No inode cache is necessary since cinq_inodes are in memory.
// Therefore no public alloc/free-like functions are provided.
/* cnode.c */
extern struct dentry *cinq_lookup(struct inode *dir, struct dentry *dentry,
                                  struct nameidata *nameidata);
extern int cinq_mkdir(struct inode *dir, struct dentry *dentry, int mode);

#endif // CINQUAIN_META_CINQ_META_H_
