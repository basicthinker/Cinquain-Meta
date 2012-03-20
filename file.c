/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  file.c
//  cinquain-meta
//
//  Created by Jinglei Ren on 3/20/12.
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

int cinq_release_file (struct inode * inode, struct file * filp) {
  return 0;
}
