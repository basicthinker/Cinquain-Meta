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
#include "cinq_cache/cinq_cache.h"

#define SPNFS_DELIM_POS 7 // for spnfs style file name 

ssize_t cinq_file_read(struct file *filp, char *buf, size_t len, loff_t *ppos) {
  struct fingerprint fp;
  struct data_set *ds;
  struct data_entry *cur;
  char *fname = (char *)filp->f_dentry->d_name.name;
  loff_t buf_offset;
  loff_t data_offset;
  size_t cpy_len;
  loff_t filend = filp->f_pos + len;
  loff_t curend;
  
  if (*ppos >= filp->f_dentry->d_inode->i_size) return 0;
  
  fp.uid = 0;
  strncpy(fp.value, fname, SPNFS_DELIM_POS - 1);
  strncpy((char *)fp.value + SPNFS_DELIM_POS - 1,
          fname + SPNFS_DELIM_POS, FINGERPRINT_BYTES - SPNFS_DELIM_POS + 1);
  ds = wcache_read(&fp, filp->f_pos, len);
  
  list_for_each_entry(cur, &ds->entries, entry) {
    curend = cur->offset + cur->len;
    if (cur->offset < filp->f_pos) {
      buf_offset = 0;
      data_offset = filp->f_pos - cur->offset;
    } else {
      buf_offset = cur->offset - filp->f_pos;
      data_offset = 0;
    }
    cpy_len = curend > filend ?
        filend - cur->offset - data_offset : cur->len - data_offset;
    bufcpy(buf + buf_offset, cur->data + data_offset, cpy_len);
    DEBUG_ON_(cur->offset + data_offset + cpy_len > filend,
              "[Err@cinq_file_read] cache provides more than expected: %lld.\n",
              cur->offset + data_offset + cpy_len);
    *ppos += cpy_len;
  }
  return *ppos - filp->f_pos;
}

ssize_t cinq_file_write(struct file *filp, const char *buf, size_t len,
                        loff_t *ppos) {
  struct fingerprint fp;
  struct data_entry de;
  struct inode *inode = filp->f_path.dentry->d_inode;
  char *fname = (char *)filp->f_dentry->d_name.name;
  
  strncpy(fp.value, fname, SPNFS_DELIM_POS - 1);
  strncpy((char *)fp.value + SPNFS_DELIM_POS - 1,
          fname + SPNFS_DELIM_POS, FINGERPRINT_BYTES - SPNFS_DELIM_POS + 1);
  
  de.data = (char *)buf;
  de.offset = filp->f_pos;
  de.len = len;
  
  wcache_write(&fp, &de);
  *ppos += len;
  if (*ppos > inode->i_size) {
    i_size_write(inode, *ppos);
  }
  return len;
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
