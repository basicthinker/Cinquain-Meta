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
#include "util.h"

static inline int inode_meta_root_(const struct inode *inode) {
  return i_tag(inode)->t_fs == META_FS;
}

static inline int cnode_is_root_(const struct cinq_inode *cnode) {
  return cnode->ci_parent == cnode;
}

// creates a new tag without associating inode to it
static inline struct cinq_tag *tag_new_with_(const struct cinq_fsnode *fs,
                                             enum cinq_visibility mode,
                                             const struct inode *inode) {
  struct cinq_tag *tag = tag_malloc_();
  if (unlikely(!tag)) {
    return NULL;
  }
  tag->t_fs = (struct cinq_fsnode *)fs;
  tag->t_inode = (void *)inode;
  tag->t_mode = mode;
  tag->t_host = NULL;
  atomic_set(&tag->t_nchild, 0);
  return tag;
}

// @inode: whose i_ino is set to address of new tag.
static inline struct cinq_tag *tag_new_(const struct cinq_fsnode *fs,
                                        enum cinq_visibility mode,
                                        struct inode *inode) {
  struct cinq_tag *tag = tag_new_with_(fs, mode, inode);
  inode->i_ino = (unsigned long) tag;
  return tag;
}

static inline void inc_nchild_(struct cinq_tag *tag) {
  atomic_inc(&tag->t_nchild);
}

static inline void drop_nchild_(struct cinq_tag *tag) {
  atomic_dec(&tag->t_nchild);
}

static inline void tag_drop_inode_(struct cinq_tag *tag) {
  iput(tag->t_inode);
  tag->t_inode = NULL;
}

static inline struct cinq_tag *cnode_find_tag_(const struct cinq_inode *cnode,
                                               const struct cinq_fsnode *fs) {
  struct cinq_tag *tag;
  HASH_FIND_PTR(cnode->ci_tags, &fs, tag);
  return tag;
}

static inline struct cinq_tag *cnode_find_tag_syn(struct cinq_inode *cnode,
                                                  struct cinq_fsnode *fs) {
  read_lock(&cnode->ci_tags_lock);
  struct cinq_tag *tag = cnode_find_tag_(cnode, fs);
  read_unlock(&cnode->ci_tags_lock);
  return tag;
}

static inline void cnode_add_tag_(struct cinq_inode *cnode,
                                  struct cinq_tag *tag) {
  HASH_ADD_PTR(cnode->ci_tags, t_fs, tag);
  tag->t_host = cnode;
}

static inline void cnode_add_tag_syn(struct cinq_inode *cnode,
                                     struct cinq_tag *tag) {
  write_lock(&cnode->ci_tags_lock);
  cnode_add_tag_(cnode, tag);
  write_unlock(&cnode->ci_tags_lock);
}

static inline void cnode_rm_tag_(struct cinq_inode *cnode,
                                 struct cinq_tag* tag) {
  HASH_DEL(cnode->ci_tags, tag);
  tag->t_host = NULL;
}

static inline void cnode_rm_tag_syn(struct cinq_inode *cnode,
                                    struct cinq_tag* tag) {
  write_lock(&cnode->ci_tags_lock);
  cnode_rm_tag_(cnode, tag);
  write_unlock(&cnode->ci_tags_lock);
}

static struct cinq_inode *cnode_new_(char *name) {
  
  struct cinq_inode *cnode = cnode_malloc_();
  if (unlikely(!cnode)) {
    DEBUG_("[Error@cnode_new] allocation fails: %s.\n", name);
    return NULL;
  }
  
  // Initializes cnode
  cnode->ci_id = (unsigned long)cnode;
  strcpy(cnode->ci_name, name);
  cnode->ci_tags = NULL;
  cnode->ci_children = NULL;
  rwlock_init(&cnode->ci_tags_lock);
  rwlock_init(&cnode->ci_children_lock);
  cnode->ci_parent = NULL;
  
  return cnode;
}

static inline struct cinq_inode *cnode_find_child_(struct cinq_inode *parent,
                                                   const char *name) {
  struct cinq_inode *child;
  HASH_FIND_BY_STR(ci_child, parent->ci_children, name, child);
  return child;
}

static inline struct cinq_inode *cnode_find_child_syn(struct cinq_inode *parent,
                                                      const char *name) {
  read_lock(&parent->ci_children_lock);
  struct cinq_inode *child = cnode_find_child_(parent, name);
  read_unlock(&parent->ci_children_lock);
  return child;
}

static inline void cnode_add_child_(struct cinq_inode *parent, struct cinq_inode *child) {
  HASH_ADD_BY_STR(ci_child, parent->ci_children, ci_name, child);
  child->ci_parent = parent;
}

static inline void cnode_add_child_syn(struct cinq_inode *parent,
                                       struct cinq_inode *child) {
  write_lock(&parent->ci_children_lock);
  cnode_add_child_(parent, child);
  write_unlock(&parent->ci_children_lock);
}

static inline void cnode_rm_child_(struct cinq_inode *parent, struct cinq_inode* child) {
  HASH_DELETE(ci_child, parent->ci_children, child);
  child->ci_parent = NULL;
}

static inline void cnode_rm_child_syn(struct cinq_inode *parent, struct cinq_inode* child) {
  write_lock(&parent->ci_children_lock);
  cnode_rm_child_(parent, child);
  write_unlock(&parent->ci_children_lock);
}

static void cnode_evict(struct cinq_inode *cnode) {
  if (cnode->ci_tags || cnode->ci_children) {
    DEBUG_("[Error@cnode_evict] failed to delete cnode %lx "
           "who still has tags or children.\n", cnode->ci_id);
    return;
  }
  struct cinq_inode *parent = cnode->ci_parent;
  if (!cnode_is_root_(cnode) && parent) {
    cnode_rm_child_syn(parent, cnode);
  }
  cnode_free_(cnode);
}

// @dir: can be containing directory when adding new inode in it,
//       or contained directory when its parents are tagged.
static struct inode *cinq_get_inode_(const struct inode *dir, int mode) {
  struct super_block *sb = dir->i_sb;
  struct inode * inode = new_inode(sb);
  if (inode) {
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
        inode->i_fop = &cinq_dir_operations;
        /* directory inodes start off with i_nlink == 2 (for "." entry) */
        inc_nlink(inode);
        break;
      case S_IFLNK:
        inode->i_op = &cinq_symlink_inode_operations;
        break;
      default:
        DEBUG_("[Warn@cinq_new_inode] mode not matched under %lx.\n",
               dir->i_ino);
        iput(inode);
        return NULL;
    }
  }
  return inode;
}

// Contrary to convention that derives childen from parent
static void cnode_tag_ancestors_(const struct inode *child,
                                 const struct cinq_fsnode *fs) {
  struct cinq_inode *ci_child = i_cnode(child);
  struct cinq_inode *ci_parent = ci_child->ci_parent;
  struct cinq_tag *tag;
  while (!cnode_is_root_(ci_child) && ci_parent) {
    write_lock(&ci_parent->ci_tags_lock);
    tag = cnode_find_tag_(ci_parent, fs);
    if (tag) {
      write_unlock(&ci_parent->ci_tags_lock);
      inc_nchild_(tag);
      break;
    }
    int mode = (child->i_mode & ~S_IFMT) | S_IFDIR;
    struct inode *parent = cinq_get_inode_(child, mode);
    if (!parent) {
      DEBUG_("[Error@cnode_tag_ancestors_] inode allocation failed "
             "when tagging ancestors of %s.\n", i_cnode(child)->ci_name);
      return;
    }
    tag = tag_new_(fs, CINQ_VISIBLE, parent);
    cnode_add_tag_(ci_parent, tag);
    write_unlock(&ci_parent->ci_tags_lock);
    inc_nchild_(tag);
    
    journal_inode(parent, CREATE);
    journal_cnode(ci_parent, UPDATE);
    
    ci_child = ci_parent;
    ci_parent = ci_child->ci_parent;
  }
}

static inline void tag_evict(struct cinq_tag *tag) {
  tag_drop_inode_(tag);
  cnode_rm_tag_syn(tag->t_host, tag);
  tag_free_(tag);
}

static inline void tag_inc_nchild(struct cinq_tag *dir_tag,
                                  struct inode *child,
                                  struct cinq_fsnode *req_fs) {
  if (unlikely(i_cnode(child)->ci_parent != dir_tag->t_host)) {
    DEBUG_("[Error@tag_inc_nchild] not parent and child: %s not under %s\n",
           i_cnode(child)->ci_name, dir_tag->t_host->ci_name);
  }
  if (dir_tag->t_fs != req_fs) {
    cnode_tag_ancestors_(child, req_fs);
  } else {
    inc_nchild_(dir_tag);
  }
}

// This function is NOT thread safe, since it is used in the end,
// and a single thread is proper then.
void cnode_evict_all(struct cinq_inode *root) {
  if (root->ci_children) {
    struct cinq_inode *cur, *tmp;
    HASH_ITER(ci_child, root->ci_children, cur, tmp) {
      cnode_evict_all(cur);
    }
    DEBUG_ON_(root->ci_children != NULL,
              "[Error@cnode_evict_all] not empty children: %lx\n", root->ci_id);
  }
  if (root->ci_tags) {
    struct cinq_tag *cur, *tmp;
    HASH_ITER(hh, root->ci_tags, cur, tmp) {
      tag_evict(cur);
    }
    DEBUG_ON_(root->ci_tags != NULL,
              "[Error@cnode_evict_all] not empty tags: %lx\n", root->ci_id);
  }
  cnode_evict(root);
}

void cinq_destroy_inode(struct inode *inode) {
  printf("i_nlink = %d, i_count = %d\n",
         inode->i_nlink, atomic_read(&inode->i_count));
  if (inode->i_nlink || atomic_read(&inode->i_count))
    return;
  inode_free_(inode);
  
#ifdef CINQ_DEBUG
  atomic_dec(&num_inode_);
#endif // CINQ_DEBUG
}

struct inode *cnode_make_tree(struct super_block *sb) {
  struct cinq_inode *croot = cnode_new_("/");
  croot->ci_parent = croot;
  
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

  struct cinq_tag *tag = tag_new_(META_FS, CINQ_INVISIBLE, iroot);
  cnode_add_tag_syn(croot, tag);
  return iroot;
}

static int cinq_mknod_(struct inode *dir, struct dentry *dentry, int mode) {
  struct cinq_inode *parent = i_cnode(dir);
  struct cinq_inode *child;
  struct cinq_tag *tag;
  struct cinq_fsnode *req_fs = dentry->d_fsdata;
  
  char *name = (char *)dentry->d_name.name;
  if (dentry->d_name.len > MAX_NAME_LEN) {
    DEBUG_("[Error@cinq_mkdir] name is too long: %s\n", name);
    return -ENAMETOOLONG;
  }
  
  write_lock(&parent->ci_children_lock);
  child = cnode_find_child_(parent, name);
  if (child) {
    write_unlock(&parent->ci_children_lock);
    
    write_lock(&child->ci_tags_lock);
    tag = cnode_find_tag_(child, req_fs);
    if (unlikely(tag)) {
      DEBUG_("[Error@cinq_mknod] mknod meets an existing one: %s\n",
             tag->t_host->ci_name);
      wr_release_return(&child->ci_tags_lock, -EINVAL);
    }
    
    struct inode *inode = cinq_get_inode_(dir, mode);
    if (!inode) wr_release_return(&child->ci_tags_lock, -ENOSPC);
    tag = tag_new_(req_fs, mode >> CINQ_MODE_SHIFT, inode);
    if (!tag) {
      iput(inode);
      wr_release_return(&child->ci_tags_lock, -ENOSPC);
    }
    cnode_add_tag_(child, tag);
    write_unlock(&child->ci_tags_lock);
    
    journal_inode(inode, CREATE);
    journal_cnode(child, UPDATE);
  } else {
    child = cnode_new_(name);
    if (unlikely(!child)) wr_release_return(&parent->ci_children_lock, -ENOSPC);
    struct inode *inode = cinq_get_inode_(dir, mode);
    if (unlikely(!inode)) wr_release_return(&parent->ci_children_lock, -ENOSPC);
    tag = tag_new_(req_fs, mode >> CINQ_MODE_SHIFT, inode);
    if (unlikely(!tag)) {
      iput(inode);
      wr_release_return(&parent->ci_children_lock, -ENOSPC);
    }
    cnode_add_tag_(child, tag);
    cnode_add_child_(parent, child);
    write_unlock(&parent->ci_children_lock);
    
    journal_inode(inode, CREATE);
    journal_cnode(child, CREATE);
    journal_cnode(parent, UPDATE);
  }
  
  d_instantiate(dentry, tag->t_inode);
  dir->i_mtime = dir->i_ctime = CURRENT_TIME;
  // journal_inode(dir, UPDATE);
  
  tag_inc_nchild(i_tag(dir), tag->t_inode, req_fs);
  return 0;
}

// Refer to definition comments in cinq-meta.h
int cinq_create(struct inode *dir, struct dentry *dentry,
                int mode, struct nameidata *nameidata) {
  if (nameidata) {
    dentry->d_fsdata = nameidata->path.dentry->d_fsdata;
  } else if (!dentry->d_fsdata) {
    DEBUG_("[Error@cinq_create] no fsnode is specified.\n");
    return -EINVAL;
  }
  return cinq_mknod_(dir, dentry, mode | S_IFREG);
}

int cinq_symlink(struct inode *dir, struct dentry *dentry,
                 const char *symname) {
  if (unlikely(!dentry->d_fsdata)) {
    DEBUG_("[Error@cinq_symlink] no fsnode is specified.\n");
    return -EINVAL;
  }
  
  int err = cinq_mknod_(dir, dentry, S_IFLNK | S_IRWXUGO);
  if (!err) {
    struct inode *inode = dentry->d_inode;
    struct cinq_tag *tag = i_tag(inode);
    int size = sizeof(symname);
    tag->t_symname = (char *)kmalloc(size);
    if (!tag->t_symname) return -ENOSPC;
    memcpy(tag->t_symname, symname, size);
  }
  return err;
}

int cinq_mkdir(struct inode *dir, struct dentry *dentry, int mode) {
  
  if (unlikely(inode_meta_root_(dir))) { // not actually make dir
    char namestr[MAX_NAME_LEN + 1];
    strncpy(namestr, (char *)dentry->d_name.name, dentry->d_name.len);
    char *fsnames[2] = { NULL, NULL };
    struct cinq_fsnode *fsnodes[2] = { NULL, NULL };
    char *cur = namestr;
    char *token = strsep(&cur, FS_DELIM);
    for (int i = 0; token && i < 2; token = strsep(&cur, FS_DELIM), ++i) {
      fsnames[i] = token;
      fsnodes[i] = cfs_find_syn(&file_systems, token);
    }
    struct cinq_fsnode *parent_fs = fsnodes[0];
    struct cinq_fsnode *child_fs = fsnodes[1];
    
    if (!fsnames[1] || (!parent_fs && strcmp(fsnames[0], "META_FS"))) {
      DEBUG_("[Error@cinq_mkdir] parent fs NOT properly specified: %s\n",
             fsnames[0]);
      return -EINVAL;
    }
    if (!parent_fs) parent_fs = META_FS;
  
    struct cinq_inode *dir_cnode = i_cnode(dir);
    if (!child_fs) { // makes new file system node
      child_fs = fsnode_new(parent_fs, fsnames[1]);
      struct inode *iroot = cinq_get_inode_(dir, mode);
      struct cinq_tag *tag = tag_new_(child_fs,
                                      mode >> CINQ_MODE_SHIFT, iroot);
      cnode_add_tag_syn(dir_cnode, tag);
      
      d_instantiate(dentry, iroot);
      child_fs->fs_root = dentry;
      dir->i_mtime = dir->i_ctime = CURRENT_TIME;
      
      journal_inode(iroot, CREATE);
      journal_cnode(dir_cnode, UPDATE);
      // journal_inode(dir, UPDATE);
    } else { // make inheritance
      fsnode_move(child_fs, parent_fs);
    }
    return 0;
  }
               
  struct cinq_fsnode *fs = dentry->d_fsdata;
  if (unlikely(!fs)) {
   DEBUG_("[Error@cinq_mkdir] null fsnode for %s under dir %lx.\n",
          dentry->d_name.name, dir->i_ino);
   return -EINVAL;
  }
  int err = cinq_mknod_(dir, dentry, mode | S_IFDIR);
  if (!err) {
    inc_nlink(dir); // for ".." entry
  }
  // journal_inode(dir, UPDATE);
  return err;
}

static inline struct inode *cinq_lookup_(const struct inode *dir,
                                         const char *name) {
  struct cinq_inode *parent = i_cnode(dir);
  struct cinq_fsnode *fs;

  struct cinq_inode *child = cnode_find_child_syn(parent, name);
  if (!child) {
    DEBUG_("[Info@cinq_lookup_] dir or file is NOT found: %s@%lx\n",
           name, dir->i_ino);
    return NULL;
  }
  
  fs = i_fs(dir);
  struct cinq_tag *tag;
  int found = 0;
  read_lock(&child->ci_tags_lock);
  foreach_ancestor_tag(fs, tag, child) {
    if (tag) {
      found = 1;
      break;
    }
  }
  read_unlock(&child->ci_tags_lock);

  return found ? tag->t_inode : NULL;
  
  // Since we directly read meta data in memory, there is no iget-like function.
  // No inodes are cached or added to super_block inode list.
}

// Refer to definition comments in cinq-meta.h
struct dentry *cinq_lookup(struct inode *dir, struct dentry *dentry,
                           struct nameidata *nameidata) {
  if (dentry->d_name.len >= MAX_NAME_LEN)
    return ERR_PTR(-ENAMETOOLONG);
  char *name = (char *)dentry->d_name.name;
  
  struct inode *inode;
  if (inode_meta_root_(dir)) {
    struct cinq_fsnode *fs = cfs_find_syn(&file_systems, name);
    if (!fs) return NULL;
    inode = fs->fs_root->d_inode;
    dentry->d_fsdata = fs;
  } else {
    inode = cinq_lookup_(dir, name);
    // pass the request ID on
    dentry->d_fsdata = nameidata ?
        nameidata->path.dentry->d_fsdata : dentry->d_parent->d_fsdata;
  }
  if (!inode) {
    return ERR_PTR(-EIO);
  }
  return d_splice_alias(inode, dentry);
}

// Finds or creates a tag specified by dir and dentry.
// Associates the tag with the specified inode.
static int cinq_tag_with_(struct inode *dir, struct dentry *dentry,
                          const struct inode *inode) {
  char *name = (char *)dentry->d_name.name;
  struct cinq_inode *dir_cnode = i_cnode(dir);
  struct cinq_tag *dir_tag = i_tag(dir);
  struct cinq_fsnode *req_fs = dentry->d_fsdata;
  struct cinq_inode *child;
  struct cinq_tag *tag;
  
  write_lock(&dir_cnode->ci_children_lock);
  child = cnode_find_child_(dir_cnode, name);
  if (child) {
    write_unlock(&dir_cnode->ci_children_lock);
    
    write_lock(&child->ci_tags_lock);
    tag = cnode_find_tag_(child, req_fs);
    if (!tag) {
      tag = tag_new_with_(req_fs, CINQ_VISIBLE, inode);
      if (unlikely(!tag)) wr_release_return(&child->ci_tags_lock, -ENOSPC);
      cnode_add_tag_(child, tag);
    } else {
      DEBUG_("[Error@cinq_add_link_] re-link existing entry: %s.\n", name);
      wr_release_return(&child->ci_tags_lock, -ENOTEMPTY);
    }
    write_unlock(&child->ci_tags_lock);
    
    journal_cnode(child, UPDATE);
  } else {
    child = cnode_new_(name);
    if (!child) wr_release_return(&dir_cnode->ci_children_lock, -ENOSPC);
    tag = tag_new_with_(req_fs, CINQ_VISIBLE, inode);
    if (unlikely(!tag)) wr_release_return(&dir_cnode->ci_children_lock, -ENOSPC);
    cnode_add_tag_(child, tag);
    cnode_add_child_(dir_cnode, child);
    write_unlock(&dir_cnode->ci_children_lock);
    
    journal_cnode(child, CREATE);
    journal_cnode(dir_cnode, UPDATE);
  }

  tag_inc_nchild(dir_tag, tag->t_inode, req_fs);
  return 0;
}

int cinq_link(struct dentry *old_dentry, struct inode *dir,
              struct dentry *dentry) {
	struct inode *inode = old_dentry->d_inode;
  if (unlikely(!inode)) {
    DEBUG_("[Error@cinq_link] link to invalid dentry without inode: %s.\n",
           old_dentry->d_name.name);
    return -EINVAL;
  }
  
  inc_nlink(inode);
	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	
  int err = cinq_tag_with_(dir, dentry, inode);
  if (!err) {
    d_instantiate(dentry, inode);
    return 0;
  }
  
  drop_nlink(inode); // cancel inc_nlink()
	return err;
}

// Note that this parameter dentry should be an existing valid one,
// slightly different from the convention.
// As convention, the request fs should be specified via dentry->d_fsdata.
int cinq_unlink(struct inode *dir, struct dentry *dentry) {
  struct inode *inode = dentry->d_inode;
  if (unlikely(!inode)) {
    DEBUG_("[Error@cinq_unlink] unlink invalid dentry without inode: %s.\n",
           dentry->d_name.name);
    return -EINVAL;
  }
  struct cinq_inode *cnode = i_cnode(inode);
  struct cinq_fsnode *req_fs = dentry->d_fsdata;

  struct cinq_tag *tag;
  write_lock(&cnode->ci_tags_lock);
  int has_parent_tag = 0;
  tag = cnode_find_tag_(cnode, req_fs);
  if (!tag) {
    tag = tag_new_with_(req_fs, CINQ_VISIBLE, NULL);
    if (unlikely(!tag)) wr_release_return(&cnode->ci_tags_lock, -ENOSPC);
    cnode_add_tag_(cnode, tag);
  } else {
    tag_drop_inode_(tag);
    has_parent_tag = 1;
  }
  write_unlock(&cnode->ci_tags_lock);
  
  if (has_parent_tag) {
    tag = cnode_find_tag_syn(i_cnode(dir), req_fs);
    if (tag) {
      drop_nchild_(tag);
    } else {
      DEBUG_("[Warn@cinq_unlink] tag should have parent: %s\n", cnode->ci_name);
    }
  }
  
  drop_nlink(inode);
  inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
  return 0;
}

static int cinq_empty_dir_(struct cinq_inode *dir_cnode,
                           struct cinq_fsnode *fs) {
  struct cinq_tag *tag;
  int num = 0;
  read_lock(&dir_cnode->ci_tags_lock);
  foreach_ancestor_tag(fs, tag, dir_cnode) {
    if (tag) {
      num = atomic_read(&tag->t_nchild);
      if (num) break;
    }
  }
  read_unlock(&dir_cnode->ci_tags_lock);
  
  return num == 0;
}

int cinq_rmdir(struct inode *dir, struct dentry *dentry) {
  struct inode *inode = dentry->d_inode;
  struct cinq_fsnode *req_fs = dentry->d_fsdata;
  
  if (!cinq_empty_dir_(i_cnode(inode), req_fs)) {
    return -ENOTEMPTY;
  }
  
  drop_nlink(inode); // delete "." entry
  int err = cinq_unlink(dir, dentry);
  if (!err) {
    drop_nlink(dir); // delete ".." entry
  }
  return err;
}

void *cinq_follow_link(struct dentry *dentry, struct nameidata *nd) {
  struct cinq_tag *tag = i_tag(dentry->d_inode);
  nd_set_link(nd, (char *)tag->t_symname);
  return NULL;
}

int cinq_rename(struct inode *old_dir, struct dentry *old_dentry,
                struct inode *new_dir, struct dentry *new_dentry) {
  if (unlikely(!new_dentry->d_fsdata)) {
    DEBUG_("[Error@cinq_rename] no fsnode is specified when moving %s\n",
           i_cnode(old_dentry->d_inode)->ci_name);
    return -EINVAL;
  }
  
  struct inode *new_inode = new_dentry->d_inode;
  int err;
  if (new_inode) {
    drop_nlink(new_inode);
    
    cinq_unlink(new_dir, new_dentry);
  } else {
    
  }
  err = cinq_tag_with_(new_dir, new_dentry, old_dentry->d_inode);
  if (!err) {
    cinq_unlink(old_dir, old_dentry);
  }
  return err;
}

int cinq_setattr(struct dentry *dentry, struct iattr *attr) {
  return 0;
}
