/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  cnode.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/11/12.
//

#include "cinq_meta.h"
#include "util.h"

#ifdef __KERNEL__

static struct kmem_cache *cinq_inode_cachep;

#define cnode_malloc_() \
    ((struct cinq_inode *)kmem_cache_alloc(cinq_inode_cachep, GFP_KERNEL))
#define cnode_free_(p) (kmem_cache_free(cinq_inode_cachep, p))

#else

#define cnode_malloc_() \
    ((struct cinq_inode *)malloc(sizeof(struct cinq_inode)))
#define cnode_free_(p) (free(p))

#endif // __KERNEL__

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
  atomic_set(&tag->t_count, 0);
  tag->t_symname = NULL;
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
  if (unlikely(!tag->t_inode)) {
    DEBUG_("[Warn@tag_drop_inode_] drop nonexistent one: %s\n",
           tag->t_host->ci_name);
    return;
  }
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
  struct cinq_tag *tag;
  read_lock(&cnode->ci_tags_lock);
  tag = cnode_find_tag_(cnode, fs);
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
  atomic_set(&cnode->ci_count, 0);
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
  struct cinq_inode *child;
  read_lock(&parent->ci_children_lock);
  child = cnode_find_child_(parent, name);
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

struct inode *cnode_lookup_inode(struct cinq_inode *cnode, struct cinq_fsnode *fs) {
  struct cinq_tag *tag;
  read_lock(&cnode->ci_tags_lock);
  foreach_ancestor_tag(fs, tag, cnode) {
    if (tag) {
      rd_release_return(&cnode->ci_tags_lock, tag->t_inode);
    }
  }
  read_unlock(&cnode->ci_tags_lock);
  return NULL;
}

static void cnode_evict(struct cinq_inode *cnode) {
  struct cinq_inode *parent;
  if (cnode->ci_tags || cnode->ci_children) {
    DEBUG_("[Error@cnode_evict] failed to delete cnode %lx "
           "who still has tags or children.\n", cnode->ci_id);
    return;
  }
  parent = cnode->ci_parent;
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
        DEBUG_("[Warn@cinq_new_inode] mode not matched under %s "
               "with mode %o.\n", i_cnode(dir)->ci_name, mode);
        iput(inode);
        return NULL;
    }
  }
  atomic_set(&inode->i_count, 1000000000); // prevents it from being evicted
  return inode;
}

// Contrary to convention that derives childen from parent
static void cnode_tag_ancestors_(const struct dentry *dentry) {
  struct inode *child = dentry->d_inode;
  struct cinq_fsnode *fs = dentry->d_fsdata;
  struct cinq_inode *ci_child = i_cnode(child);
  struct cinq_inode *ci_parent = ci_child->ci_parent;
  struct cinq_tag *tag;
  int to_ln_parent = S_ISDIR(child->i_mode) ? 1 : 0;
  while (!cnode_is_root_(ci_child) && ci_parent) {
    write_lock(&ci_parent->ci_tags_lock);
    tag = cnode_find_tag_(ci_parent, fs);
    if (tag) {
      inc_nchild_(tag);
      if (to_ln_parent) {
        inc_nlink(tag->t_inode);
      } else {
        to_ln_parent = 1;
      }
      write_unlock(&ci_parent->ci_tags_lock);
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
    if (to_ln_parent) {
      inc_nlink(tag->t_inode);
    } else {
      to_ln_parent = 1;
    }
    
    journal_inode(parent, CREATE);
    journal_cnode(ci_parent, UPDATE);
    
    ci_child = ci_parent;
    ci_parent = ci_child->ci_parent;
  }
}

static inline void tag_evict(struct cinq_tag *tag) {
  if (tag->t_inode) {
    inode_free_(tag->t_inode);

#ifdef CINQ_DEBUG
  atomic_dec(&num_inode_);
#endif // CINQ_DEBUG
  }
  cnode_rm_tag_syn(tag->t_host, tag);
  tag_free_(tag);
}

static inline void local_inc_ref(struct inode *dir, struct dentry *dentry) {
  struct cinq_tag *dir_tag = i_tag(dir);
  struct inode *inode = dentry->d_inode;
  struct cinq_fsnode *req_fs = dentry->d_fsdata;
  DEBUG_ON_(i_cnode(inode)->ci_parent != dir_tag->t_host,
            "[Error@local_inc_ref] not parent and child: %s not under %s\n",
            i_cnode(inode)->ci_name, dir_tag->t_host->ci_name);
  if (dir_tag->t_fs != req_fs) {
    cnode_tag_ancestors_(dentry);
  } else {
    inc_nchild_(dir_tag);
    if (S_ISDIR(inode->i_mode)) {
      inc_nlink(dir_tag->t_inode);
    }
  }
}

static inline void local_drop_ref(struct inode *dir, struct dentry *dentry) {
  struct cinq_tag *dir_tag = i_tag(dir);
  struct inode *inode = dentry->d_inode;
  struct cinq_fsnode *req_fs = dentry->d_fsdata;
  DEBUG_ON_(i_cnode(inode)->ci_parent != dir_tag->t_host,
            "[Error@local_drop_ref] not parent and child: %s not under %s\n",
            i_cnode(inode)->ci_name, dir_tag->t_host->ci_name);
  struct cinq_tag *tag;
  if (dir_tag->t_fs != req_fs) {
    tag = cnode_find_tag_syn(i_cnode(dir), req_fs);
    DEBUG_ON_(!tag, "[Error@local_drop_ref] wild tag without parent: %s(%s)\n",
              tag->t_host->ci_name, tag->t_fs->fs_name);
  } else {
    tag = dir_tag;
  }
  drop_nchild_(tag);
  if (S_ISDIR(inode->i_mode)) {
    drop_nlink(tag->t_inode);
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

// TODO
void cinq_destroy_inode(struct inode *inode) {
//  int empty_nlink = S_ISDIR(inode->i_mode) ? 2 : 1;
//
//#ifdef CINQ_DEBUG
//    int nchild = atomic_read(&i_tag(inode)->t_nchild);
//    DEBUG_ON_(inode->i_nlink - empty_nlink != nchild,
//              "[Info@cinq_destroy_inode] nlink=%d, empty_nlink=%d, nchild=%d\n",
//              inode->i_nlink, empty_nlink, nchild);
//#endif // CINQ_DEBUG
//  
//  if (inode->i_nlink == empty_nlink && !nchild) {
//    inode_free_(inode);
//    
//#ifdef CINQ_DEBUG
//    atomic_dec(&num_inode_);
//#endif // CINQ_DEBUG
//  }
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
  iroot->i_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP;
  iroot->i_mtime = iroot->i_atime = iroot->i_ctime = CURRENT_TIME;
  // insert_inode_hash(inode);
  iroot->i_op = &cinq_dir_inode_operations;
  iroot->i_fop = &cinq_dir_operations;

  struct cinq_tag *tag = tag_new_(META_FS, CINQ_INVISIBLE, iroot);
  cnode_add_tag_syn(croot, tag);
  return iroot;
}

static int cinq_mkinode_(struct inode *dir, struct dentry *dentry,
                         int mode, struct inode *inode) {
  struct cinq_inode *parent = i_cnode(dir);
  struct cinq_inode *child;
  struct cinq_tag *tag;
  struct cinq_fsnode *req_fs = dentry->d_fsdata;
  
  char *name = (char *)dentry->d_name.name;
  if (dentry->d_name.len > MAX_NAME_LEN) {
    DEBUG_("[Error@cinq_mkdir] name is too long: %s\n", name);
    return -ENAMETOOLONG;
  }
  
  if (!inode) {
    inode = cinq_get_inode_(dir, mode);
  } else {
    ihold(inode);
  }
  if (unlikely(!inode)) {
    return -ENOSPC;
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
  
  local_inc_ref(dir, dentry);
  return 0;
}

// Refer to definition comments in cinq-meta.h
int cinq_create(struct inode *dir, struct dentry *dentry,
                int mode, struct nameidata *nameidata) {
  dentry->d_fsdata = nameidata ?
      nameidata->path.dentry->d_fsdata :
      dentry->d_parent->d_fsdata;
  if (!dentry->d_fsdata) {
    DEBUG_("[Error@cinq_create] no fsnode is specified.\n");
    return -EINVAL;
  }
  return cinq_mkinode_(dir, dentry, mode | S_IFREG, NULL);
}

int cinq_symlink(struct inode *dir, struct dentry *dentry,
                 const char *symname) {
  dentry->d_fsdata = dentry->d_parent->d_fsdata;
  if (unlikely(!dentry->d_fsdata)) {
    DEBUG_("[Error@cinq_symlink] no fsnode is specified.\n");
    return -EINVAL;
  }
  
  int err = cinq_mkinode_(dir, dentry, S_IFLNK | S_IRWXUGO, NULL);
  if (!err) {
    struct inode *inode = dentry->d_inode;
    struct cinq_tag *tag = i_tag(inode);
    int size = sizeof(symname);
    tag->t_symname = (char *)malloc(size);
    if (!tag->t_symname) return -ENOSPC;
    memcpy(tag->t_symname, symname, size);
  }
  return err;
}

int cinq_mkdir(struct inode *dir, struct dentry *dentry, int mode) {
  mode |= S_IFDIR;
  if (unlikely(inode_meta_root(dir))) { // not actually make dir
    char namestr[MAX_NAME_LEN + 1];
    strncpy(namestr, (char *)dentry->d_name.name, dentry->d_name.len + 1);
    char *fsnames[2] = { NULL, NULL };
    struct cinq_fsnode *fsnodes[2] = { NULL, NULL };
    char *cur = namestr;
    char *token = strsep(&cur, FS_DELIM);
    int i;
    for (i = 0; token && i < 2; token = strsep(&cur, FS_DELIM), ++i) {
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
      DEBUG_("cinq_mkdir: new fs %s under %s", fsnames[1],
             parent_fs == META_FS ? "META_FS" : parent_fs->fs_name);
      child_fs = fsnode_new(parent_fs, fsnames[1]);
      struct inode *iroot = cinq_get_inode_(dir, mode);
      struct cinq_tag *tag = tag_new_(child_fs,
                                      mode >> CINQ_MODE_SHIFT, iroot);
      cnode_add_tag_syn(dir_cnode, tag);
      
      d_instantiate(dentry, iroot);
      child_fs->fs_root = dentry;
      dentry->d_fsdata = child_fs;
      dir->i_mtime = dir->i_ctime = CURRENT_TIME;
      
      journal_inode(iroot, CREATE);
      journal_cnode(dir_cnode, UPDATE);
      // journal_inode(dir, UPDATE);
    } else { // make inheritance
      fsnode_move(child_fs, parent_fs);
    }
    return 0;
  }
               
  dentry->d_fsdata = dentry->d_parent->d_fsdata;
  if (unlikely(!dentry->d_fsdata)) {
   DEBUG_("[Error@cinq_mkdir] null fsnode for %s under dir %lx.\n",
          dentry->d_name.name, dir->i_ino);
   return -EINVAL;
  }
  // journal_inode(dir, UPDATE);
  return cinq_mkinode_(dir, dentry, mode, NULL);
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
  return cnode_lookup_inode(child, fs);

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
  if (inode_meta_root(dir)) {
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
  
	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
  inc_nlink(inode);
  ihold(inode);
	
  dentry->d_fsdata = dentry->d_parent->d_fsdata;
  int err = cinq_tag_with_(dir, dentry, inode);
  if (!err) {
    d_instantiate(dentry, inode);
    DEBUG_ON_(S_ISDIR(dentry->d_inode->i_mode),
              "[Warn@cinq_link] link to dir.\n");
    local_inc_ref(dir, dentry);
    return 0;
  }
  
  drop_nlink(inode); // cancel inc_nlink(inode)
  iput(inode); // cancel ihold(inode)
	return err;
}

// Note that this parameter dentry should be an existing valid one,
// slightly different from the convention.
int cinq_unlink(struct inode *dir, struct dentry *dentry) {
  struct cinq_inode *dir_cnode = i_cnode(dir);
  struct cinq_inode *cnode = cnode_find_child_syn(dir_cnode,
                                                  (char *)dentry->d_name.name);
  struct inode *inode = dentry->d_inode;
  if (unlikely(!inode)) {
    DEBUG_("[Error@cinq_unlink] unlink invalid dentry without inode: %s.\n",
           dentry->d_name.name);
    return -EINVAL;
  }
  if (!dentry->d_fsdata) dentry->d_fsdata = dentry->d_parent->d_fsdata;
  struct cinq_fsnode *req_fs = dentry->d_fsdata;

  struct cinq_tag *tag;
  write_lock(&cnode->ci_tags_lock);
  tag = cnode_find_tag_(cnode, req_fs);
  if (!tag) {
    tag = tag_new_with_(req_fs, CINQ_VISIBLE, NULL);
    if (unlikely(!tag)) wr_release_return(&cnode->ci_tags_lock, -ENOSPC);
    cnode_add_tag_(cnode, tag);
  } else if (tag->t_inode) { // delete existing one
    tag_drop_inode_(tag);
    local_drop_ref(dir, dentry);
  } else {
    DEBUG_("[Warn@cinq_unlink] unlink null inode: %s(%s)\n",
           tag->t_host->ci_name, tag->t_fs->fs_name);
  }
  write_unlock(&cnode->ci_tags_lock);

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
  if (!dentry->d_fsdata) dentry->d_fsdata = dentry->d_parent->d_fsdata;
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
//  new_dentry->d_fsdata = new_dentry->d_parent->d_fsdata;
//  if (unlikely(!new_dentry->d_fsdata)) {
//    DEBUG_("[Error@cinq_rename] no fsnode is specified when moving %s\n",
//           i_cnode(old_dentry->d_inode)->ci_name);
//    return -EINVAL;
//  }
//  
//  struct inode *new_inode = new_dentry->d_inode;
//  int err;
//  if (new_inode) {
//    cinq_unlink(new_dir, new_dentry);
//  }
//  err = cinq_mkinode_(new_dir, new_dentry,
//                      old_dentry->d_inode->i_mode, old_dentry->d_inode);
//  if (!err) {
//    cinq_unlink(old_dir, old_dentry);
//  }
//  return err;
  return 0;
}


static inline int cinq_setsize_(struct inode *inode, loff_t newsize) {

	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
        S_ISLNK(inode->i_mode)))
		return -EINVAL;
	if (i_tag(inode)->t_symname)
		return -EINVAL;
  
  inode->i_size = newsize;
	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	
  journal_inode(inode, UPDATE);

	return 0;
}

int cinq_setattr(struct dentry *dentry, struct iattr *attr) {
  struct inode *inode = dentry->d_inode;
  int error;

  error = inode_change_ok(inode, attr);
  if (error)
    return error;
  
  if ((attr->ia_valid & ATTR_SIZE) && attr->ia_size != inode->i_size) {
    error = cinq_setsize_(inode, attr->ia_size);
    if (error)
      return error;
  }
  setattr_copy(inode, attr);
  
  return error;
}

#ifdef __KERNEL__

int init_cnode_cache(void) {
  cinq_inode_cachep = kmem_cache_create(
      "cinq_inode_cache", sizeof(struct cinq_inode), 0,
      (SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD), NULL);
  if (cinq_inode_cachep == NULL)
    return -ENOMEM;
  return 0;
}

void destroy_cnode_cache(void) {
  kmem_cache_destroy(cinq_inode_cachep);
}

#endif
