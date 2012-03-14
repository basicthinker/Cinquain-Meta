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


static struct cinq_inode *inode_new(struct cinq_inode *parent) {
  struct cinq_inode *inode = inode_malloc();
  memset(inode, 0, sizeof(struct cinq_inode));
  
  inode->i_no = (unsigned long)inode;
  if ((void *)inode->i_no != inode) {
    log("[Error@inode_new] conversion fails: i_no %lx != inode %p",
        inode->i_no, inode);
    return ERR_PTR(-EADDRNOTAVAIL);
  }
  
  inode->i_ctime = CURRENT_SEC;
  inode->i_mtime = inode->i_ctime;
  
  // Cinquain-specific
  if (parent) {
    inode->in_parent = parent;
  } else {
    inode->in_parent = inode; // root
  }

  rwlock_init(&inode->in_tags_lock);
  rwlock_init(&inode->in_children_lock);
  inode->in_tags = NULL;
  inode->in_children = NULL;
  return inode;
}

// @dentry: contains fs_node address in d_fsdata
int cinq_mkdir(inode_t *dir, struct dentry *dentry, int mode) {
  struct cinq_inode *parent = (struct cinq_inode *)dir->i_no;
  struct cinq_fsnode *fs = dentry->d_fsdata;
  
  struct cinq_inode *child;
  read_lock(&parent->in_children_lock);
  HASH_FIND_BY_STR(in_child, parent->in_children,
                   (const char *)dentry->d_name.name, child);
  read_unlock(&parent->in_children_lock);
  if (child) {
    struct cinq_fsnode *tag = NULL;
    write_lock(&child->in_tags_lock);
    HASH_FIND_BY_INT(fs_tag, child->in_tags, &fs->fs_id, tag);
    if (tag) {
      
    }
    write_unlock(&child->in_tags_lock);
  } else {
    child = inode_new(parent);
  }
  
  return 0;
}

// @fsnode: in-out parameter
static inline inode_t *__cinq_lookup(const inode_t *dir, const char *name,
                                     struct cinq_fsnode **fsnode_p) {
  struct cinq_inode *parent = (struct cinq_inode *)dir->i_no;
  struct cinq_inode *target;
  read_lock(&parent->in_children_lock);
  HASH_FIND_BY_STR(in_child, parent->in_children, name, target);
  read_unlock(&parent->in_children_lock);
  
  if (!target) {
    log("[Info@__cinq_lookup] no dir or file name is found: %s@%p\n",
        name, dir);
    return NULL;
  }
  
  struct cinq_fsnode *lookuper = *fsnode_p;
  struct cinq_fsnode *found = NULL;
  read_lock(&target->in_tags_lock);
  HASH_FIND_BY_INT(fs_tag, target->in_tags, &lookuper->fs_id, found);
  while (!found) {
    if (fsnode_is_root(lookuper)) {
      log("[Info@__cinq_lookup] no relative file system has that dir or file:"
          "%s@%u", name, (*fsnode_p)->fs_id);
      break;
    }
    lookuper = lookuper->fs_parent;
    HASH_FIND_BY_INT(fs_tag, target->in_tags, &lookuper->fs_id, found);
  }
  read_unlock(&target->in_tags_lock);
 
  *fsnode_p = found;
  return target;
}

// @dentry: a negative dentry, namely whose d_inode is null.
//    It is used to pass in target name.
// @nameidata: reserves the result of last segment
struct dentry *cinq_lookup(inode_t *dir, struct dentry *dentry,
                           struct nameidata *nameidata) {
  if (dentry->d_name.len >= MAX_NAME_LEN)
    return ERR_PTR(-ENAMETOOLONG);
  
  struct cinq_fsnode *lookuper = nameidata->path.dentry->d_fsdata;
  inode_t *inode = __cinq_lookup(dir, (const char *)dentry->d_name.name,
                                 &lookuper);
  if (!inode) {
    d_add(dentry, NULL);
    return NULL;
  }
  dentry->d_fsdata = lookuper;
  return d_splice_alias(inode, dentry);
}
