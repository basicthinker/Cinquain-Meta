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

#include "vfs.h"
#include "journal.h"

/* Cinquain File System Data Structures and Operations */

struct cinq_fsnode {
  unsigned long fs_id;
  char fs_name[MAX_NAME_LEN + 1];
  struct cinq_fsnode *fs_parent;
  struct dentry *fs_root;
  spinlock_t lock; // used for fs holders
  
  // Using hash table to store children
  struct cinq_fsnode *fs_children;
  rwlock_t fs_children_lock;
  UT_hash_handle fs_child; // used for parent's children
  UT_hash_handle fs_tag; // used for cinq_inode's tags
  UT_hash_handle fs_member; // used for global file system list
};

static inline int fsnode_is_root(const struct cinq_fsnode *fsnode) {
  return fsnode->fs_parent == NULL;
}

/* fsnode.c */

// Creates a fsnode.
// @parent the fsnode's parent, while NULL indicates a root fsnode.
extern struct cinq_fsnode *fsnode_new(struct cinq_fsnode *parent,
                                      const char *name);

// Destroys a single fsnode without children
extern void fsnode_evict(struct cinq_fsnode *fsnode);

// Destroys recursively a single fsnode and all its descendants
extern void fsnode_evict_all(struct cinq_fsnode *fsnode);

extern void fsnode_move(struct cinq_fsnode *child,
                        struct cinq_fsnode *new_parent);

// Removes the fsnode and connect its single child to its parent
extern void fsnode_bridge(struct cinq_fsnode *out);

enum cinq_visibility {
  CINQ_VISIBLE = 0,
  CINQ_INVISIBLE = 1
};
#define CINQ_MODE_SHIFT 30


struct cinq_file_systems {
  rwlock_t lock;
  struct cinq_fsnode *cfs_table;
};

static inline void cfs_init(struct cinq_file_systems *cfs) {
  rwlock_init(&cfs->lock);
  cfs->cfs_table = NULL;
}

static inline struct cinq_fsnode *cfs_find_(struct cinq_file_systems *cfs,
                                            const char *name) {
  struct cinq_fsnode *fsnode;
  HASH_FIND_BY_STR(fs_member, cfs->cfs_table, name, fsnode);
  return fsnode;
}

static inline struct cinq_fsnode *cfs_find_syn(struct cinq_file_systems *cfs,
                                               const char *name) {
  struct cinq_fsnode *fsnode;
  read_lock(&cfs->lock);
  fsnode = cfs_find_(cfs, name);
  read_unlock(&cfs->lock);
  return fsnode;
}

static inline void cfs_add_(struct cinq_file_systems *cfs,
                            struct cinq_fsnode *fs) {
  HASH_ADD_BY_STR(fs_member, cfs->cfs_table, fs_name, fs);
}

static inline void cfs_add_syn(struct cinq_file_systems *cfs,
                               struct cinq_fsnode *fs) {
  write_lock(&cfs->lock);
  cfs_add_(cfs, fs);
  write_unlock(&cfs->lock);
}

static inline void cfs_rm_(struct cinq_file_systems *cfs,
                           struct cinq_fsnode *fs) {
  HASH_DELETE(fs_member, cfs->cfs_table, fs);
}

static inline void cfs_rm_syn(struct cinq_file_systems *cfs,
                              struct cinq_fsnode *fs) {
  write_lock(&cfs->lock);
  cfs_rm_(cfs, fs);
  write_unlock(&cfs->lock);
}

struct cinq_inode;

struct cinq_tag {
  struct cinq_fsnode *t_fs; // key for hh
  struct cinq_inode *t_host; // who holds the hash table this tag belongs to
  struct inode *t_inode; // whose i_no points to this tag

  atomic_t t_nchild;
  atomic_t t_count;
  enum cinq_visibility t_mode;
  unsigned char t_file_handle[FILE_HASH_WIDTH];
  char *t_symname;

  UT_hash_handle hh; // default handle name
};

static inline struct cinq_tag *i_tag(const struct inode *inode) {
  return (struct cinq_tag *)inode->i_ino;
}

static inline struct cinq_fsnode *i_fs(const struct inode *inode) {
  return i_tag(inode)->t_fs;
}

static inline int negative(const struct cinq_tag *tag) {
  return tag->t_inode == NULL;
}

static inline struct inode *cinq_iget(struct super_block *sb,
                                      unsigned long int ino) {
  struct cinq_tag *tag = (struct cinq_tag *)ino;
  return negative(tag) ? NULL : tag->t_inode;
}

static inline int impenetrable(const struct cinq_tag *tag,
                               const struct cinq_fsnode *req_fs) {
   return (negative(tag) && tag->t_fs == req_fs) ||
          (negative(tag) && tag->t_mode == CINQ_VISIBLE);
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
  atomic_t ci_count;
};

// No inode cache is necessary since cinq_inodes are in memory.
// Therefore no public alloc/free-like functions are provided.

// Retrieves cinq_inode pointer from inode
static inline struct cinq_inode *i_cnode(const struct inode *inode) {
  return i_tag(inode)->t_host;
}

static inline int inode_meta_root(const struct inode *inode) {
  return i_tag(inode)->t_fs == META_FS;
}

#define foreach_ancestor_tag(fs, tag, cnode) \
    for (tag = cnode_find_tag_(cnode, fs); \
         fs != META_FS && (!tag || !impenetrable(tag, fs)); \
         fs = fs->fs_parent, tag = cnode_find_tag_(cnode, fs))

/* super.c */
extern struct cinq_file_systems file_systems;

extern struct dentry *cinq_mount (struct file_system_type *fs_type, int flags,
                                  const char *dev_name, void *data);
extern void cinq_kill_sb (struct super_block *sb);

extern struct inode *cinq_alloc_inode(struct super_block *sb);

extern void cinq_evict_inode(struct inode *inode);

extern void journal_fsnode(struct cinq_fsnode *fsnode,
                           enum journal_action action);
extern void journal_cnode(struct cinq_inode *cnode,
                          enum journal_action action);
extern void journal_inode(struct inode *inode, enum journal_action action);


/* cnode.c */
extern struct inode *cnode_lookup_inode(struct cinq_inode *cnode,
                                        struct cinq_fsnode *fs);
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
extern int cinq_mknod(struct inode *dir, struct dentry *dentry, int mode,
                      dev_t dev);
extern int cinq_link(struct dentry *old_dentry, struct inode *dir,
                     struct dentry *dentry);
extern int cinq_unlink(struct inode *dir, struct dentry *dentry);

extern int cinq_symlink(struct inode *dir, struct dentry *dentry,
                        const char *symname);
extern void *cinq_follow_link(struct dentry *dentry, struct nameidata *nd);

extern int cinq_rmdir(struct inode *dir, struct dentry *dentry);

extern int cinq_rename(struct inode *old_dir, struct dentry *old_dentry,
                       struct inode *new_dir, struct dentry *new_dentry);
extern int cinq_setattr(struct dentry *dentry, struct iattr *attr);

extern void cinq_destroy_inode(struct inode *inode);

// @dentry: contains cinq_fsnode.fs_id in its d_fsdata, which specifies
//    the file system to take the operation.
//    Its d_name contains the string name of new dir.
// @mode: the left most 2 bits denote inheritance type
extern int cinq_mkdir(struct inode *dir, struct dentry *dentry, int mode);

// helper functions
extern struct inode *cnode_make_tree(struct super_block *sb);

extern void cnode_evict_all(struct cinq_inode *root);


/* file.c */

extern ssize_t cinq_file_read(struct file *filp, char *buf, size_t len,
                              loff_t *ppos);
extern ssize_t cinq_file_write(struct file *filp, const char *buf, size_t len,
                               loff_t *ppos);
extern int cinq_dir_open(struct inode *inode, struct file *file);

extern int cinq_dir_release(struct inode * inode, struct file * filp);

extern loff_t cinq_dir_lseek(struct file *filp, loff_t offset, int origin);

extern int cinq_readdir (struct file * filp, void * dirent, filldir_t filldir);

extern int cinq_dir_release(struct inode * inode, struct file * filp);


/* cinq_meta.c */
extern struct file_system_type cinqfs;
extern const struct super_operations cinq_super_operations;
extern const struct inode_operations cinq_dir_inode_operations;
extern const struct inode_operations cinq_file_inode_operations;
extern const struct inode_operations cinq_symlink_inode_operations;
extern const struct file_operations cinq_file_operations;
extern const struct file_operations cinq_dir_operations;
extern const struct export_operations cinq_export_operations;


#ifdef __KERNEL__

extern int init_cnode_cache(void);
extern void destroy_cnode_cache(void);

extern int init_tag_cache(void);
extern void destroy_tag_cache(void);

extern int init_inode_cache(void);
extern void destroy_inode_cache(void);

extern int init_fsnode_cache(void);
extern void destroy_fsnode_cache(void);

/* NOT used in user space */

static struct backing_dev_info cinq_backing_dev_info  __read_mostly = {
	.ra_pages	= 0,	/* No readahead */
	.capabilities	= BDI_CAP_NO_ACCT_AND_WRITEBACK | BDI_CAP_SWAP_BACKED,
#ifdef __OLD_KERNEL__
  .unplug_io_fn	= default_unplug_io_fn,
#endif // __OLD_KERNEL__
};

static struct dentry *cinq_get_parent(struct dentry *child) {
	return ERR_PTR(-ESTALE);
}

static int cinq_match(struct inode *ino, void *vfh) {
	__u32 *fh = vfh;
	__u64 inum = fh[2];
	inum = (inum << 32) | fh[1];
	return ino->i_ino == inum && fh[0] == ino->i_generation;
}

static struct dentry *cinq_fh_to_dentry(struct super_block *sb,
                                        struct fid *fid, int fh_len,
                                        int fh_type) {
  struct inode *inode;
  struct dentry *dentry = NULL;
  u64 inum = fid->raw[2];
  inum = (inum << 32) | fid->raw[1];

  if (fh_len < 3) return NULL;
  
// inode = cinq_iget(NULL, inum);
  inode = ilookup5(sb, (unsigned long)(inum + fid->raw[0]),
                   cinq_match, fid->raw);
  if (inode) {
	dentry = d_find_alias(inode);
	iput(inode);
  } else DEBUG_(KERN_ERR "cinq_fh_to_dentry: NOT found inode for fid '%x-%x-%x'.\n",
		  	  	fid->raw[2], fid->raw[1], fid->raw[0]);

  if(!dentry) DEBUG_(KERN_ERR "cinq_fh_to_dentry: NOT found dentry for fid '%x-%x-%x'.\n",
			  fid->raw[2], fid->raw[1], fid->raw[0]);
  else DEBUG_("cinq_fh_to_dentry: handle '%x-%x-%x' ==> dentry '%s' (%p) by '%s'.\n",
		      fid->raw[2], fid->raw[1], fid->raw[0], dentry->d_name.name, dentry,
			  ((struct cinq_fsnode *)dentry->d_fsdata)->fs_name);

  return dentry;
}

static int cinq_encode_fh(struct dentry *dentry, __u32 *fh, int *len,
                          int connectable) {
	struct inode *inode = dentry->d_inode;
  
	if (*len < 3)
		return 255;
  
	if (inode_unhashed(inode)) {
		/* Unfortunately insert_inode_hash is not idempotent,
		 * so as we hash inodes here rather than at creation
		 * time, we need a lock to ensure we only try
		 * to do it once
		 */
		static DEFINE_SPINLOCK(lock);
		spin_lock(&lock);
		if (inode_unhashed(inode))
			__insert_inode_hash(inode,
                          inode->i_ino + inode->i_generation);
		spin_unlock(&lock);
	}
  
	fh[0] = inode->i_generation;
	fh[1] = inode->i_ino;
	fh[2] = ((__u64)inode->i_ino) >> 32;
  
	*len = 3;
	DEBUG_("cinq_encode_fh: dentry %s (%p) by '%s' ==> handle '%x-%x-%x'.\n",
			dentry->d_name.name, dentry,
			((struct cinq_fsnode *)dentry->d_fsdata)->fs_name,
			fh[2], fh[1], fh[0]);
	return 1;
}

#endif


#endif // CINQUAIN_META_CINQ_META_H_
