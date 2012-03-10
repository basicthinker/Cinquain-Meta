/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  fsnode.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/10/12.
//

#include "cinq_meta.h"

static unsigned int fs_id_count = 0;

// Checks wether two fsnodes have direct relation.
// Used to prevent cyclic path in tree.
static inline int __fsnode_ancestor(struct cinq_fsnode *ancestor,
                                  struct cinq_fsnode *descendant) {
  while (!fsnode_is_root(descendant)) {
    descendant = descendant->parent;
    if (descendant == ancestor) return 1;
  }
  return 0;
}

struct cinq_fsnode *fsnode_alloc(struct cinq_fsnode *parent) {
  struct cinq_fsnode *fsnode = fsnode_malloc();
  fsnode->fs_id = fs_id_count++;
  if (parent) {
    fsnode->parent = parent;
    HASH_ADD_INT(fsnode->parent->children, fs_id, fsnode);
  } else {
    fsnode->parent = fsnode; // denotes root
  }
  fsnode->children = NULL; // required by uthash
  return fsnode;
}

void fsnode_free(struct cinq_fsnode *fsnode) {
  if (HASH_COUNT(fsnode->children)) {
    log("[Error: fsnode_free] failed to delete fsnode %u "
        "who still has children.\n",
        fsnode->fs_id);
    return;
  }
  if (!fsnode_is_root(fsnode) && fsnode->parent) {
    HASH_DEL(fsnode->parent->children, fsnode);
  }
  fsnode_mfree(fsnode);
}

void fsnode_free_all(struct cinq_fsnode *fsnode) {
  if (fsnode->children) {
    struct cinq_fsnode *current, *tmp;
    HASH_ITER(hh, fsnode->children, current, tmp) {
      fsnode_free_all(current);
    }
  }
  fsnode_free(fsnode);
}

void fsnode_move(struct cinq_fsnode *child,
                          struct cinq_fsnode *new_parent) {
  if (__fsnode_ancestor(child, new_parent)) {
    log("[Error: fsnode_change_parent] change fsnode %u's parent "
        "from %u to %u.\n",
        child->fs_id, child->parent->fs_id, new_parent->fs_id);
    return;
  }
  HASH_DEL(child->parent->children, child);
  child->parent = new_parent; // supposed to be atomic
  HASH_ADD_INT(new_parent->children, fs_id, child);
}
