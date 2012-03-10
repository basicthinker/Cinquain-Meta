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

// News a fsnode.
// @parent the fsnode's parent, while NULL indicates a root fsnode.
extern struct cinq_fsnode *fsnode_alloc(struct cinq_fsnode *parent);

// Destroys a single fsnode without children
extern void fsnode_free(struct cinq_fsnode *fsnode);

// Destroys recursively a single fsnode and all its descendants
extern void fsnode_free_all(struct cinq_fsnode *fsnode);

extern void fsnode_move(struct cinq_fsnode *child,
                        struct cinq_fsnode *new_parent);

#endif // CINQUAIN_META_CINQ_META_H_
