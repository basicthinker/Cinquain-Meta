/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  cinq_meta.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 4/20/12.
//

#include "cinq_meta.h"

#ifdef __KERNEL__

static int __init init_cinq_fs(void) {
  int err;

  err = init_cnode_cache();
  if (err) return err;

  err = init_fsnode_cache();
  if (err) return err;

  err = init_jentry_cache();
  if (err) return err;

  return register_filesystem(&cinqfs);
}

static void __exit exit_cinq_fs(void) {
  destroy_cnode_cache();
  destroy_fsnode_cache();
  destroy_jentry_cache();
  unregister_filesystem(&cinqfs);
}

module_init(init_cinq_fs);
module_exit(exit_cinq_fs);
MODULE_LICENSE("GPL");

#endif
