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

// Checks wether two fsnodes have direct relation.
// Used to prevent cyclic path in tree.
static inline int fsnode_ancestor_(struct cinq_fsnode *ancestor,
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
  
  DEBUG_ON_(fsnode, "[Warning@fsnode_new] duplicate names: %s\n", name);
  
  fsnode = fsnode_malloc();
  fsnode->fs_id = (unsigned long)fsnode;
  DEBUG_ON_((void *)fsnode->fs_id != fsnode,
           "[Error@cnode_new] conversion fails: fs_id %lx != fsnode %p",
           fsnode->fs_id, fsnode);
  
  strncpy(fsnode->fs_name, name, MAX_NAME_LEN);
  HASH_ADD_BY_STR(fs_member, file_systems, fs_name, fsnode); // to global list
  if (parent) {
    fsnode->fs_parent = parent;
    HASH_ADD_BY_PTR(fs_child, fsnode->fs_parent->fs_children, fs_id, fsnode);
  } else {
    fsnode->fs_parent = fsnode; // denotes root
  }
  fsnode->fs_root = NULL; // filled after registeration
  fsnode->fs_children = NULL; // required by uthash
  return fsnode;
}

void fsnode_free(struct cinq_fsnode *fsnode) {
  d_genocide(fsnode->fs_root);
  if (fsnode->fs_children) {
    DEBUG_("[Warning@fsnode_free] failed to delete fsnode %lx(%s) "
           "who still has children.\n", fsnode->fs_id, fsnode->fs_name);
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
    struct cinq_fsnode *cur, *tmp;
    HASH_ITER(fs_child, fsnode->fs_children, cur, tmp) {
      fsnode_free_all(cur);
    }
  }
  fsnode_free(fsnode);
}

void fsnode_move(struct cinq_fsnode *child,
                 struct cinq_fsnode *new_parent) {
  if (fsnode_ancestor_(child, new_parent)) {
    DEBUG_("[Error@fsnode_change_parent] change fsnode %lx's parent "
           "from %lx to %lx.\n",
           child->fs_id, child->fs_parent->fs_id, new_parent->fs_id);
    return;
  }
  HASH_REMOVE(fs_child, child->fs_parent->fs_children, child);
  child->fs_parent = new_parent; // supposed to be atomic
  HASH_ADD_BY_PTR(fs_child, new_parent->fs_children, fs_id, child);
}

void fsnode_bridge(struct cinq_fsnode *out) {
  DEBUG_ON_(HASH_CNT(fs_child, out->fs_children) > 1,
            "[Warning@fsnode_bridge] crossing out who has more than one child: "
            "%lx(%s).\n", out->fs_id, out->fs_name);
  if (out->fs_children) {
    fsnode_move(out->fs_children, out->fs_parent);
  }
  fsnode_free(out);
}
