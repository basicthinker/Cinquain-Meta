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
static inline int fsnode_ancestor(struct cinq_fsnode *ancestor,
                                  struct cinq_fsnode *descendant) {
  while (!fsnode_is_root(descendant)) {
    descendant = descendant->parent;
    if (descendant == ancestor) return 1;
  }
  return 0;
}

struct cinq_fsnode *fsnode_alloc(struct cinq_fsnode *parent) {
  struct cinq_fsnode *fsnode = fsnode_malloc();
  if (parent) {
    fsnode->parent = parent;
  } else {
    fsnode->parent = fsnode; // denotes root
  }
  return fsnode;
}

void fsnode_free(struct cinq_fsnode *fsnode) {
  if (fsnode->children) {
    log("[fsnode_free] fsnode (tag: %d) is deleted who still has children.",
        fsnode->tag);
  }
  fsnode_mfree(fsnode);
}


