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
    if (unlikely(tag)) atomic_dec(&tag->t_count);
  } else {
    struct cinq_inode *cnode = filp->private_data;
    if (unlikely(cnode)) atomic_dec(&cnode->ci_count);
  }
  return 0;
}


/* Relationship between i_mode and the DT_xxx types */
static inline unsigned char dt_type(struct inode *inode) {
  return (inode->i_mode >> 12) & 15;
}

#define move_cursor(cur, count, hh) ( \
  atomic_dec(&cursor->count), \
  cursor = cursor->hh.next, \
  cursor ? atomic_inc(&cursor->count) : NULL, \
  filp->private_data = cursor \
)

int cinq_readdir(struct file *filp, void *dirent, filldir_t filldir) {
	struct dentry *dentry = filp->f_path.dentry;
  struct inode *inode = dentry->d_inode;
  struct cinq_inode *cnode = i_cnode(inode);
  char *name;
  
  if (unlikely(inode_meta_root(inode))) {
    struct cinq_tag *cursor = filp->private_data;

    switch (filp->f_pos) {
      case 0:
        if (filldir(dirent, ".", 1, filp->f_pos, inode->i_ino, DT_DIR) < 0)
          break;
        filp->f_pos++;
        /* fallthrough */
      case 1:
        if (filldir(dirent, "..", 2, filp->f_pos,
                    parent_ino(dentry), DT_DIR) < 0)
          break;
        filp->f_pos++;
        /* fallthrough */
      default:
        read_lock(&cnode->ci_tags_lock);
        if (filp->f_pos == 2 && cursor && cursor != cnode->ci_tags) {
          atomic_dec(&cursor->t_count);
          cursor = cnode->ci_tags;
          atomic_inc(&cursor->t_count);
          filp->private_data = cursor;
        }
        for (; cursor != NULL; move_cursor(cursor, t_count, hh)) {
          if (cursor->t_fs == META_FS) continue;
          if (filldir(dirent, (name = cursor->t_fs->fs_name), strlen(name),
                      filp->f_pos, cursor->t_inode->i_ino, DT_DIR) < 0)
            return 0;
          filp->f_pos++;
        }
        read_unlock(&cnode->ci_tags_lock);
    }
  } else {
    struct cinq_inode *cursor = filp->private_data;
    struct inode *target;
    ino_t ino;

    switch (filp->f_pos) {
      case 0:
        if (filldir(dirent, ".", 1, filp->f_pos, inode->i_ino, DT_DIR) < 0)
          break;
        filp->f_pos++;
        /* fallthrough */
      case 1:
        ino = cnode_lookup_inode(cnode->ci_parent, dentry->d_fsdata)->i_ino;
        if (filldir(dirent, "..", 2, filp->f_pos, ino, DT_DIR) < 0)
          break;
        filp->f_pos++;
        /* fallthrough */
      default:
        read_lock(&cnode->ci_children_lock);
        if (filp->f_pos == 2 && cursor && cursor != cnode->ci_children) {
          atomic_dec(&cursor->ci_count);
          cursor = cnode->ci_children;
          atomic_inc(&cursor->ci_count);
          filp->private_data = cursor;
        }
        for (; cursor != NULL; move_cursor(cursor, ci_count, ci_child)) {
          target = cnode_lookup_inode(cursor, dentry->d_fsdata);
          if (!target) continue;
          if (filldir(dirent, (name = cursor->ci_name), strlen(name),
                      filp->f_pos, target->i_ino, dt_type(target)) < 0)
            return 0;
          filp->f_pos++;
        }
        read_unlock(&cnode->ci_children_lock);
    }
  }
  return 0;
}
