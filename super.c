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

struct thread_task journal_thread;

struct cinq_journal cinq_journal;

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
	sb->s_time_gran	= 1;
  
	inode = cnode_make_tree(sb);
	if (!inode) {
		return -ENOMEM;
	}
  
	root = d_alloc_root(inode);
  sb->s_root = root;
  if (!root) {
    cnode_evict_all(i_cnode(inode));
    return -ENOMEM;
	}
  return 0;
}

struct dentry *cinq_mount (struct file_system_type *fs_type, int flags,
                           const char *dev_name, void *data) {
  journal_init(&cinq_journal, "Cinquain");
  thread_init(&journal_thread, journal_writeback, &cinq_journal,
              "cinquain-journal");
  thread_run(&journal_thread);
  return mount_nodev(fs_type, flags, data, cinq_fill_super_);
}

void cinq_kill_sb (struct super_block *sb) {
  if (sb->s_root) {
    while (!journal_empty_syn(&cinq_journal)) {
      sleep(1);
    }
    thread_stop(&journal_thread);
    cnode_evict_all(i_cnode(sb->s_root->d_inode));
    d_genocide(sb->s_root);
  }
  DEBUG_ON_(!sb->s_root, "[Warning@cinq_kill_sb]: invoked on null dentry.\n");
}

void cinq_evict_inode(struct inode *inode) {
  if (!inode->i_nlink) { // && !is_bad_inode(inode)
    invalidate_inode_buffers(inode);
  }
}


THREAD_FUNC_(journal_writeback)(void *data) {
  struct cinq_journal *journal = data;
  struct journal_entry *entry;
  int num = 0;
  
  while (!thread_should_stop()) {
    set_current_state(TASK_RUNNING);
    
    while (!journal_empty_syn(journal)) {
      entry = journal_get_syn(journal);
      int key_size = sizeof(entry->key);
      switch (entry->action) {
        case CREATE:
          fprintf(stdout, "journal (%d)\t- CREATE key %lx(%d).\n",
                  ++num, *((unsigned long *)entry->key), key_size);
          break;
        case UPDATE:
          fprintf(stdout, "journal (%d)\t- UPDATE key %lx(%d).\n",
                  ++num, *((unsigned long *)entry->key), key_size);
          break;
        case DELETE:
          fprintf(stdout, "journal (%d)\t- UPDATE key %lx(%d).\n",
                  ++num, *((unsigned long *)entry->key), key_size);
          break;
        default:
          DEBUG_("[Error@journal_writeback] journal entry action is NOT valid:"
                 " %d.\n", entry->action);
      }
    }
    
    set_current_state(TASK_INTERRUPTIBLE);
    msleep(1000);
  }
  THREAD_RETURN_;
}

void journal_fsnode(struct cinq_fsnode *fsnode, enum journal_action action) {
  struct journal_entry *entry = journal_entry_new(&fsnode->fs_id,
                                                  fsnode, action);
  journal_add_syn(&cinq_journal, entry);
}

void journal_cnode(struct cinq_inode *cnode, enum journal_action action) {
  struct journal_entry *entry = journal_entry_new(&cnode->ci_id, cnode, action);
  journal_add_syn(&cinq_journal, entry);
}

void journal_inode(struct inode *inode, enum journal_action action) {
  struct journal_entry *entry = journal_entry_new(&inode->i_ino, inode, action);
  journal_add_syn(&cinq_journal, entry);
}
