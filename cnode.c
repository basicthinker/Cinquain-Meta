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

struct inode *cinq_alloc_inode(struct super_block *sb) {
  struct inode *inode = inode_malloc();
  
#ifndef __KERNEL__ // for user space
  inode->i_nlink = 1;
  inode->i_ctime = inode->i_mtime = current_time_();
#endif // __KERNEL__
  
  return inode;
}

static inline int cnode_is_root_(struct cinq_inode *cnode) {
  return cnode->ci_parent == cnode;
}

// Retrieves cinq_inode pointer from inode
static inline struct cinq_inode *cnode_(const struct inode *inode) {
  return ((struct cinq_tag *)inode->i_ino)->t_host;
}

static inline struct cinq_tag *tag_new_(struct cinq_fsnode *fs,
                                       struct cinq_inode *host,
                                       struct inode *inode,
                                       enum cinq_inherit_type mode) {
  struct cinq_tag *tag = tag_malloc();
  if (unlikely(!tag)) {
    return NULL;
  }
  tag->t_fs = fs;
  tag->t_host = host;
  tag->t_inode = inode;
  tag->t_mode = mode;
  
  inode->i_ino = (unsigned long) tag;
  return tag;
}

// Retrieves the super_operations corresponding to the tag
static inline const struct super_operations *s_op_(struct cinq_tag *tag) {
  return tag->t_inode->i_sb->s_op;
}

static inline void tag_free_(struct cinq_tag *tag) {
  s_op_(tag)->destroy_inode(tag->t_inode);
  write_lock(&tag->t_host->ci_tags_lock);
  HASH_DEL(tag->t_host->ci_tags, tag);
  write_unlock(&tag->t_host->ci_tags_lock);
  tag_mfree(tag);
}

static struct cinq_inode *cnode_new_(struct cinq_inode *parent) {
  
  struct cinq_inode *cnode = cnode_malloc();
  if (unlikely(!cnode)) {
    DEBUG_("[Error@cnode_new] allocation fails: parent=%p.p\n", parent);
    return NULL;
  }
  
  // Initializes cnode
  cnode->ci_id = (unsigned long)cnode;
  cnode->ci_tags = NULL;
  cnode->ci_children = NULL;
  rwlock_init(&cnode->ci_tags_lock);
  rwlock_init(&cnode->ci_children_lock);
  
  if (parent) {
    cnode->ci_parent = parent;
    write_lock(&parent->ci_children_lock);
    HASH_ADD_BY_PTR(ci_child, parent->ci_children, ci_id, cnode);
    write_unlock(&parent->ci_children_lock);
  } else {
    cnode->ci_parent = cnode; // root
  }
  return cnode;
}

static void cnode_free_(struct cinq_inode *cnode) {
  if (cnode->ci_tags || cnode->ci_children) {
    DEBUG_("[Error@cnode_free] failed to delete cnode %lx "
           "who still has tags or children.\n", cnode->ci_id);
    return;
  }
  struct cinq_inode *parent = cnode->ci_parent;
  if (!cnode_is_root_(cnode) && parent) {
    write_lock(&parent->ci_children_lock);
    HASH_REMOVE(ci_child, parent->ci_children, cnode);
    write_unlock(&parent->ci_children_lock);
  }
  cnode_free_(cnode);
}

// This function is NOT thread safe, since it is used in the end,
// and a single thread is proper then.
void cnode_free_all(struct cinq_inode *root) {
  if (root->ci_children) {
    struct cinq_inode *cur, *tmp;
    HASH_ITER(ci_child, root->ci_children, cur, tmp) {
      cnode_free_all(cur);
    }
    DEBUG_ON_(root->ci_children != NULL,
              "[Error@cnode_free_all] not empty children: %lu\n", root->ci_id);
  }
  if (root->ci_tags) {
    struct cinq_tag *cur, *tmp;
    HASH_ITER(hh, root->ci_tags, cur, tmp) {
      tag_free_(cur);
    }
    DEBUG_ON_(root->ci_tags != NULL,
              "[Error@cnode_free_all] not empty tags: %lu\n", root->ci_id);
  }
  cnode_free_(root);
}

int cinq_mkdir(struct inode *dir, struct dentry *dentry, int mode) {
  struct cinq_inode *parent = cnode_(dir);
  struct cinq_fsnode *fs = dentry->d_fsdata;
  
  struct cinq_inode *child;
  char *name = (char *)dentry->d_name.name;
  if (dentry->d_name.len > MAX_NAME_LEN) {
    DEBUG_("[Error@cinq_mkdir] name is too long: %s\n", name);
    return -ENAMETOOLONG;
  }
  
  write_lock(&parent->ci_children_lock);
  HASH_FIND_BY_STR(ci_child, parent->ci_children, name, child);
  
  if (child) {
    write_unlock(&parent->ci_children_lock);
    
    struct cinq_tag *tag = NULL;
    
    write_lock(&child->ci_tags_lock);
    HASH_FIND_PTR(child->ci_tags, &fs, tag);
    if (!tag) {
      struct inode *inode = cinq_alloc_inode(NULL);
      tag = tag_new_(fs, child, inode, 0);
      HASH_ADD_PTR(child->ci_tags, t_fs, tag);
    }
    tag->t_mode = (mode >> CINQ_MODE_SHIFT);
    tag->t_inode->i_mode = (mode & I_MODE_FILTER);
    write_unlock(&child->ci_tags_lock);
    
  } else {
    child = cnode_new_(parent);
    strcpy(child->ci_name, name);
    struct inode *inode = cinq_alloc_inode(NULL);
    struct cinq_tag *tag = tag_new_(fs, child, inode, CINQ_MERGE);
    HASH_ADD_PTR(child->ci_tags, t_fs, tag);
    HASH_ADD_BY_STR(ci_child, parent->ci_children, ci_name, child);
    write_unlock(&parent->ci_children_lock);
  }
  return 0;
}

// @fsnode: in-out parameter
static inline struct inode *cinq_lookup_(const struct inode *dir,
                                         const char *name,
                                         struct cinq_fsnode **fs_p) {
  struct cinq_inode *parent = cnode_(dir);
  struct cinq_inode *child;
  
  read_lock(&parent->ci_children_lock);
  HASH_FIND_BY_STR(ci_child, parent->ci_children, name, child);
  read_unlock(&parent->ci_children_lock);
  
  if (!child) {
    DEBUG_("[Info@cinq_lookup_] no dir or file name is found: %s@%p\n",
           name, dir);
    return NULL;
  }
  
  struct cinq_fsnode *fs = *fs_p;
  struct cinq_tag *tag = NULL;
  read_lock(&child->ci_tags_lock);
  HASH_FIND_PTR(child->ci_tags, &fs, tag);
  while (!tag) {
    if (fsnode_is_root(fs)) {
      DEBUG_("[Info@cinq_lookup_] no file system has that dir or file:"
             "%s from %lu", name, (*fs_p)->fs_id);
      break;
    }
    fs = fs->fs_parent;
    HASH_FIND_PTR(child->ci_tags, &fs, tag);
  }
  read_unlock(&child->ci_tags_lock);
 
  *fs_p = tag->t_fs;
  return tag->t_inode;
}

struct dentry *cinq_lookup(struct inode *dir, struct dentry *dentry,
                           struct nameidata *nameidata) {
  if (dentry->d_name.len >= MAX_NAME_LEN)
    return ERR_PTR(-ENAMETOOLONG);
  
  struct cinq_fsnode *fs = nameidata ?
      nameidata->path.dentry->d_fsdata : dentry->d_fsdata;
  struct inode *inode = cinq_lookup_(dir, (const char *)dentry->d_name.name,
                                     &fs);
  dentry->d_fsdata = fs;
  if (!inode) {
    d_add(dentry, NULL);
    return NULL;
  }
  return d_splice_alias(inode, dentry);
}
