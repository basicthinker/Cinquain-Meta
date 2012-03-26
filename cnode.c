/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  inode.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/11/12.
//

#include "cinq_meta.h"

static inline int inode_is_root_(const struct inode *inode) {
  return ((struct cinq_tag *)inode->i_ino)->t_fs == META_FS;
}

static inline int cnode_is_root_(const struct cinq_inode *cnode) {
  return cnode->ci_parent == cnode;
}

// @inode: whose i_ino is set to address of new tag.
static inline struct cinq_tag *tag_new_(struct cinq_fsnode *fs,
                                        struct cinq_inode *host,
                                        enum cinq_inherit_type mode,
                                        struct inode *inode) {
  struct cinq_tag *tag = tag_malloc();
  if (unlikely(!tag)) {
    return NULL;
  }
  tag->t_fs = fs;
  tag->t_host = host;
  tag->t_inode = inode;
  tag->t_mode = mode;
  
  inode->i_ino = (unsigned long) tag;
  return tag;
}

// Retrieves the super_operations corresponding to the tag
static inline const struct super_operations *s_op_(struct cinq_tag *tag) {
  return tag->t_inode->i_sb->s_op;
}

static inline void tag_free_(struct cinq_tag *tag) {
  destroy_inode(tag->t_inode);
  write_lock(&tag->t_host->ci_tags_lock);
  HASH_DEL(tag->t_host->ci_tags, tag);
  write_unlock(&tag->t_host->ci_tags_lock);
  tag_mfree(tag);
}

// @parent NULL for root cnode
static struct cinq_inode *cnode_new_(struct cinq_inode *parent, char *name) {
  
  struct cinq_inode *cnode = cnode_malloc();
  if (unlikely(!cnode)) {
    DEBUG_("[Error@cnode_new] allocation fails: parent=%p.\n", parent);
    return NULL;
  }
  
  // Initializes cnode
  cnode->ci_id = (unsigned long)cnode;
  strcpy(cnode->ci_name, name);
  cnode->ci_tags = NULL;
  cnode->ci_children = NULL;
  rwlock_init(&cnode->ci_tags_lock);
  rwlock_init(&cnode->ci_children_lock);
  
  if (parent) {
    cnode->ci_parent = parent;
  } else {
    cnode->ci_parent = cnode; // root
  }
  return cnode;
}

static void cnode_free_(struct cinq_inode *cnode) {
  if (cnode->ci_tags || cnode->ci_children) {
    DEBUG_("[Error@cnode_free] failed to delete cnode %lx "
           "who still has tags or children.\n", cnode->ci_id);
    return;
  }
  struct cinq_inode *parent = cnode->ci_parent;
  if (!cnode_is_root_(cnode) && parent) {
    write_lock(&parent->ci_children_lock);
    HASH_REMOVE(ci_child, parent->ci_children, cnode);
    write_unlock(&parent->ci_children_lock);
  }
  cnode_mfree(cnode);
}

// This function is NOT thread safe, since it is used in the end,
// and a single thread is proper then.
void cnode_free_all(struct cinq_inode *root) {
  if (root->ci_children) {
    struct cinq_inode *cur, *tmp;
    HASH_ITER(ci_child, root->ci_children, cur, tmp) {
      cnode_free_all(cur);
    }
    DEBUG_ON_(root->ci_children != NULL,
              "[Error@cnode_free_all] not empty children: %lx\n", root->ci_id);
  }
  if (root->ci_tags) {
    struct cinq_tag *cur, *tmp;
    HASH_ITER(hh, root->ci_tags, cur, tmp) {
      tag_free_(cur);
    }
    DEBUG_ON_(root->ci_tags != NULL,
              "[Error@cnode_free_all] not empty tags: %lx\n", root->ci_id);
  }
  cnode_free_(root);
}

static struct inode *cinq_new_inode_(struct inode *dir, int mode,
                                     const struct qstr *qstr) {
  struct super_block *sb = dir->i_sb;
  struct inode * inode = new_inode(sb);
  if (!inode) {
    return ERR_PTR(-ENOMEM);
  }
  
  inode_init_owner(inode, dir, mode);
  // inode->i_ino is set when host tag is allocated
  inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
  // insert_inode_hash(inode);
  switch (mode & S_IFMT) {
		case S_IFREG:
			inode->i_op = &cinq_file_inode_operations;
			inode->i_fop = &cinq_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &cinq_dir_inode_operations;
      //	inode->i_fop = &simple_dir_operations;
			/* directory inodes start off with i_nlink == 2 (for "." entry) */
			inc_nlink(inode);
			break;
		case S_IFLNK:
      //  inode->i_op = &page_symlink_inode_operations;
			break;
    default:
			DEBUG_("[Warning@cinq_new_inode] mode not matched: %s under %p.\n",
             qstr->name, dir);
			break;
  }

  mark_inode_dirty(inode);
  return inode;
}

struct inode *cnode_make_tree(struct super_block *sb) {
  struct cinq_inode *croot = cnode_new_(NULL, "/");
  
  // Construct a root inode independant of any file system
  struct inode * iroot = new_inode(sb);
  if (!iroot) {
    return ERR_PTR(-ENOMEM);
  }
  iroot->i_uid = 0;
  iroot->i_gid = 0;
  iroot->i_mode |= S_IFDIR;
  iroot->i_mtime = iroot->i_atime = iroot->i_ctime = CURRENT_TIME;
  // insert_inode_hash(inode);
  iroot->i_op = &cinq_dir_inode_operations;
  // use t_fs = 0 for root tag
  struct cinq_tag *tag = tag_new_(META_FS, croot, CINQ_OVERWR, iroot);
  HASH_ADD_PTR(croot->ci_tags, t_fs, tag);
  return iroot;
}

int cinq_mkdir(struct inode *dir, struct dentry *dentry, int mode) {
  inode_inc_link_count(dir);
  
  struct cinq_inode *parent = cnode(dir);
  struct cinq_inode *child;
  struct cinq_tag *tag;
  struct cinq_fsnode *fs = dentry->d_fsdata;
  if (!fs) {
    DEBUG_("[Error@cinq_mkdir] null fsnode for %s under dir %lx.\n",
            dentry->d_name.name, dir->i_ino);
    return -EFAULT;
  }
  
  if (inode_is_root_(dir)) {
    // registers new file system node
    struct inode *fs_inode = cinq_new_inode_(dir, mode, &dentry->d_name);
    tag = tag_new_(fs, parent, CINQ_MERGE, fs_inode);
    d_instantiate(dentry, fs_inode);
    fs->fs_root = dentry;
    write_lock(&parent->ci_tags_lock);
    HASH_ADD_PTR(parent->ci_tags, t_fs, tag);
    write_unlock(&parent->ci_tags_lock);
    return 0;
  }
  
  char *name = (char *)dentry->d_name.name;
  if (dentry->d_name.len > MAX_NAME_LEN) {
    DEBUG_("[Error@cinq_mkdir] name is too long: %s\n", name);
    return -ENAMETOOLONG;
  }
  
  write_lock(&parent->ci_children_lock);
  HASH_FIND_BY_STR(ci_child, parent->ci_children, name, child);
  
  if (child) {
    write_unlock(&parent->ci_children_lock);
    
    tag = NULL;
    write_lock(&child->ci_tags_lock);
    HASH_FIND_PTR(child->ci_tags, &fs, tag);
    if (!tag) {
      struct inode *inode = cinq_new_inode_(dir, mode, &dentry->d_name);
      tag = tag_new_(fs, child, 0, inode);
      HASH_ADD_PTR(child->ci_tags, t_fs, tag);
    }
    tag->t_mode = (mode >> CINQ_MODE_SHIFT);
    tag->t_inode->i_mode = (mode & I_MODE_FILTER);
    write_unlock(&child->ci_tags_lock);
    
  } else {
    child = cnode_new_(parent, name);
    struct inode *inode = cinq_new_inode_(dir, mode, &dentry->d_name);
    tag = tag_new_(fs, child, CINQ_MERGE, inode);
    HASH_ADD_PTR(child->ci_tags, t_fs, tag);
    HASH_ADD_BY_STR(ci_child, parent->ci_children, ci_name, child);
    write_unlock(&parent->ci_children_lock);
  }
  d_instantiate(dentry, tag->t_inode);
  return 0;
}

// @fsnode: in-out parameter
static inline struct inode *cinq_lookup_(const struct inode *dir,
                                         const char *name,
                                         struct cinq_fsnode **fs_p) {
  struct cinq_inode *parent = cnode(dir);
  if (inode_is_root_(dir)) {
    struct cinq_fsnode *fs;
    read_lock(&file_systems.lock);
    HASH_FIND_BY_STR(fs_member, file_systems.fs_table, name, fs);
    read_unlock(&file_systems.lock);
    if (!fs) return NULL;
    *fs_p = fs;
    return fs->fs_root->d_inode;
  }
  
  struct cinq_inode *child;
  read_lock(&parent->ci_children_lock);
  HASH_FIND_BY_STR(ci_child, parent->ci_children, name, child);
  read_unlock(&parent->ci_children_lock);
  if (!child) {
    DEBUG_("[Info@cinq_lookup_] no dir or file name is found: %s@%lx\n",
           name, dir->i_ino);
    return NULL;
  }
  
  struct cinq_fsnode *fs = *fs_p;
  struct cinq_tag *tag = NULL;
  read_lock(&child->ci_tags_lock);
  HASH_FIND_PTR(child->ci_tags, &fs, tag);
  while (!tag) {
    if (fsnode_is_root(fs)) {
      DEBUG_("[Info@cinq_lookup_] no file system has that dir or file:"
             "%s from %lx", name, (*fs_p)->fs_id);
      return NULL;
    }
    fs = fs->fs_parent;
    HASH_FIND_PTR(child->ci_tags, &fs, tag);
  }
  read_unlock(&child->ci_tags_lock);

  *fs_p = tag->t_fs;
  return tag->t_inode;
  
  // Since we directly read meta data in memory, there is no iget-like function.
  // No inodes are cached or added to super_block inode list.
}

struct dentry *cinq_lookup(struct inode *dir, struct dentry *dentry,
                           struct nameidata *nameidata) {
  if (dentry->d_name.len >= MAX_NAME_LEN)
    return ERR_PTR(-ENAMETOOLONG);
  
  struct cinq_fsnode *fs = nameidata ?
      nameidata->path.dentry->d_fsdata : dentry->d_fsdata;
  struct inode *inode = cinq_lookup_(dir, (const char *)dentry->d_name.name,
                                     &fs); // incorporates cinq_iget function
  dentry->d_fsdata = fs;
  if (!inode) {
    return ERR_PTR(-EIO);
  }
  return d_splice_alias(inode, dentry);
}

int cinq_create(struct inode *dir, struct dentry *dentry,
                int mode, struct nameidata *nameidata) {
  return 0;
}

int cinq_link(struct dentry *old_dentry, struct inode *dir,
              struct dentry *dentry) {
  return 0;
}

int cinq_unlink(struct inode *dir, struct dentry *dentry) {
  return 0;
}

int cinq_symlink(struct inode *dir, struct dentry *dentry,
                 const char *symname) {
  return 0;
}

int cinq_rmdir(struct inode *dir, struct dentry *dentry) {
  return 0;
}

int cinq_rename(struct inode *old_dir, struct dentry *old_dentry,
                struct inode *new_dir, struct dentry *new_dentry) {
  return 0;
}

int cinq_setattr(struct dentry *dentry, struct iattr *attr) {
  return 0;
}
