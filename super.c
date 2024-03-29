/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  super.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/22/12.
//

#include "cinq_meta.h"
#include "cinq_cache/cinq_cache.h"
#include "thread.h"

struct cinq_file_systems file_systems;
static struct thread_task journal_thread;
static struct cinq_journal cinq_journal;

#ifdef CINQ_DEBUG
#ifndef __KERNEL__
atomic_t num_dentry_;
#endif
atomic_t num_inode_;
#endif // CINQ_DEBUG

// @data: can be NULL
static int cinq_fill_super_(struct super_block *sb, void *data, int silent) {
  struct inode *inode = NULL;
  struct dentry *root;
  // int err;
#ifdef __KERNEL__
  save_mount_options(sb, data);
#endif
  // err = cinq_parse_options(data, &fsi->mount_opts);
  
  sb->s_maxbytes = MAX_LFS_FILESIZE;
  sb->s_blocksize	= PAGE_CACHE_SIZE;
  sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
  sb->s_magic	= CINQ_MAGIC;
  sb->s_op = &cinq_super_operations;
  sb->s_export_op = &cinq_export_operations;
  sb->s_time_gran	= 1;
  
  inode = cnode_make_tree(sb);
  if (!inode) {
	return -ENOMEM;
  }
  
  root = d_alloc_root(inode);
  if (!root) {
    cnode_evict_all(i_cnode(inode));
    return -ENOMEM;
  }
  root->d_fsdata = META_FS;
  sb->s_root = root;
  
  return 0;
}

struct dentry *cinq_mount(struct file_system_type *fs_type, int flags,
                           const char *dev_name, void *data) {
  cfs_init(&file_systems);
  journal_init(&cinq_journal, "Cinquain");
  rwcache_init();
  thread_init(&journal_thread, journal_writeback, &cinq_journal,
              "cinquain-journal");
  thread_run(&journal_thread);
  return mount_nodev(fs_type, flags, data, cinq_fill_super_);
}

void cinq_kill_sb(struct super_block *sb) {
  if (sb->s_root) {
    int i = 0;
    for (i = 0; i < NUM_WAY; ++i) {
      while (!journal_empty_syn(&cinq_journal, i)) {
        sleep(1);
      }
    }
    rwcache_fini();
    thread_stop(&journal_thread);
    fsnode_evict_all(META_FS);
    d_genocide(sb->s_root);
    cnode_evict_all(i_cnode(sb->s_root->d_inode));
    // dput(sb->s_root); // cancel the extra reference and delete // FIX ME
  }
  DEBUG_ON_(!sb->s_root, "[Warn@cinq_kill_sb]: invoked on null dentry.\n");
}

THREAD_FUNC_(journal_writeback)(void *data) {
  int i = 0;
  struct cinq_journal *journal = data;
  struct cinq_jentry *entry;
  
  while (!thread_should_stop()) {
    set_current_state(TASK_RUNNING);
    
    for (i = 0; i < NUM_WAY; ++i) {
	  while (!journal_empty_syn(journal, i)) {
		entry = journal_get_syn(journal, i);
		jentry_free(entry);
      }
    }
    
    set_current_state(TASK_INTERRUPTIBLE);
    sleep(1);
  }
  THREAD_RETURN_;
}

void journal_fsnode(struct cinq_fsnode *fsnode, enum journal_action action) {
  struct cinq_jentry *entry = jentry_new(&fsnode->fs_id,
                                                  fsnode, action);
  journal_add_syn(&cinq_journal, entry);
}

void journal_cnode(struct cinq_inode *cnode, enum journal_action action) {
  struct cinq_jentry *entry = jentry_new(&cnode->ci_id, cnode, action);
  journal_add_syn(&cinq_journal, entry);
}

void journal_inode(struct inode *inode, enum journal_action action) {
  struct cinq_jentry *entry = jentry_new(&inode->i_ino, inode, action);
  journal_add_syn(&cinq_journal, entry);
}

