/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  cinq_meta.h
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/9/12.
//

#ifndef CINQUAIN_META_CINQ_META_H_
#define CINQUAIN_META_CINQ_META_H_

#include "journal.h"
#include "vfs.h"
#include "thread.h"

/* Cinquain File System Data Structures and Operations */

struct cinq_fsnode {
  unsigned long fs_id;
  char fs_name[MAX_NAME_LEN + 1];
  struct cinq_fsnode *fs_parent;
  struct dentry *fs_root;
  
  // Using hash table to store children
  struct cinq_fsnode *fs_children;
  rwlock_t fs_children_lock;
  UT_hash_handle fs_child; // used for parent's children
  UT_hash_handle fs_tag; // used for cinq_inode's tags
  UT_hash_handle fs_member; // used for global file system list
};

static inline int fsnode_is_root(const struct cinq_fsnode *fsnode) {
  return fsnode->fs_parent == fsnode;
}

/* fsnode.c */

// Creates a fsnode.
// @parent the fsnode's parent, while NULL indicates a root fsnode.
extern struct cinq_fsnode *fsnode_new(const char *name,
                                      struct cinq_fsnode *parent);

// Destroys a single fsnode without children
extern void fsnode_evict(struct cinq_fsnode *fsnode);

// Destroys recursively a single fsnode and all its descendants
extern void fsnode_evict_all(struct cinq_fsnode *fsnode);

extern void fsnode_move(struct cinq_fsnode *child,
                        struct cinq_fsnode *new_parent);

// Removes the fsnode and connect its single child to its parent
extern void fsnode_bridge(struct cinq_fsnode *out);

enum cinq_inherit_type {
  CINQ_MERGE = 0,
  CINQ_OVERWR = 1
};
#define CINQ_MODE_SHIFT 30


struct cinq_file_systems {
  rwlock_t lock;
  struct cinq_fsnode *fs_table;
};

static inline struct cinq_fsnode *cfs_find_(struct cinq_file_systems *table,
                                            const char *name) {
  struct cinq_fsnode *fsnode;
  HASH_FIND_BY_STR(fs_member, table->fs_table, name, fsnode);
  return fsnode;
}

static inline struct cinq_fsnode *cfs_find_syn(struct cinq_file_systems *table,
                                               const char *name) {
  read_lock(&table->lock);
  struct cinq_fsnode *fsnode = cfs_find_(table, name);
  read_unlock(&table->lock);
  return fsnode;
}

static inline void cfs_add_(struct cinq_file_systems *table,
                            struct cinq_fsnode *fs) {
  HASH_ADD_BY_STR(fs_member, table->fs_table, fs_name, fs);
}

static inline void cfs_add_syn(struct cinq_file_systems *table,
                               struct cinq_fsnode *fs) {
  write_lock(&table->lock);
  cfs_add_(table, fs);
  write_unlock(&table->lock);
}

static inline void cfs_rm_(struct cinq_file_systems *table,
                           struct cinq_fsnode *fs) {
  HASH_DELETE(fs_member, table->fs_table, fs);
}

static inline void cfs_rm_syn(struct cinq_file_systems *table,
                              struct cinq_fsnode *fs) {
  write_lock(&table->lock);
  cfs_rm_(table, fs);
  write_unlock(&table->lock);
}

struct cinq_inode;

struct cinq_tag {
  struct cinq_fsnode *t_fs; // key for hh
  struct cinq_inode *t_host; // who holds the hash table this tag belongs to
  struct inode *t_inode; // whose i_no points to this tag

  atomic_t t_nchild;
  enum cinq_inherit_type t_mode;
  unsigned char t_file_handle[FILE_HASH_WIDTH];

  UT_hash_handle hh; // default handle name
};

static inline void inc_nchild_(struct cinq_tag *tag) {
  atomic_inc(&tag->t_nchild);
}

static inline void drop_nchild_(struct cinq_tag *tag) {
  atomic_dec(&tag->t_nchild);
}

static inline struct cinq_tag *i_tag(const struct inode *inode) {
  return (struct cinq_tag *)inode->i_ino;
}

static inline struct cinq_fsnode *i_fs(const struct inode *inode) {
  return i_tag(inode)->t_fs;
}

static inline struct cinq_fsnode *d_fs(const struct dentry *dentry) {
  return i_tag(dentry->d_inode)->t_fs;
}


struct cinq_inode {
  unsigned long ci_id;
  char ci_name[MAX_NAME_LEN + 1];

  struct cinq_tag *ci_tags; // hash table of tags
  rwlock_t ci_tags_lock;
  
  struct cinq_inode *ci_parent;
  struct cinq_inode *ci_children; // hash table of children
  UT_hash_handle ci_child;
  rwlock_t ci_children_lock;
};

// No inode cache is necessary since cinq_inodes are in memory.
// Therefore no public alloc/free-like functions are provided.

// Retrieves cinq_inode pointer from inode
static inline struct cinq_inode *i_cnode(const struct inode *inode) {
  return i_tag(inode)->t_host;
}


/* super.c */

extern struct dentry *cinq_mount (struct file_system_type *fs_type, int flags,
                                  const char *dev_name, void *data);
extern void cinq_kill_sb (struct super_block *sb);

extern void cinq_dirty_inode(struct inode *inode);

extern int cinq_write_inode(struct inode *inode, struct writeback_control *wbc);

extern void cinq_evict_inode(struct inode *inode);

extern struct cinq_journal cinq_journal;

extern struct thread_task journal_thread;

extern THREAD_FUNC_(journal_writeback)(void *data);

extern void journal_fsnode(struct cinq_fsnode *fsnode,
                           enum journal_action action);
extern void journal_cnode(struct cinq_inode *cnode,
                          enum journal_action action);
extern void journal_inode(struct inode *inode, enum journal_action action);


/* cnode.c */

// @dentry: a negative dentry, namely whose d_inode is null.
//    dentry->d_fsdata should better contains cinq_fsnode.fs_id that specifies
//    the file system (cinq_fsnode) to take the operation. Otherwise,
//    the nameidata is used to get the fsnode.
//    dentry->d_name contains the name of target.
// @nameidata: reserves the result of last segment.
//    If the fsnode is specified via dentry->d_fsdata,
//    this parameter can be set null.
extern struct dentry *cinq_lookup(struct inode *dir, struct dentry *dentry,
                                  struct nameidata *nameidata);

// @dentry: a negative dentry, namely whose d_inode is null.
//    dentry->d_fsdata should better contains cinq_fsnode.fs_id that specifies
//    the file system (cinq_fsnode) to take the operation. Otherwise,
//    the nameidata is used to get the fsnode.
//    dentry->d_name contains the name of target.
// @nameidata: reserves the result of last segment.
//    If the fsnode is specified via dentry->d_fsdata,
//    this parameter can be set null.
extern int cinq_create(struct inode *dir, struct dentry *dentry,
                int mode, struct nameidata *nameidata);
extern int cinq_link(struct dentry *old_dentry, struct inode *dir,
              struct dentry *dentry);
extern int cinq_unlink(struct inode *dir, struct dentry *dentry);

extern int cinq_symlink(struct inode *dir, struct dentry *dentry, const char *symname);

extern int cinq_rmdir(struct inode *dir, struct dentry *dentry);

extern int cinq_rename(struct inode *old_dir, struct dentry *old_dentry,
                struct inode *new_dir, struct dentry *new_dentry);
extern int cinq_setattr(struct dentry *dentry, struct iattr *attr);

// @dentry: contains cinq_fsnode.fs_id in its d_fsdata, which specifies
//    the file system to take the operation.
//    Its d_name contains the string name of new dir.
// @mode: the left most 2 bits denote inheritance type
extern int cinq_mkdir(struct inode *dir, struct dentry *dentry, int mode);

// helper functions
extern struct inode *cnode_make_tree(struct super_block *sb);

extern void cnode_evict_all(struct cinq_inode *root);


/* file.c */
extern int cinq_open(struct inode *inode, struct file *file);

extern ssize_t cinq_read(struct file *filp, char *buf, size_t len,
                         loff_t *ppos);
extern ssize_t cinq_write(struct file *filp, const char *buf, size_t len,
                          loff_t *ppos);
extern int cinq_release_file (struct inode * inode, struct file * filp);

extern int cinq_readdir (struct file * filp, void * dirent, filldir_t filldir);


/* cinq_meta.c */
extern struct cinq_file_systems file_systems;
extern const struct file_system_type cinqfs;
extern const struct super_operations cinq_super_operations;
extern const struct inode_operations cinq_dir_inode_operations;
extern const struct inode_operations cinq_file_inode_operations;
extern const struct file_operations cinq_file_operations;
extern const struct file_operations cinq_dir_operations;

#endif // CINQUAIN_META_CINQ_META_H_
