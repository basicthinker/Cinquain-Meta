/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  inode.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/11/12.
//

#include "cinq_meta.h"
#include "stub.h"

static inline inode_t *__cinq_lookup(inode_t *dir, struct qstr *name,
                                     struct cinq_fsnode *fsnode) {
  return NULL;
}

// @dentry: a negative dentry, namely whose d_inode is null.
//  It is used to pass in target name.     
struct dentry *cinq_lookup(inode_t *dir, struct dentry *dentry,
                           struct nameidata *nameidata) {
  if (dentry->d_name.len >= MAX_FILE_NAME_LEN)
    return ERR_PTR(-ENAMETOOLONG);
  
  inode_t *inode = __cinq_lookup(dir, &dentry->d_name,
                                 nameidata->path.dentry->d_fsdata);
  if (!inode) {
    d_add(dentry, NULL);
    return NULL;
  }
  return d_splice_alias(inode, dentry);
}