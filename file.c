/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  file.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/20/12.
//

#include "cinq_meta.h"

int cinq_file_open(struct inode *inode, struct file *file) {
  return 0;
}

ssize_t cinq_file_read(struct file *filp, char *buf, size_t len, loff_t *ppos) {
  return 0;
}

ssize_t cinq_file_write(struct file *filp, const char *buf, size_t len,
                   loff_t *ppos) {
  return 0;
}

int cinq_file_release(struct inode * inode, struct file * filp) {
  return 0;
}

int cinq_dir_open(struct inode *inode, struct file *filp) {
  struct inode *dir = filp->f_dentry->d_inode;
  if (unlikely(!dir)) {
    DEBUG_("[Error@cinq_dir_open] meets null inode.");
    return -EINVAL;
  }
  struct cinq_inode *cnode = i_cnode(dir);
  if (unlikely(inode_meta_root(dir))) {
    read_lock(&cnode->ci_tags_lock);
    filp->private_data = cnode->ci_tags;
    atomic_inc(&cnode->ci_tags->t_count);
    read_unlock(&cnode->ci_tags_lock);
  } else {
    read_lock(&cnode->ci_children_lock);
    filp->private_data = cnode->ci_children;
    atomic_inc(&cnode->ci_children->ci_count);
    read_unlock(&cnode->ci_children_lock);
  }
  return 0;
}

int cinq_dir_release(struct inode * inode, struct file * filp) {
  struct inode *dir = filp->f_dentry->d_inode;
  if (unlikely(!dir)) {
    DEBUG_("[Error@cinq_dir_open] meets null inode.");
    return -EINVAL;
  }
  if (unlikely(inode_meta_root(dir))) {
    struct cinq_tag *tag = filp->private_data;
    atomic_dec(&tag->t_count);
  } else {
    struct cinq_inode *cnode = filp->private_data;
    atomic_dec(&cnode->ci_count);
  }
  return 0;
}


/* Relationship between i_mode and the DT_xxx types */
static inline unsigned char dt_type(struct inode *inode) {
  return (inode->i_mode >> 12) & 15;
}

int cinq_readdir(struct file *filp, void *dirent, filldir_t filldir) {
	struct dentry *dentry = filp->f_path.dentry;
  struct inode *inode = dentry->d_inode;
  struct cinq_inode *dir_cnode = i_cnode(inode);
  
  if (unlikely(inode_meta_root(inode))) {
    struct cinq_tag *tag;
    read_lock(&dir_cnode->ci_tags_lock);
    for (tag = filp->private_data; tag != NULL;
         atomic_dec(&tag->t_count), tag = tag->hh.next) {
      atomic_inc(&tag->t_count);
      if (tag->t_fs == META_FS) continue;
      if (filldir(dirent, tag->t_fs->fs_name, strlen(tag->t_fs->fs_name),
                  filp->f_pos, tag->t_inode->i_ino, DT_DIR) < 0) {
        filp->private_data = tag;
        return 0;
      }
      filp->f_pos++;
    }
    read_unlock(&dir_cnode->ci_tags_lock);
  } else {
    ino_t ino;
    struct cinq_inode *cnode;
    struct inode *cur;

    switch (filp->f_pos) {
      case 0:
        if (filldir(dirent, ".", 1, filp->f_pos, inode->i_ino, DT_DIR) < 0)
          break;
        filp->f_pos++;
        /* fallthrough */
      case 1:
        ino = cnode_lookup_inode(dir_cnode->ci_parent, dentry->d_fsdata)->i_ino;
        if (filldir(dirent, "..", 2, filp->f_pos, ino, DT_DIR) < 0)
          break;
        filp->f_pos++;
        /* fallthrough */
      default:
        read_lock(&dir_cnode->ci_children_lock);
        for (cnode = dir_cnode->ci_children; cnode != NULL;
             atomic_dec(&cnode->ci_count), cnode = cnode->ci_child.next) {
          cur = cnode_lookup_inode(cnode, dentry->d_fsdata);
          atomic_inc(&cnode->ci_count);
          if (!cur) continue;
          if (filldir(dirent, cnode->ci_name, strlen(cnode->ci_name),
                      filp->f_pos, cur->i_ino, dt_type(cur)) < 0) {
            filp->private_data = cnode;
            return 0;
          }
          filp->f_pos++;
        }
        read_unlock(&dir_cnode->ci_children_lock);
    }
  }
  return 0;
}
