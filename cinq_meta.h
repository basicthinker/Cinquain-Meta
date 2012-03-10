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

struct cinq_fsnode {
  unsigned short tag;
  struct cinq_fsnode *parent;
  struct cinq_fsnode *children;
};

static inline int fsnode_is_root(struct cinq_fsnode *fsnode) {
  return fsnode->parent == fsnode;
}

extern struct cinq_fsnode *fsnode_alloc(struct cinq_fsnode *parent);
extern void fsnode_free(struct cinq_fsnode *fsnode);
extern void fsnode_change_parent(struct cinq_fsnode *child,
                                 struct cinq_fsnode *new_parent);

#endif // CINQUAIN_META_CINQ_META_H_
