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
  char *fingerprint;
  int fp_len;
  if (file->private_data) {
    fingerprint = file->private_data;
    fp_len = FILE_HASH_WIDTH;
  } else {
    fingerprint = (char *)file->f_path.dentry->d_name.name;
    fp_len = strlen(fingerprint);
  }
  return 0;
}

ssize_t cinq_file_read(struct file *filp, char *buf, size_t len, loff_t *ppos) {
  char *str = "abcdefghijklmnopqrstuvwxyz\n";
  if (*ppos == strlen(str)) return 0;
  
  bufcpy(buf, str + filp->f_pos, 1);
  // filp->f_pos += 1;
  *ppos += 1;
  return 1;
}

ssize_t cinq_file_write(struct file *filp, const char *buf, size_t len,
                        loff_t *ppos) {
//  DEBUG_("cinq_file_write: TO DO write to memory cache: %c", *buf);
//  *ppos += 1;
//  return 1;
  *ppos += len;
  return len;
}

int cinq_file_release(struct inode * inode, struct file * filp) {
  DEBUG_("cinq_file_release: TO DO flush to back store.\n");
  filp->private_data = "fingerprint";
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
    struct cinq_tag *cur;
    read_lock(&cnode->ci_tags_lock);
    cur = filp->private_data = cnode->ci_tags;
    if (cur) atomic_inc(&cur->t_count);
    read_unlock(&cnode->ci_tags_lock);
  } else {
    struct cinq_inode *cur;
    read_lock(&cnode->ci_children_lock);
    cur = filp->private_data = cnode->ci_children;
    if (cur) atomic_inc(&cur->ci_count);
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
  atomic_dec(&cur->count), \
  cur = cur->hh.next, \
  cur ? atomic_inc(&cur->count) : NULL, \
  filp->private_data = cur \
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
        DEBUG_("cinq_readdir(1): filldir '.'\n");
        filp->f_pos++;
        /* fallthrough */
      case 1:
        if (filldir(dirent, "..", 2, filp->f_pos,
                    parent_ino(dentry), DT_DIR) < 0)
          break;
        DEBUG_("cinq_readdir(1): filldir '..'\n");
        filp->f_pos++;
        /* fallthrough */
      default:
        read_lock(&cnode->ci_tags_lock);
        if (filp->f_pos == 2) {
          if (cursor) atomic_dec(&cursor->t_count);
          cursor = cnode->ci_tags;
          if (cursor) atomic_inc(&cursor->t_count);
          filp->private_data = cursor;
        }
        for (; cursor != NULL; move_cursor(cursor, t_count, hh)) {
          if (cursor->t_fs == META_FS) continue;
          name = cursor->t_fs->fs_name;
          if (filldir(dirent, name, strlen(name),
                      filp->f_pos, cursor->t_inode->i_ino, DT_DIR) < 0)
            return 0;
          DEBUG_("cinq_readdir(1): filldir %s (%ld).\n", name, strlen(name));
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
        DEBUG_("cinq_readdir(2): filldir '.'\n");
        filp->f_pos++;
        /* fallthrough */
      case 1:
        ino = cnode_lookup_inode(cnode->ci_parent, dentry->d_fsdata)->i_ino;
        if (filldir(dirent, "..", 2, filp->f_pos, ino, DT_DIR) < 0)
          break;
        DEBUG_("cinq_readdir(2): filldir '..'\n");
        filp->f_pos++;
        /* fallthrough */
      default:
        read_lock(&cnode->ci_children_lock);
        if (filp->f_pos == 2) {
          if (cursor) atomic_dec(&cursor->ci_count);
          cursor = cnode->ci_children;
          if (cursor) atomic_inc(&cursor->ci_count);
          filp->private_data = cursor;
        }
        for (; cursor != NULL; move_cursor(cursor, ci_count, ci_child)) {
          target = cnode_lookup_inode(cursor, dentry->d_fsdata);
          if (!target) continue;
          name = cursor->ci_name;
          if (filldir(dirent, name, strlen(name),
                      filp->f_pos, target->i_ino, dt_type(target)) < 0)
            return 0;
          DEBUG_("cinq_readdir(2): filldir %s (%ld).\n", name, strlen(name));
          filp->f_pos++;
        }
        read_unlock(&cnode->ci_children_lock);
    }
  }
  return 0;
}
