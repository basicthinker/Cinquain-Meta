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

static struct kmem_cache *cinq_tag_cachep;
#define tag_malloc_() \
    ((struct cinq_tag *)kmem_cache_alloc(cinq_tag_cachep, GFP_KERNEL))
#define tag_free_(p) (kmem_cache_free(cinq_tag_cachep, p))

static struct kmem_cache *vfs_inode_cachep;
#define inode_malloc_() \
    ((struct inode *)kmem_cache_alloc(vfs_inode_cachep, GFP_KERNEL))
#define inode_free_(p) (kmem_cache_free(vfs_inode_cachep, p))

/*
#else

#define cnode_malloc_() \
    ((struct cinq_inode *)malloc(sizeof(struct cinq_inode)))
#define cnode_free_(p) (free(p))

#define tag_malloc_() \
    ((struct cinq_tag *)malloc(sizeof(struct cinq_tag)))
#define tag_free_(p) (free(p))

#define inode_malloc_(void) \
    ((struct inode *)malloc(sizeof(struct inode)))
#define inode_free_(p) (free(p))
*/
#endif // __KERNEL__

static inline int cnode_is_root_(const struct cinq_inode *cnode) {
  return cnode->ci_parent == cnode;
}

// TODO Clustered on swappable pages if memory is constrained
struct inode *cinq_alloc_inode(struct super_block *sb) {
  struct inode *inode = inode_malloc_();
  INIT_LIST_HEAD(&inode->i_dentry);

#ifdef __KERNEL__
  inode_init_once(inode);
#endif
#ifdef CINQ_DEBUG
  atomic_inc(&num_inode_);
#endif // CINQ_DEBUG

  return inode;
}

// Not actually delete inodes since they are in-memory
void cinq_evict_inode(struct inode *inode) {

}

// TODO
void cinq_destroy_inode(struct inode *inode) {

}

// creates a new tag without associating inode to it
static inline struct cinq_tag *tag_new_with_(const struct cinq_fsnode *fs,
                                             struct inode *inode,
                                             enum cinq_visibility mode) {
  struct cinq_tag *tag = tag_malloc_();
  if (unlikely(!tag)) {
    return NULL;
  }
  tag->t_fs = (void *)fs;
  tag->t_inode = (void *)inode;
  if (likely(inode)) ihold(inode);
  tag->t_mode = mode;
  tag->t_host = NULL;
  atomic_set(&tag->t_nchild, 0);
  atomic_set(&tag->t_count, 0);
  tag->t_symname = NULL;
  return tag;
}

// @inode: whose i_ino is set to address of new tag.
static inline struct cinq_tag *tag_new_(const struct cinq_fsnode *fs,
                                        struct inode *inode,
                                        enum cinq_visibility mode) {
  struct cinq_tag *tag = tag_new_with_(fs, inode, mode);
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

static inline void tag_reset_inode_(struct cinq_tag *tag, struct inode *inode) {
  tag_drop_inode_(tag);
  ihold(inode);
  tag->t_inode = inode;
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
  // tag->t_host = NULL;
}

static inline void cnode_rm_tag_syn(struct cinq_inode *cnode,
                                    struct cinq_tag* tag) {
  write_lock(&cnode->ci_tags_lock);
  cnode_rm_tag_(cnode, tag);
  write_unlock(&cnode->ci_tags_lock);
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

struct inode *cnode_lookup_inode(struct cinq_inode *cnode, struct cinq_fsnode *req_fs) {
  struct cinq_tag *tag;
  struct cinq_fsnode *fs = req_fs;
  read_lock(&cnode->ci_tags_lock);
  foreach_ancestor_tag(fs, tag, cnode) {
    if (tag) {
      rd_release_return(&cnode->ci_tags_lock, tag->t_inode);
    }
  }
  read_unlock(&cnode->ci_tags_lock);
  DEBUG_("cnode_lookup_inode: failed to find tag of FS '%s' on %s.\n",
         req_fs->fs_name, cnode->ci_name);
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

// @dir: can be containing directory when adding new inode in it,
//       or contained directory when its parents are being tagged.
static struct inode *cinq_get_inode_(const struct inode *dir, int mode,
                                     dev_t dev) {
  struct super_block *sb = dir->i_sb;
  struct inode * inode = new_inode(sb);
  
  if (inode) {
#ifdef __KERNEL__
    inode->i_generation = get_seconds();
    inode->i_mapping->backing_dev_info = &cinq_backing_dev_info;
#endif

    inode_init_owner(inode, dir, mode);
    // inode->i_ino is set when host tag is allocated
    inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
    // insert_inode_hash(inode);
    switch (mode & S_IFMT) {
      default:
        init_special_inode(inode, mode, dev);
        break;
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
    
    struct inode *parent = cinq_get_inode_(child,
    		(child->i_mode & ~S_IFMT) | S_IFDIR, 0);
    if (!parent) {
      DEBUG_("[Error@cnode_tag_ancestors_] inode allocation failed "
             "when tagging ancestors of %s.\n", i_cnode(child)->ci_name);
      write_unlock(&ci_parent->ci_tags_lock);
      return;
    }
    tag = tag_new_(fs, parent, CINQ_VISIBLE);
    cnode_add_tag_(ci_parent, tag);
    write_unlock(&ci_parent->ci_tags_lock);
    
    inc_nchild_(tag);
    if (to_ln_parent) {
      inc_nlink(tag->t_inode);
    } else {
      to_ln_parent = 1;
    }
    
    ci_child = ci_parent;
    ci_parent = ci_child->ci_parent;
  }
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

// Reaching ci_tags_lock of parent cnode
static inline void local_drop_ref(struct inode *dir, struct dentry *dentry) {
  struct cinq_tag *dir_tag = i_tag(dir), *tag;
  struct inode *inode = dentry->d_inode;
  struct cinq_fsnode *req_fs = dentry->d_fsdata;
  DEBUG_ON_(i_cnode(inode)->ci_parent != dir_tag->t_host,
            "[Error@local_drop_ref] not parent and child cnodes: %s and %s\n",
            i_cnode(inode)->ci_name, dir_tag->t_host->ci_name);
  if (dir_tag->t_fs != req_fs) {
    tag = cnode_find_tag_syn(i_cnode(dir), req_fs); // reflects latest update
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

struct inode *cnode_make_tree(struct super_block *sb) {
  int mode = S_IFDIR | S_IRWXUGO | S_ISVTX;
  struct cinq_inode *croot = cnode_new_("/");
  croot->ci_parent = croot;
  
  // Construct a root inode independant of any file system
  struct inode * iroot = new_inode(sb);
  if (!iroot) {
    return ERR_PTR(-ENOMEM);
  }
  inode_init_owner(iroot, NULL, mode);
  iroot->i_blocks = 0;
  iroot->i_uid = current_fsuid();
  iroot->i_gid = current_fsgid();
  iroot->i_mtime = iroot->i_atime = iroot->i_ctime = CURRENT_TIME;
#ifdef __KERNEL__
  iroot->i_generation = get_seconds();
  iroot->i_mapping->backing_dev_info = &cinq_backing_dev_info;
#endif
  // insert_inode_hash(inode);
  inc_nlink(iroot); // as dir
  iroot->i_op = &cinq_dir_inode_operations;
  iroot->i_fop = &cinq_dir_operations;

  struct cinq_tag *tag = tag_new_(META_FS, iroot, CINQ_INVISIBLE);
  cnode_add_tag_syn(croot, tag);
  return iroot;
}

static int cinq_mkinode_(struct inode *dir, struct dentry *dentry,
                         int mode, dev_t dev) {
  struct cinq_inode *parent = i_cnode(dir);
  struct cinq_inode *child;
  struct inode *inode;
  struct cinq_tag *tag;
  struct cinq_fsnode *req_fs = dentry->d_fsdata;
  
  char *name = (char *)dentry->d_name.name;
  if (unlikely(dentry->d_name.len > MAX_NAME_LEN)) {
    DEBUG_("[Error@cinq_mkdir] name is too long: %s\n", name);
    return -ENAMETOOLONG;
  }
  
  inode = cinq_get_inode_(dir, mode, dev);
  if (unlikely(!inode)) {
    return -ENOSPC;
  }
  tag = tag_new_(req_fs, inode, mode >> CINQ_MODE_SHIFT);
  if (unlikely(!tag)) {
    iput(inode);
    return -ENOSPC;
  }
  
  write_lock(&parent->ci_children_lock);
  child = cnode_find_child_(parent, name);
  if (child) {
	struct cinq_tag *old_tag;
    write_unlock(&parent->ci_children_lock);
    DEBUG_(">>> cinq_mkinode_(1): tag existing cnode %s with %s.\n",
           child->ci_name, req_fs->fs_name);
    
    write_lock(&child->ci_tags_lock);
    old_tag = cnode_find_tag_(child, req_fs);
    if (unlikely(old_tag)) {
      if (negative(old_tag)) cnode_rm_tag_(child, old_tag);
      else {
        DEBUG_("[Error@cinq_mkinode_] cinq_mkinode_ meets existing '%s'.\n",
               old_tag->t_host->ci_name);
        wr_release_return(&child->ci_tags_lock, -EINVAL);
      }
    }
    cnode_add_tag_(child, tag);
    write_unlock(&child->ci_tags_lock);
  } else {
    child = cnode_new_(name);
    if (unlikely(!child)) wr_release_return(&parent->ci_children_lock, -ENOSPC);
    cnode_add_tag_(child, tag);
    cnode_add_child_(parent, child);
    write_unlock(&parent->ci_children_lock);
    DEBUG_(">>> cinq_mkinode_(2): create %s under cnode %s by FS %s.\n",
           child->ci_name, parent->ci_name, req_fs->fs_name);
  }
  
  d_instantiate(dentry, tag->t_inode);
  // to pin the dentry in core
  if (dentry) {
    spin_lock(&dentry->d_lock);
    dentry->d_count = 0x7fffffff;
    spin_unlock(&dentry->d_lock);
  }
  dir->i_mtime = dir->i_ctime = CURRENT_TIME;
  
  local_inc_ref(dir, dentry);
  return 0;
}

// Refer to definition comments in cinq_meta.h
int cinq_create(struct inode *dir, struct dentry *dentry,
                int mode, struct nameidata *nameidata) {
  dentry->d_fsdata = nameidata ?
      nameidata->path.dentry->d_fsdata :
      dentry->d_parent->d_fsdata;
  if (!dentry->d_fsdata) {
    DEBUG_("[Error@cinq_create] no fsnode is specified.\n");
    return -EINVAL;
  }
  return cinq_mkinode_(dir, dentry, mode | S_IFREG, 0);
}

int cinq_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t dev) {
  dentry->d_fsdata = dentry->d_parent->d_fsdata;
  if (unlikely(!dentry->d_fsdata)) {
    DEBUG_("[Error@cinq_mknod] null fsnode for %s under dir %lx.\n",
           dentry->d_name.name, dir->i_ino);
    return -EINVAL;
  }
  return cinq_mkinode_(dir, dentry, mode, dev);
}

int cinq_symlink(struct inode *dir, struct dentry *dentry,
                 const char *symname) {
  dentry->d_fsdata = dentry->d_parent->d_fsdata;
  if (unlikely(!dentry->d_fsdata)) {
    DEBUG_("[Error@cinq_symlink] no fsnode is specified.\n");
    return -EINVAL;
  }
  
  int err = cinq_mkinode_(dir, dentry, S_IFLNK | S_IRWXUGO, 0);
  if (!err) {
    struct inode *inode = dentry->d_inode;
    struct cinq_tag *tag = i_tag(inode);
    int len = strlen(symname);
    tag->t_symname = (char *)malloc(len + 1);
    if (!tag->t_symname) return -ENOSPC;
    strncpy(tag->t_symname, symname, len + 1);
    DEBUG_("cinq_symlink: symlink to '%s'.\n", tag->t_symname);
  }
  return err;
}

int cinq_mkdir(struct inode *dir, struct dentry *dentry, int mode) {
  mode |= S_IFDIR;
  if (unlikely(inode_meta_root(dir))) { // not actually make dir
	struct cinq_inode *dir_cnode;
	struct cinq_fsnode *parent_fs, *child_fs;
    char namestr[MAX_NAME_LEN + 1];
    char *delim_pos;
    char *parent_name, *child_name;
    struct qstr *d_name;

    strncpy(namestr, (char *)dentry->d_name.name, dentry->d_name.len + 1);
    delim_pos = memchr(namestr, FS_DELIM, MAX_NAME_LEN + 1);
    if (unlikely(!delim_pos)) {
      DEBUG_("[Error@cinq_mkdir] FS view names NOT properly specified: %s\n",
             namestr);
      return -EINVAL;
    }
    *delim_pos = '\0';
    parent_name = namestr;
    child_name = delim_pos + 1;
    // FIX ME: deallocate old d_name.name
    d_name = &dentry->d_name;
    d_name->len = strlen(child_name);
    d_name->name = malloc(d_name->len + 1);
    strncpy((char *)d_name->name, child_name, d_name->len + 1);
    d_name->hash = full_name_hash(d_name->name, d_name->len);
    
    parent_fs = cfs_find_syn(&file_systems, parent_name);
    child_fs = cfs_find_syn(&file_systems, child_name);
    
    if (unlikely(!parent_fs && strcmp(parent_name, "META_FS"))) {
      DEBUG_("[Error@cinq_mkdir] parent fs NOT properly specified: %s\n",
             parent_name);
      return -EINVAL;
    }
    if (!parent_fs) parent_fs = META_FS;
  
    dir_cnode = i_cnode(dir);
    if (!child_fs) { // makes new file system node
      struct cinq_tag *tag;
      struct inode *iroot = cinq_get_inode_(dir, mode, 0);
      if (unlikely(!iroot)) {
        DEBUG_("[Error@cinq_mkdir] failed to allocate root inode for FS view %s.\n",
               child_fs->fs_name);
        return -ENOSPC;
      }
      child_fs = fsnode_new(parent_fs, child_name);
      tag = tag_new_(child_fs, iroot, mode >> CINQ_MODE_SHIFT);
      if (unlikely(!tag)) {
        DEBUG_("[Error@cinq_mkdir] failed to allocate root tag for FS view %s.\n",
               child_fs->fs_name);
        return -ENOSPC;
      }
      cnode_add_tag_syn(dir_cnode, tag);
      
      d_instantiate(dentry, iroot);
      dget(dentry); // extra count to pin the dentry in core
      child_fs->fs_root = dentry;
      dentry->d_fsdata = child_fs; // source of request ID (1)
      dir->i_mtime = dir->i_ctime = CURRENT_TIME;
      
      DEBUG_("<<< cinq_mkdir: created root dentry %s(%p) with root inode %lx "
    		 "for FS view %s under %s.\n",
             dentry->d_name.name, dentry, dentry->d_inode->i_ino,
             child_name, parent_fs == META_FS ? "META_FS" : parent_fs->fs_name);
    } else { // make inheritance
      DEBUG_(">>> cinq_mkdir: move FS view %s to %s.\n", child_fs->fs_name,
             parent_fs == META_FS ? "META_FS" : parent_fs->fs_name);
      if (child_fs->fs_parent != parent_fs) {
        fsnode_move(child_fs, parent_fs);
      }
    }
    return 0;
  }
               
  dentry->d_fsdata = dentry->d_parent->d_fsdata; // source of request ID (2)
  if (unlikely(!dentry->d_fsdata)) {
   DEBUG_("[Error@cinq_mkdir] null fsnode for %s under dir %lx.\n",
          dentry->d_name.name, dir->i_ino);
   return -EINVAL;
  }

  DEBUG_("<<< cinq_mkdir: new dir %s under inode %lx on cnode %s by %s.\n",
         dentry->d_name.name, dir->i_ino, i_cnode(dir)->ci_name,
         ((struct cinq_fsnode *)dentry->d_fsdata)->fs_name);
  return cinq_mkinode_(dir, dentry, mode, 0);
}

// Looking up a directory or file
static inline struct inode *cinq_lookup_(const struct inode *dir,
                                         const char *name) {
  struct cinq_inode *parent = i_cnode(dir);
  struct cinq_fsnode *fs;

  struct cinq_inode *child = cnode_find_child_syn(parent, name);
  if (unlikely(!child)) {
    //  DEBUG_("[Info@cinq_lookup_] cnode is NOT found: %s under cnode %s\n",
    //         name, parent->ci_name);
    return NULL;
  }
  
  fs = i_fs(dir);
  if (unlikely(!fs)) {
    DEBUG_("[Error@cinq_lookup_] fs is NOT found for inode at %p.\n", dir);
    return NULL;
  }
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
    DEBUG_(">>> cinq_lookup(1): to look up FS view %s.\n", name);
    struct cinq_fsnode *fs = cfs_find_syn(&file_systems, name);
    if (!fs) return NULL;
    inode = fs->fs_root->d_inode;
    dentry->d_fsdata = fs;
  } else {
    struct cinq_inode *cnode = i_cnode(dir);
    struct cinq_tag *tag;
	// pass the request ID on
	dentry->d_fsdata = nameidata ?
	    nameidata->path.dentry->d_fsdata : dentry->d_parent->d_fsdata;
    // Check request FS to prevent overlooking its recent updates
	tag = cnode_find_tag_syn(cnode, dentry->d_fsdata);
	if (tag) {
      if (negative(tag)) return NULL; // the parent is removed
      dir = tag->t_inode; // change to the view of request FS
	}
    DEBUG_(">>> cinq_lookup(2): to look up %s by FS %s under inode %lx on cnode %s.\n",
           name, ((struct cinq_fsnode *)dentry->d_fsdata)->fs_name,
           dir->i_ino, i_cnode(dir)->ci_name);
    inode = cinq_lookup_(dir, name);
  }

  if (!inode) {
    DEBUG_("<<< cinq_lookup: FAILED to locate %s by FS %s under inode %lx on cnode %s.\n",
           dentry->d_name.name, ((struct cinq_fsnode *)dentry->d_fsdata)->fs_name,
           dir->i_ino, i_cnode(dir)->ci_name);
    return NULL;
  }
  return d_splice_alias(inode, dentry);
}

// Finds or creates a tag specified by dir and dentry.
// Associates the tag with the specified inode.
static int cinq_tag_with_(struct inode *dir, struct dentry *dentry,
                          struct inode *inode) {
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
      tag = tag_new_with_(req_fs, inode, CINQ_VISIBLE);
      if (unlikely(!tag)) wr_release_return(&child->ci_tags_lock, -ENOSPC);
      cnode_add_tag_(child, tag);
    } else {
      DEBUG_("[Warn@cinq_tag_with_] re-link existing entry: %s.\n", name);
      tag_reset_inode_(tag, inode);
    }
    write_unlock(&child->ci_tags_lock);
  } else {
    child = cnode_new_(name);
    if (!child) wr_release_return(&dir_cnode->ci_children_lock, -ENOSPC);
    tag = tag_new_with_(req_fs, inode, CINQ_VISIBLE);
    if (unlikely(!tag)) wr_release_return(&dir_cnode->ci_children_lock, -ENOSPC);
    cnode_add_tag_(child, tag);
    cnode_add_child_(dir_cnode, child);
    write_unlock(&dir_cnode->ci_children_lock);
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
    dget(dentry); // extra count to pin the dentry in core
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
  struct inode *inode = dentry->d_inode;
  struct cinq_inode *dir_cnode = i_cnode(dir);
  struct cinq_inode *cnode = cnode_find_child_syn(dir_cnode, dentry->d_name.name);
  struct cinq_tag *tag;

  DEBUG_("cinq_unlink: to delete %s located on cnode %s\n",
		  dentry->d_name.name, i_cnode(inode) ? i_cnode(inode)->ci_name : NULL);

  if (unlikely(!inode || !cnode)) {
    printk(KERN_ERR "[Error@cinq_unlink] unlink invalid dentry %s "
        "without inode (%p) or cnode (%p).\n",
        dentry->d_name.name, inode, cnode);
    return -EINVAL;
  }

  if (!dentry->d_fsdata) dentry->d_fsdata = dentry->d_parent->d_fsdata;

  write_lock(&cnode->ci_tags_lock);
  tag = cnode_find_tag_(cnode, dentry->d_fsdata);
  if (!tag) {
    tag = tag_new_with_(dentry->d_fsdata, NULL, CINQ_VISIBLE);
    if (unlikely(!tag)) wr_release_return(&cnode->ci_tags_lock, -ENOSPC);
    cnode_add_tag_(cnode, tag);
  } else if (tag->t_inode) { // delete existing one
    tag_drop_inode_(tag);
    // locking order: chld->ci_tags_lock ==> parent->ci_tags_lock
    local_drop_ref(dir, dentry);
  } else {
    DEBUG_("[Warn@cinq_unlink] unlink null tag on %s by FS %s\n",
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
  
  struct cinq_inode *cnode = cnode_find_child_syn(i_cnode(dir), dentry->d_name.name);
  if (!cinq_empty_dir_(cnode, req_fs)) {
    return -ENOTEMPTY;
  }
  
  if (i_fs(inode) == req_fs) drop_nlink(inode); // delete "." entry
  int err = cinq_unlink(dir, dentry);
  if (!err && i_fs(inode) == req_fs) {
    drop_nlink(dir); // delete ".." entry
  }
  return err;
}

void *cinq_follow_link(struct dentry *dentry, struct nameidata *nd) {
  struct cinq_tag *tag = i_tag(dentry->d_inode);
  DEBUG_("cinq_follow_link: follow link to '%s'.\n", (char *)tag->t_symname);
  nd_set_link(nd, (char *)tag->t_symname);
  return NULL;
}

int cinq_rename(struct inode *old_dir, struct dentry *old_dentry,
                struct inode *new_dir, struct dentry *new_dentry) {
  struct inode *new_inode = new_dentry->d_inode;
  struct inode *old_inode = old_dentry->d_inode;
  struct cinq_tag *new_tag;
  struct cinq_fsnode *req_fs = new_dentry->d_fsdata = old_dentry->d_fsdata;
  if (unlikely(!old_dentry->d_fsdata)) {
    DEBUG_("[Error@cinq_rename] no fsnode is specified when moving %s\n",
           i_cnode(old_dentry->d_inode)->ci_name);
    return -EINVAL;
  }

  DEBUG_("cinq_rename: dentry %s under cnode %p ==> dentry %s under cnode %p\n",
		  old_dentry->d_name.name, i_cnode(old_dir),
		  new_dentry->d_name.name, i_cnode(new_dir));

  if (new_inode && (new_tag = i_tag(new_inode), new_tag->t_fs == req_fs)) {
    if (S_ISDIR(new_inode->i_mode) && !cinq_empty_dir_(i_cnode(new_inode), req_fs)) {
      DEBUG_("cinq_rename: move to non-empty dir %lx on cnode %s\n",
          new_inode->i_ino, i_cnode(new_inode)->ci_name);
      return -ENOTEMPTY;
    }
    tag_reset_inode_(new_tag, old_inode);
  } else {
	cinq_tag_with_(new_dir, new_dentry, old_inode);
  }

  cinq_unlink(old_dir, old_dentry);
  return 0;
}

static inline int cinq_setsize_(struct inode *inode, loff_t newsize) {

  if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
      S_ISLNK(inode->i_mode)))
    return -EINVAL;
  if (i_tag(inode)->t_symname)
    return -EINVAL;
  
  inode->i_size = newsize;
  inode->i_mtime = inode->i_ctime = CURRENT_TIME;

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
  
  DEBUG_("cinq_setattr: set %s(%s) size %lld.\n",
         i_cnode(inode)->ci_name,
         i_tag(inode)->t_fs->fs_name, inode->i_size);
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

int init_tag_cache(void) {
  cinq_tag_cachep = kmem_cache_create(
      "cinq_tag_cache", sizeof(struct cinq_tag), 0,
      (SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD), NULL);
  if (cinq_tag_cachep == NULL)
    return -ENOMEM;
  return 0;
}

void destroy_tag_cache(void) {
  kmem_cache_destroy(cinq_tag_cachep);
}

int init_inode_cache(void) {
  vfs_inode_cachep = kmem_cache_create(
      "vfs_inode_cache", sizeof(struct inode), 0,
      (SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD), NULL);
  if (vfs_inode_cachep == NULL)
    return -ENOMEM;
  return 0;
}

void destroy_inode_cache(void) {
  kmem_cache_destroy(vfs_inode_cachep);
}

#endif
