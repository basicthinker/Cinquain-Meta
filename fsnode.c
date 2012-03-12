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

struct cinq_fsnode *file_systems = NULL;
static unsigned int fs_id_count = 0;

// Checks wether two fsnodes have direct relation.
// Used to prevent cyclic path in tree.
static inline int __fsnode_ancestor(struct cinq_fsnode *ancestor,
                                  struct cinq_fsnode *descendant) {
  while (!fsnode_is_root(descendant)) {
    descendant = descendant->fs_parent;
    if (descendant == ancestor) return 1;
  }
  return 0;
}

struct cinq_fsnode *fsnode_new(const char *name, struct cinq_fsnode *parent) {
  struct cinq_fsnode *fsnode;
  HASH_FIND_BY_STR(fs_member, file_systems, name, fsnode);
  if (fsnode) {
    log("[Warning@fsnode_new] duplicate names: %s\n", name);
  }
  fsnode = fsnode_malloc();
  fsnode->fs_id = fs_id_count++;
  strncpy(fsnode->fs_name, name, MAX_NAME_LEN);
  HASH_ADD_BY_STR(fs_member, file_systems, fs_name, fsnode); // to global list
  if (parent) {
    fsnode->fs_parent = parent;
    HASH_ADD_BY_INT(fs_child, fsnode->fs_parent->fs_children, fs_id, fsnode);
  } else {
    fsnode->fs_parent = fsnode; // denotes root
  }
  fsnode->fs_children = NULL; // required by uthash
  return fsnode;
}

void fsnode_free(struct cinq_fsnode *fsnode) {
  if (fsnode->fs_children) {
    log("[Error@fsnode_free] failed to delete fsnode %u "
        "who still has children.\n",
        fsnode->fs_id);
    return;
  }
  if (!fsnode_is_root(fsnode) && fsnode->fs_parent) {
    HASH_REMOVE(fs_child, fsnode->fs_parent->fs_children, fsnode);
  }
  HASH_REMOVE(fs_member, file_systems, fsnode);
  fsnode_mfree(fsnode);
}

void fsnode_free_all(struct cinq_fsnode *fsnode) {
  if (fsnode->fs_children) {
    struct cinq_fsnode *current, *tmp;
    HASH_ITER(fs_child, fsnode->fs_children, current, tmp) {
      fsnode_free_all(current);
    }
  }
  fsnode_free(fsnode);
}

void fsnode_move(struct cinq_fsnode *child,
                          struct cinq_fsnode *new_parent) {
  if (__fsnode_ancestor(child, new_parent)) {
    log("[Error@fsnode_change_parent] change fsnode %u's parent "
        "from %u to %u.\n",
        child->fs_id, child->fs_parent->fs_id, new_parent->fs_id);
    return;
  }
  HASH_REMOVE(fs_child, child->fs_parent->fs_children, child);
  child->fs_parent = new_parent; // supposed to be atomic
  HASH_ADD_BY_INT(fs_child, new_parent->fs_children, fs_id, child);
}
