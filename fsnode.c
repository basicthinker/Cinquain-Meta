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

// Checks wether two fsnodes have direct relation.
// Used to prevent cyclic path in tree.
static inline int fsnode_ancestor_(struct cinq_fsnode *ancestor,
                                  struct cinq_fsnode *descendant) {
  while (descendant != META_FS) {
    descendant = descendant->fs_parent;
    if (descendant == ancestor) return 1;
  }
  return 0;
}

struct cinq_fsnode *fsnode_new(struct cinq_fsnode *parent, const char *name) {
  
  struct cinq_fsnode *fsnode = fsnode_malloc_();
  fsnode->fs_id = (unsigned long)fsnode;
  DEBUG_ON_((void *)fsnode->fs_id != fsnode,
           "[Error@cnode_new] conversion fails: fs_id %lx != fsnode %p",
           fsnode->fs_id, fsnode);
  strncpy(fsnode->fs_name, name, MAX_NAME_LEN);
  fsnode->fs_parent = parent;
  fsnode->fs_root = NULL; // filled after registeration
  fsnode->fs_children = NULL; // required by uthash
  rwlock_init(&fsnode->fs_children_lock);
  
  write_lock(&file_systems.lock);
  struct cinq_fsnode *dup = cfs_find_(&file_systems, name);
  if (unlikely(dup)) {
    DEBUG_("[Warn@fsnode_new] duplicate names: %s\n", name);
    fsnode_free_(fsnode);
    wr_release_return(&file_systems.lock, NULL);
  }
  cfs_add_(&file_systems, fsnode);
  write_unlock(&file_systems.lock);
  
  if (parent != META_FS) {
    write_lock(&parent->fs_children_lock);
    HASH_ADD_BY_PTR(fs_child, parent->fs_children, fs_id, fsnode);
    write_unlock(&parent->fs_children_lock);
  }
  return fsnode;
}

void fsnode_evict(struct cinq_fsnode *fsnode) {
  d_genocide(fsnode->fs_root);
  if (fsnode->fs_children) {
    DEBUG_("[Warn@fsnode_evict] failed to delete fsnode %lx(%s) "
           "who still has children.\n", fsnode->fs_id, fsnode->fs_name);
    return;
  }
  
  cfs_rm_syn(&file_systems, fsnode);
  
  if (fsnode->fs_parent != META_FS) {
    write_lock(&fsnode->fs_parent->fs_children_lock);
    HASH_DELETE(fs_child, fsnode->fs_parent->fs_children, fsnode);
    write_unlock(&fsnode->fs_parent->fs_children_lock);
  }
  fsnode_free_(fsnode);
}

void fsnode_evict_all(struct cinq_fsnode *fsnode) {
  if (fsnode->fs_children) {
    struct cinq_fsnode *cur, *tmp;
    HASH_ITER(fs_child, fsnode->fs_children, cur, tmp) {
      fsnode_evict_all(cur);
    }
  }
  fsnode_evict(fsnode);
}

void fsnode_move(struct cinq_fsnode *child,
                 struct cinq_fsnode *new_parent) {
  if (unlikely(fsnode_ancestor_(child, new_parent))) {
    DEBUG_("[Error@fsnode_change_parent] change fsnode %s's parent to %s.\n",
           child->fs_name, new_parent->fs_name);
    return;
  }
  write_lock(&child->fs_parent->fs_children_lock);
  HASH_DELETE(fs_child, child->fs_parent->fs_children, child);
  write_unlock(&child->fs_parent->fs_children_lock);

  child->fs_parent = new_parent; // supposed to be atomic
  
  if (new_parent != META_FS) {
    write_lock(&new_parent->fs_children_lock);
    HASH_ADD_BY_PTR(fs_child, new_parent->fs_children, fs_id, child);
    write_unlock(&new_parent->fs_children_lock);
  }
}

void fsnode_bridge(struct cinq_fsnode *out) {
  DEBUG_ON_(HASH_CNT(fs_child, out->fs_children) > 1,
            "[Warn@fsnode_bridge] crossing out who has more than one child: "
            "%lx(%s).\n", out->fs_id, out->fs_name);
  if (out->fs_children) {
    fsnode_move(out->fs_children, out->fs_parent);
  }
  fsnode_evict(out);
}
