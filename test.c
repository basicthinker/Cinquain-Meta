/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  test.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/9/12.
//

#include <stdio.h>

#include "cinq_meta.h"

/* Test config */
#define FS_CHILDREN_ 4
#define CNODE_CHILDREN_ 6 // should be larger than FS_CHILDREN_

#define NUM_LOOKUP_ 100
#define LOOKUP_THR_NUM_ 8 // number of threads for rand_lookup
#define NUM_CREATE_ 50
#define CREATE_THR_NUM_ 8 // number of threads for rand_creat_ln_rm


static void print_fs_tree_(const int depth, const int no,
                           struct cinq_fsnode *root) {
  int i;
  for (i = 0; i < depth; ++i) {
    fprintf(stdout, "  ");
  }
  // check global list
  struct cinq_fsnode *fsnode = cfs_find_syn(&file_systems, root->fs_name);
  if (fsnode != root) {
    fprintf(stderr, "Error locating fsnode %lx: %p != %p\n",
            root->fs_id, fsnode, root);
  } else {
    fprintf(stdout, "%d. ID=%lx name=%s\n",
            no, root->fs_id, root->fs_name);
  }
  
  i = 0;
  struct cinq_fsnode *p;
  for (p = root->fs_children; p != NULL; p = p->fs_child.next) {
    print_fs_tree_(depth + 1, ++i, p);
  }
}

// This function is only for test purpose.
// User space never touches data structures in this function.
static void print_dir_tree_(const int depth, const int no,
                            struct cinq_inode *root) {
  int i;
  for (i = 0; i < depth; ++i) {
    fprintf(stdout, "  ");
  }
  fprintf(stdout, "%d. ID=%lx name=%s with", no, root->ci_id, root->ci_name);
  struct cinq_tag *cur, *tmp;
  HASH_ITER(hh, root->ci_tags, cur, tmp) {
    if (cur->t_fs == META_FS) {
      fprintf(stdout, " META_FS");
    } else {
      fprintf(stdout, " %s ", cur->t_fs->fs_name);
    }
  }
  fprintf(stdout, "\n");
  i = 0;
  struct cinq_inode *p;
  for (p = root->ci_children; p != NULL; p = p->ci_child.next) {
    print_dir_tree_(depth + 1, ++i, p);
  }
}

static inline void print_fs_tree(struct cinq_fsnode *root) {
  print_fs_tree_(0, 1, root);
}

static inline void print_dir_tree(struct cinq_inode *root) {
  print_dir_tree_(0, 1, root);
}

// Includes examples for file system registration
static void make_fs_tree(struct dentry *droot) {
  struct inode *iroot = droot->d_inode;
  int mode = S_IFDIR | S_IRWXO | S_IRGRP;
  struct dentry *dent;

  // make file-system tree by invoking cinq_mkdir
  // Example for file system registration via cinq_mkdir().
  // e.g. mkdir [parent_fs_name].[child_fs_name]
  char sub[MAX_NAME_LEN + 1];
  sprintf(sub, "META_FS.0_0_0"); // use META_FS to register root fsnode
  struct qstr dname = { .name = (unsigned char *)sub, .len = strlen(sub) };
  dent = d_alloc(droot, &dname);
  iroot->i_op->mkdir(iroot, dent, mode);
  
  int i, j;
  for (i = 1; i <= FS_CHILDREN_; ++i) {
    sprintf(sub, "0_0_0.0_%x_0", i); // unique name and relation
    struct qstr dname = { .name = (unsigned char *)sub, .len = strlen(sub) };
    dent = d_alloc(droot, &dname);
    iroot->i_op->mkdir(iroot, dent, mode);
    
    for (j = 1; j <= FS_CHILDREN_; ++j) {
      char subsub[MAX_NAME_LEN + 1];
      sprintf(subsub, "0_%x_0.0_%x_%x", i, i, j); // unique name and relation
      struct qstr dname = { .name = (unsigned char *)subsub,
                            .len = strlen(subsub) };
      dent = d_alloc(droot, &dname);
      iroot->i_op->mkdir(iroot, dent, mode);
    }
  }
}

// Includes example for invoking cinq_mkdir
static void *make_dir_tree(void *fsnode) {
  struct cinq_fsnode *fs = (struct cinq_fsnode *)fsnode;
  struct dentry *root = fs->fs_root;
  int i, j, k;
  int mi, mj, mk;
  sscanf(fs->fs_name, "%x_%x_%x", &mi, &mj, &mk);
  char buffer[MAX_NAME_LEN + 1];
  int mode = (CINQ_VISIBLE << CINQ_MODE_SHIFT) | S_IFDIR | S_IRWXO | S_IRGRP;
  
  // make three-layer directory tree for test
  for (i = mi; i < CNODE_CHILDREN_; ++i) {
    // Example for invoking cinq_mkdir
    // (1) prepare parameters
    struct inode *dir = root->d_inode; // containing dir
    sprintf(buffer, "%x", i);
    struct qstr dname =                // new dir name passed via dentry
        { .name = (unsigned char *)buffer, .len = strlen(buffer) };
    struct dentry *den = d_alloc(root, &dname);

    // (2) invoke cinq_mkdir
    if (dir->i_op->mkdir(dir, den, mode)) {
      DEBUG_("[Error@make_dir_tree] failed to make dir '%s' by fs %lx(%s).\n",
             buffer, fs->fs_id, fs->fs_name);
      pthread_exit(NULL);
    } // end of example
    
    for (j = mj; j < CNODE_CHILDREN_; ++j) {
      struct inode *subdir = den->d_inode;
      sprintf(buffer, "%x.%x", i, j);
      struct qstr dname =
          { .name = (unsigned char *)buffer, .len = strlen(buffer) };
      struct dentry *subden = d_alloc(den, &dname);
      if (subdir->i_op->mkdir(subdir, subden, mode)) {
        DEBUG_("[Error@make_dir_tree] failed to make sub-dir '%s'"
               " by fs %lx(%s).\n", buffer, fs->fs_id, fs->fs_name);
        pthread_exit(NULL);
      }
      
      for (k = mk; k < CNODE_CHILDREN_; ++k) {
        struct inode *subsubdir = subden->d_inode;
        sprintf(buffer, "%x.%x.%x", i, j, k);
        struct qstr dname = 
            { .name = (unsigned char *)buffer, .len = strlen(buffer) };
        struct dentry *subsubden = d_alloc(subden, &dname);
        if (subsubdir->i_op->mkdir(subsubdir, subsubden, mode)) {
          DEBUG_("[Error@make_dir_tree] failed to make sub-sub-dir '%s' "
                 "by fs %lx(%s).\n", buffer, fs->fs_id, fs->fs_name);
          pthread_exit(NULL);
        }
      }
    }
  }
  pthread_exit(NULL);
}

// Example for invoking cinq_lookup
static struct dentry *do_lookup_(struct dentry *droot,
                                 char seg[][MAX_NAME_LEN + 1],
                                 const int num) {
  struct dentry *den = droot;           // (1) start from super_block.s_root
  struct inode *inode = den->d_inode;   //     and corresponding inode
  int i;
  for (i = 0; i < num; ++i) {       // (2) for each segment in the path
    
    // (3) prepare parameters
    struct qstr dname =
        { .name = (unsigned char *)seg[i], .len = strlen(seg[i]) };
    struct dentry *subden = d_alloc(den, &dname);
    
    // (4) invoke cinq_lookup
    inode->i_op->lookup(inode, subden, NULL);
    
    // (5) retrieve lookup result
    if (!subden->d_inode) { // when target path is not found
      return NULL;
    }
    // (6) prepare for next segment
    den = subden;
    inode = den->d_inode;
  } // continue next segment

  return den;
}

static spinlock_t lookup_num_lock_;
static int lookup_ok_cnt = 0;

static void *rand_lookup(void *droot) {
  // randomly choose client file system
  char fs_name[MAX_NAME_LEN + 1];
  const int fs_j = rand() % FS_CHILDREN_ + 1;
  const int fs_k = rand() % (FS_CHILDREN_ + 1);
  sprintf(fs_name, "0_%x_%x", fs_j, fs_k);
  
  const int k_num_seg = 4;
  char dir[k_num_seg][MAX_NAME_LEN + 1];
  int pass;
  int num_ok = 0;
  int i;
  for (i = 0; i < NUM_LOOKUP_; ++i) {
    // manually fills dir segments
    // which should be parsed from path in practice
    const int dir_i = rand() % (CNODE_CHILDREN_ + 1);
    const int dir_j = rand() % (CNODE_CHILDREN_ + 1);
    const int dir_k = rand() % (CNODE_CHILDREN_ + 1);
    // lookup path "/fs_name/dir[1]/dir[2]/dir[3]"
    sprintf(dir[0], "%s", fs_name); // the first segment is special
    sprintf(dir[1], "%x", dir_i);   // the rest are normal ones under root
    sprintf(dir[2], "%s.%x", dir[1], dir_j);
    sprintf(dir[3], "%s.%x", dir[2], dir_k);
    
    // Omits dcache lookup.
    // Dentry cache should be firstly used in practice.
    
    struct dentry *found_dent = do_lookup_(droot, (void *)dir, k_num_seg);
    if (!found_dent) { // when target path is not found
      if (dir_i >= CNODE_CHILDREN_ || dir_j >= CNODE_CHILDREN_ ||
          dir_k >= CNODE_CHILDREN_) { // when it should be not-found
        pass = 1;
      } else {
        pass = 0;
      }
    } else { // successful lookup
      pass = 0;
      char *cur_fs_name = i_fs(found_dent->d_inode)->fs_name;
      if (strcmp(fs_name, cur_fs_name)) { // not identical
        // when ancestor is used
        if (fs_j > dir_j) { // root file system should be used
          if (strcmp(cur_fs_name, "0_0_0") == 0) {
            pass = 1;
          }
        } else if (fs_k > dir_k) { // second layer file system shoud be used
          int cur_j, cur_k;
          sscanf(cur_fs_name, "0_%x_%x", &cur_j, &cur_k);
          if (cur_j == fs_j && cur_k == 0) {
            pass = 1;
          }
        }
      } else if (fs_j <= dir_j && fs_k <= dir_k) {
        pass = 1; // when file system not changed
      }
    }
    fprintf(stdout, "%s finds %s\t->\t%s\t%s\n",
            fs_name, dir[k_num_seg - 1], 
            found_dent ? i_fs(found_dent->d_inode)->fs_name : "-",
            pass ? "OK" : "WRONG");
    if (pass) ++num_ok;
  } // for
  spin_lock(&lookup_num_lock_);
  lookup_ok_cnt += num_ok;
  spin_unlock(&lookup_num_lock_);
  pthread_exit(NULL);
}


atomic_t readdir_is_ok = { 1 };

struct some_entry {
  char name[MAX_NAME_LEN + 1];
  int size;
  struct list_head member;
};

/* Example for using filldir */
static int example_filldir(void *dirent, const char *name, int name_len,
                           loff_t pos, u64 ino, unsigned dt_type) {
  // User-defined way to use dirent
  struct list_head *list = (struct list_head *)dirent;
  // The way to retrieve inode
  struct inode *inode = cinq_iget(NULL, ino);
  
  struct some_entry *cur =
      (struct some_entry *)malloc(sizeof(struct some_entry));
  strcpy(cur->name, name);
  cur->size = inode->i_size;
  list_add(&cur->member, list);
  
  // the rest part is only for test purpose
  if (strcmp(name, ".") && strcmp(name, "..") &&
      strcmp(name, i_cnode(inode)->ci_name)) {
    DEBUG_("[Error@example_filldir] extracted inode not matched for %s.\n",
           name);
    atomic_set(&readdir_is_ok, 0);
  }
  return 0;
}

static void do_ls_(struct dentry *dent) {
  
  /* Example for invoking cinq_readdir() */
  LIST_HEAD(entries);
  struct some_entry *cur, *tmp;
  
  struct file *filp = dentry_open(dent, NULL, 0, NULL);
  filp->f_op->readdir(filp, &entries, example_filldir);
  list_for_each_entry_safe_reverse(cur, tmp, &entries, member) {
    fprintf(stdout, "%s\t%d\n", cur->name, cur->size);
    list_del(&cur->member);
    free(cur);
  }
  put_filp(filp); // remember to free
}

static void test_readdir(struct dentry *droot) {
  struct dentry *dent;
  char *fsname;
  const int k_num_seg = 4;
  char dir[k_num_seg][MAX_NAME_LEN + 1];
  
  // list all user file systems
  fprintf(stdout, "ls root:\n");
  do_ls_(droot);

  // lookup path "/fs_name/dir[1]/dir[2]/dir[3]"
  fsname = "0_0_0";
  strcpy(dir[0], fsname);
  strcpy(dir[1], "0");
  dent = do_lookup_(droot, dir, 2);
  if (!dent || !dent->d_inode) {
    DEBUG_("[Error@test_readdir] cannot find %s of %s.\n", dir[1], fsname);
  } else {
    fprintf(stdout, "\nls %s of fs %s:\n", dent->d_name.name, fsname);
    do_ls_(dent);
  }
  
  fsname = "0_4_3";
  strcpy(dir[0], fsname);
  strcpy(dir[1], "2");
  strcpy(dir[2], "2.2");
  dent = do_lookup_(droot, dir, 3);
  if (!dent || !dent->d_inode) {
    DEBUG_("[Error@test_readdir] cannot find %s of %s.\n", dir[2], fsname);
  } else {
    fprintf(stdout, "\nls %s of fs %s:\n", dent->d_name.name, fsname);
    do_ls_(dent);
  }
}

static spinlock_t create_ln_rm_lock_;
static int create_test_cnt = 0;
static int create_ok_cnt = 0;
static int ln_ok_cnt = 0;
static int unlink_ok_cnt = 0;
static int rm_ok_cnt = 0;

// Includes example for invoking:
// cinq_create(), cinq_link(), cinq_unlink(), cinq_rmdir()
static void *rand_create_ln_rm(void *droot) {
  // randomly choose client file system
  char fs_name[MAX_NAME_LEN + 1];
  const int fs_j = rand() % FS_CHILDREN_ + 1;
  const int fs_k = rand() % (FS_CHILDREN_ + 1);
  sprintf(fs_name, "0_%x_%x", fs_j, fs_k);
  struct cinq_fsnode *req_fs = cfs_find_syn(&file_systems, fs_name);
  
  const int k_num_seg = 4;
  char dir[k_num_seg][MAX_NAME_LEN + 1];
  int num_create_test = 0, num_create_ok = 0;
  int num_link_ok = 0, num_unlink_ok = 0, num_rmdir_ok = 0;
  int i;
  for (i = 0; i < NUM_CREATE_; ++i) {
    // manually fills dir segments
    // which should be parsed from path in practice
    const int dir_i = rand() % CNODE_CHILDREN_;
    const int dir_j = rand() % CNODE_CHILDREN_;
    const int dir_k = rand() % CNODE_CHILDREN_;
    if (fs_j <= dir_j && fs_k <= dir_k) continue;
    
    // lookup path "/fs_name/dir[1]/dir[2]/dir[3]"
    sprintf(dir[0], "%s", fs_name); // the first segment is special
    sprintf(dir[1], "%x", dir_i);   // the rest are normal ones under root
    sprintf(dir[2], "%s.%x", dir[1], dir_j);
    sprintf(dir[3], "%s.%x", dir[2], dir_k);
    
    // Omits dcache lookup.
    // Dentry cache should be firstly used in practice.
    
    struct dentry* const dir_dent = do_lookup_(droot, (void *)dir, k_num_seg);
    if (!dir_dent || strcmp(fs_name, i_fs(dir_dent->d_inode)->fs_name) == 0)
      continue;
    
    ++num_create_test;
    
    /* Example for invoking cinq_create() */
    struct inode* const dir_inode = dir_dent->d_inode;
    int mode = (CINQ_VISIBLE << CINQ_MODE_SHIFT) | S_IFREG | S_IRWXO | S_IRGRP;
    char file_name[MAX_NAME_LEN + 1];
    sprintf(file_name, "%d", rand());
    const struct qstr q_file_name =
        { .name = (unsigned char *)file_name, .len = strlen(file_name) };
    struct dentry* const file_dent = d_alloc(dir_dent, &q_file_name);
    
    if (dir_inode->i_op->create(dir_inode, file_dent, mode, NULL)) {
      DEBUG_("[Error@rand_create_ln_rm] failed to create %s@%s.\n",
             file_name, i_cnode(dir_inode)->ci_name);
      continue;
    }
    
    /* Example for invoking cinq_link() */
    // reuse container and file_dent
    char link_name[MAX_NAME_LEN + 1];
    sprintf(link_name, "__%s", file_name); // adds some prefix to denote link
    const struct qstr q_link_name =
        { .name = (unsigned char *)link_name, .len = strlen(link_name) };
    struct dentry* const link_dent = d_alloc(dir_dent, &q_link_name);

    if (dir_inode->i_op->link(file_dent, dir_inode, link_dent)) {
      DEBUG_("[Error@rand_create_ln_rm] failed to link to %s@%s.\n",
             file_name, i_cnode(dir_inode)->ci_name);
      continue;
    }

    // repeat lookup to check create and link
    // the following part is only for test purpose
    int pass = 1;
    char dir_file[k_num_seg + 1][MAX_NAME_LEN + 1];
    memcpy(dir_file, dir, sizeof(dir));
    sprintf(dir_file[k_num_seg], "%s", file_name);
    struct dentry *d_file = do_lookup_(droot, (void *)dir_file, k_num_seg + 1);
    if (!d_file || i_fs(d_file->d_inode) != req_fs) {
      pass = 0;
    }
    fprintf(stdout, "cinq_create: %s(%s)@%s(%s)\t%s\n",
            file_name, d_file ? i_fs(d_file->d_inode)->fs_name : "-",
            dir[k_num_seg - 1], fs_name,
            pass ? "OK" : "WRONG");
    if (pass) ++num_create_ok;
    else continue;
    
    pass = 1;
    sprintf(dir_file[k_num_seg], "__%s", file_name);
    struct dentry *d_link = do_lookup_(droot, (void *)dir_file, k_num_seg + 1);
    if (!d_link || d_file->d_inode != d_link->d_inode) {
      pass = 0;
    }
    fprintf(stdout, "cinq_link: %s@%s -> %s@%s\t%s\n",
            dir_file[k_num_seg], dir_file[k_num_seg - 1],
            d_link ? i_cnode(d_link->d_inode)->ci_name : "-",
            d_link ? i_cnode(d_link->d_parent->d_inode)->ci_name : "-",
            pass ? "OK" : "WRONG");
    if (pass) ++num_link_ok;
    
    /* Example for invoking cinq_unlink() */
    if (dir_inode->i_op->unlink(dir_inode, file_dent)) {
      DEBUG_("[Error@rand_create_ln_rm] failed to unlink %s@%s.\n",
             file_name, i_cnode(dir_inode)->ci_name);
      continue;
    }
    // the following part is only for test purpose
    pass = 1;
    sprintf(dir_file[k_num_seg], "__%s", file_name);
    d_file = do_lookup_(droot, (void *)dir_file, k_num_seg + 1);
    if (!d_file || d_file->d_inode != d_link->d_inode) {
      pass = 0; // the linked file should exist
    }
    sprintf(dir_file[k_num_seg], "%s", file_name);
    d_file = do_lookup_(droot, (void *)dir_file, k_num_seg + 1);
    if (d_file) { // the unlinked file should be gone
      pass = 0;
    }
    fprintf(stdout, "cinq_unlink: %s@%s\t%s\n",
            dir_file[k_num_seg], dir_file[k_num_seg - 1],
            pass ? "OK" : "WRONG");
    if (pass) ++num_unlink_ok;
    
    /* Example for invoking cinq_rmdir() */
    pass = 0;
    struct inode *i_parent = dir_dent->d_parent->d_inode;
    if (i_parent->i_op->rmdir(i_parent, dir_dent) != -ENOTEMPTY) {
      DEBUG_("[Error@rand_create_ln_rm] removed non-emtpy dir: %s\n",
             i_cnode(dir_inode)->ci_name);
      continue;
    }
    if (dir_inode->i_op->unlink(dir_inode, link_dent)) { // make the dir empty
      DEBUG_("[Error@rand_create_ln_rm] failed to delete link file: %s@%s\n",
             link_dent->d_name.name, i_cnode(dir_inode)->ci_name);
      continue;
    }
    if (i_parent->i_op->rmdir(i_parent, dir_dent)) {
      DEBUG_("[Error@rand_create_ln_rm] failed to rmdir: %s\n",
             dir_dent->d_name.name);
      continue;
    }
    fprintf(stdout, "cinq_rmdir: %s(%s)\tOK\n",
            dir_file[k_num_seg - 1], fs_name);
    ++num_rmdir_ok;
    
  } // for
  
  spin_lock(&create_ln_rm_lock_);
  create_ok_cnt += num_create_ok;
  create_test_cnt += num_create_test;
  ln_ok_cnt += num_link_ok;
  unlink_ok_cnt += num_unlink_ok;
  rm_ok_cnt += num_rmdir_ok;
  spin_unlock(&create_ln_rm_lock_);
  pthread_exit(NULL);
}

atomic_t num_sym_ok = { .counter = 0 };
atomic_t num_sym_test = { .counter = 0 };

static void *rand_sym(void *droot) {
  // randomly choose client file system
  char fs_name[MAX_NAME_LEN + 1];
  const int fs_j = rand() % FS_CHILDREN_ + 1;
  const int fs_k = rand() % (FS_CHILDREN_ + 1);
  sprintf(fs_name, "0_%x_%x", fs_j, fs_k);
  
  const int k_num_seg = 4;
  char dir[k_num_seg][MAX_NAME_LEN + 1];
  int i;
  for (i = 0; i < NUM_CREATE_; ++i) {
    // randomly choose directory
    const int dir_i = rand() % CNODE_CHILDREN_;
    const int dir_j = rand() % CNODE_CHILDREN_;
    const int dir_k = rand() % CNODE_CHILDREN_;
    // lookup path "/fs_name/dir[1]/dir[2]/dir[3]"
    sprintf(dir[0], "%s", fs_name); // the first segment is special
    sprintf(dir[1], "%x", dir_i);   // the rest are normal ones under root
    sprintf(dir[2], "%s.%x", dir[1], dir_j);
    sprintf(dir[3], "%s.%x", dir[2], dir_k);
    
    struct dentry* const dir_dent = do_lookup_(droot, (void *)dir, k_num_seg);
    if (!dir_dent || strcmp(fs_name, i_fs(dir_dent->d_inode)->fs_name) == 0) {
      continue;
    }
    struct inode* const dir_inode = dir_dent->d_inode;
    
    atomic_inc(&num_sym_test);
    
    /* Example for invoking cinq_symlink */
    char* const symname = "/a/b/c";
    char file_name[MAX_NAME_LEN + 1];
    sprintf(file_name, "sym_%d", rand());
    const struct qstr q_file_name =
        { .name = (unsigned char *)file_name, .len = strlen(file_name) };
    struct dentry* const file_dent = d_alloc(dir_dent, &q_file_name);
    int err;
    err = dir_inode->i_op->symlink(dir_inode, file_dent, symname);
    if (err) {
      DEBUG_("[Error@rand_sym_] failed to symlink: %s@%s\n",
             file_dent->d_name.name, dir_dent->d_name.name);
      continue;
    }
    
    /* Example for invoking cinq_followlink */
    // NOTE that usage of readlink and followlink are not demonstrated here.
    // These tests are only to validate their functionality.
    // For usage in user space, refer to linux VFS path walking.
    char *buf = malloc(256);
    generic_readlink(file_dent, buf, 256); // including cinq_followlink
    err = strcmp(buf, symname);
    DEBUG_ON_(err, "[Warn@rand_sym] read out wrong symname: %s\n", buf);
    fprintf(stdout, "cinq_sym: %s\t%s\n", symname, err ? "WRONG" : "OK");
    if (!err) atomic_inc(&num_sym_ok);
  }
  pthread_exit(NULL);
}

int main(int argc, const char * argv[]) {
  // Start point
  struct dentry *meta_dent = cinqfs.mount((struct file_system_type *)&cinqfs,
                                       0, NULL, NULL);
  
  // Constructs a basic balanced file system tree
  fprintf(stdout, "\nConstruct a file system tree:\n");
  make_fs_tree(meta_dent);
  
  struct cinq_fsnode *root_fs = cfs_find_syn(&file_systems, "0_0_0");
  print_fs_tree(root_fs);

  /* The following two steps simulate the process of file system update. */
  // Moves a subtree
//  struct cinq_fsnode *extra = fsnode_new(root_fs, "extra");
//  fsnode_move(root_fs->fs_children, extra); // moves its first child
//  fprintf(stdout, "\nAfter adding 'extra' and moving:\n");
//  print_fs_tree(root_fs);
//  fprintf(stderr, "\nTry wrong operation. \n"
//          "(with error message if using -DCINQ_DEBUG)\n");
//  fsnode_move(root_fs, extra); // try invalid operation
//  
//  // Bridges the extra created above.
//  fprintf(stdout, "\nCross out 'extra':\n");
//  fsnode_bridge(extra);
//  print_fs_tree(root_fs);

  struct cinq_inode *root_cnode = i_cnode(meta_dent->d_inode);
  fprintf(stdout, "\nDir/file tree:\n");
  print_dir_tree(root_cnode);
  
  // Generates balanced dir/file tree on each file system
  const int k_fsn = HASH_CNT(fs_member, file_systems.cfs_table);
  pthread_t mkdir_t[k_fsn];
  memset(mkdir_t, 0, sizeof(mkdir_t));
  struct cinq_fsnode *fs;
  int ti, err;
  for (ti = 0, fs = file_systems.cfs_table; fs != NULL;
       ++ti, fs = fs->fs_member.next) {
    err = pthread_create(&mkdir_t[ti], NULL, make_dir_tree, fs);
    DEBUG_ON_(err, "[Error@main] error code of pthread_create: %d.\n", err);
  }
  fprintf(stdout, "\nWith three-layer dir tree:\n");
  void *status;
  for (ti = 0; ti < k_fsn; ++ti) {
    err = pthread_join(mkdir_t[ti], &status);
    DEBUG_ON_(err, "[Error@main] error code of pthread_join: %d.\n", err);
  }
  print_dir_tree(root_cnode);

  fprintf(stdout, "\nTest lookup:\n");
  pthread_t lookup_thr[LOOKUP_THR_NUM_];
  memset(lookup_thr, 0, sizeof(lookup_thr));
  for (ti = 0; ti < LOOKUP_THR_NUM_; ++ti) {
    err = pthread_create(&lookup_thr[ti], NULL, rand_lookup, meta_dent);
    DEBUG_ON_(err, "[Error@main] error code of pthread_create: %d.\n", err);
  }
  for (ti = 0; ti < LOOKUP_THR_NUM_; ++ti) {
    err = pthread_join(lookup_thr[ti], &status);
    DEBUG_ON_(err, "[Error@main] error code of pthread_join: %d.\n", err);
  }
  
  fprintf(stdout, "\nTest create_ln_rm:\n");
  pthread_t create_thr[CREATE_THR_NUM_];
  memset(create_thr, 0, sizeof(create_thr));
  for (ti = 0; ti < CREATE_THR_NUM_; ++ti) {
    err = pthread_create(&create_thr[ti], NULL, rand_create_ln_rm, meta_dent);
    DEBUG_ON_(err, "[Error@main] error code of pthread_create: %d.\n", err);
  }
  for (ti = 0; ti < CREATE_THR_NUM_; ++ti) {
    err = pthread_join(create_thr[ti], &status);
    DEBUG_ON_(err, "[Error@main] error code of pthread_join: %d.\n", err);
  }
  
  fprintf(stdout, "\nTest symlink:\n");
  pthread_t sym_thr[CREATE_THR_NUM_];
  memset(sym_thr, 0, sizeof(sym_thr));
  for (ti = 0; ti < CREATE_THR_NUM_; ++ti) {
    err = pthread_create(&sym_thr[ti], NULL, rand_sym, meta_dent);
    DEBUG_ON_(err, "[Error@main] error code of pthread_create: %d.\n", err);
  }
  for (ti = 0; ti < CREATE_THR_NUM_; ++ti) {
    err = pthread_join(sym_thr[ti], &status);
    DEBUG_ON_(err, "[Error@main] error code of pthread_join: %d.\n", err);
  }
  
  fprintf(stdout, "\nTest readdir:\n");
  test_readdir(meta_dent);

#ifdef CINQ_DEBUG
  int max_dentry_num = atomic_read(&num_dentry_);
  int max_inode_num = atomic_read(&num_inode_);
#endif
  
  // Kill file systems
  cinqfs.kill_sb(meta_dent->d_sb);
  
  // Show results
  int expected_num = NUM_LOOKUP_ * LOOKUP_THR_NUM_;
  fprintf(stdout, "\nmkdir/lookup: %d/%d checked ok [%s].\n",
          lookup_ok_cnt, expected_num,
          lookup_ok_cnt < expected_num ? "NOT Passed" : "Passed");

  fprintf(stdout, "create: %d/%d checked ok [%s].\n",
          create_ok_cnt, create_test_cnt,
          create_ok_cnt < create_test_cnt ? "NOT Passed" : "Passed");
  fprintf(stdout, "link: %d/%d checked ok [%s].\n",
          ln_ok_cnt, create_test_cnt,
          ln_ok_cnt < create_test_cnt ? "NOT Passed" : "Passed");
  fprintf(stdout, "unlink: %d/%d checked ok [%s].\n",
          unlink_ok_cnt, create_test_cnt,
          unlink_ok_cnt < create_test_cnt ? "NOT Passed" : "Passed");
  fprintf(stdout, "rmdir: %d/%d checked ok [%s].\n",
          rm_ok_cnt, create_test_cnt,
          rm_ok_cnt < create_test_cnt ? "NOT Passed" : "Passed");
  
  fprintf(stdout, "sym: %d/%d checked ok [%s].\n",
          atomic_read(&num_sym_ok), atomic_read(&num_sym_test),
          atomic_read(&num_sym_ok) < atomic_read(&num_sym_test) ?
          "NOT Passed" : "Passed");
  
  fprintf(stdout, "readdir also needs manual check of log [%s].\n",
          atomic_read(&readdir_is_ok) ?
          "Passed" : "NOT Passed");
  
#ifdef CINQ_DEBUG
  int final_dentry_num = atomic_read(&num_dentry_);
  fprintf(stdout, "dentry leak test: %d -> %d\t[%s].\n",
          max_dentry_num, final_dentry_num,
          final_dentry_num ? "NOT Passed" : "Passed");
  
  int final_inode_num = atomic_read(&num_inode_);
  fprintf(stdout, "inode leak test: %d -> %d\t[%s].\n",
          max_inode_num, final_inode_num,
          final_inode_num ? "NOT Passed" : "Passed");
#endif
  
  return 0;
}

