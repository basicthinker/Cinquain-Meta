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

int cinq_open(struct inode *inode, struct file *file) {
  return 0;
}

ssize_t cinq_read(struct file *filp, char *buf, size_t len, loff_t *ppos) {
  return 0;
}

ssize_t cinq_write(struct file *filp, const char *buf, size_t len,
                   loff_t *ppos) {
  return 0;
}

int cinq_release_file(struct inode * inode, struct file * filp) {
  return 0;
}

/* Relationship between i_mode and the DT_xxx types */
static inline unsigned char dt_type(struct inode *inode) {
  return (inode->i_mode >> 12) & 15;
}

int cinq_readdir(struct file *filp, void *dirent, filldir_t filldir) {
	struct dentry *dentry = filp->f_path.dentry;
  struct inode *inode = dentry->d_inode;
  struct cinq_inode *cnode = i_cnode(inode);
  
  if (unlikely(inode_meta_root(inode))) {
    struct cinq_tag *tag;
    read_lock(&cnode->ci_tags_lock);
    for (tag = cnode->ci_tags; tag != NULL; tag = tag->hh.next) {
      if (tag->t_fs == META_FS) continue;
      if (filldir(dirent, tag->t_fs->fs_name, strlen(tag->t_fs->fs_name),
                  filp->f_pos, tag->t_inode->i_ino, DT_DIR) < 0)
        return 0;

      filp->f_pos++;
    }
    read_unlock(&cnode->ci_tags_lock);
    return 0;
  }

  ino_t ino;
  struct cinq_inode *cur;
  struct inode *sub;

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
			for (cur = cnode->ci_children; cur != NULL; cur = cur->ci_child.next) {
        sub = cnode_lookup_inode(cur, dentry->d_fsdata);
        if (!sub) continue;
				if (filldir(dirent, cur->ci_name, strlen(cur->ci_name),
				            filp->f_pos, sub->i_ino, dt_type(sub)) < 0)
					return 0;

				filp->f_pos++;
			}
			read_unlock(&cnode->ci_children_lock);
	}
	return 0;
}
