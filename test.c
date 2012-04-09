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
#define FS_CHILDREN_ 3
#define CNODE_CHILDREN_ 5 // should be larger than FS_CHILDREN_

#define NUM_LOOKUP_ 50
#define LOOKUP_THR_NUM_ 5 // number of threads for rand_lookup
#define NUM_CREATE_ 10
#define CREATE_THR_NUM_ 5

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
      fprintf(stdout, " ROOT_TAG ");
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

static void make_fs_tree(struct cinq_fsnode *root) {
  // make two-layer file-system tree
  int i, j;
  char sub[MAX_NAME_LEN + 1];
  for (i = 1; i <= FS_CHILDREN_; ++i) {
    sprintf(sub, "0_%x_0", i); // produces unique name
    struct cinq_fsnode *child = fsnode_new(sub, root);
    for (j = 1; j <= FS_CHILDREN_; ++j) {
      char subsub[MAX_NAME_LEN + 1];
      sprintf(subsub, "0_%x_%x", i, j); // produces unique name
      fsnode_new(subsub, child);
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
  int mode = (CINQ_MERGE << CINQ_MODE_SHIFT) | S_IFDIR;
  
  // make three-layer directory tree for test
  for (i = mi; i < CNODE_CHILDREN_; ++i) {
    // Example for invoking cinq_mkdir
    // (1) prepare parameters
    struct inode *dir = root->d_inode; // containing dir
    sprintf(buffer, "%x", i);
    struct qstr dname =                // new dir name passed via dentry
        { .name = (unsigned char *)buffer, .len = strlen(buffer) };
    struct dentry *den = d_alloc(root, &dname);
    den->d_fsdata = (void *)fs->fs_id; // which client fs takes the operation
    
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
      subden->d_fsdata = (void *)fs->fs_id;
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
        subsubden->d_fsdata = (void *)fs->fs_id;
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
    subden->d_fsdata = den->d_fsdata;
    
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
static int lookup_num_ok_ = 0;

// Includes example for invoking cinq_create
static void *rand_lookup_(void *droot) {
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
      char *cur_fs_name = d_fs(found_dent)->fs_name;
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
            found_dent ? d_fs(found_dent)->fs_name : "-",
            pass ? "OK" : "WRONG");
    if (pass) ++num_ok;
  } // for
  spin_lock(&lookup_num_lock_);
  lookup_num_ok_ += num_ok;
  spin_unlock(&lookup_num_lock_);
  pthread_exit(NULL);
}

static spinlock_t create_link_num_lock_;
static int create_num_ok_ = 0;
static int link_num_ok_ = 0;
static int unlink_num_ok_ = 0;
static int rmdir_num_ok_ = 0;

// Includes example for invoking:
// cinq_create, cinq_link, cinq_unlink, cinq_rmdir
static void *rand_create_link_(void *droot) {
  // randomly choose client file system
  char fs_name[MAX_NAME_LEN + 1];
  const int fs_j = rand() % FS_CHILDREN_ + 1;
  const int fs_k = rand() % (FS_CHILDREN_ + 1);
  sprintf(fs_name, "0_%x_%x", fs_j, fs_k);
  struct cinq_fsnode *req_fs = cfs_find_syn(&file_systems, fs_name);
  
  const int k_num_seg = 4;
  char dir[k_num_seg][MAX_NAME_LEN + 1];
  int num_create_ok = 0, num_link_ok = 0;
  int num_unlink_ok = 0, num_rmdir_ok = 0;
  int i;
  for (i = 0; i < NUM_CREATE_; ++i) {
    // manually fills dir segments
    // which should be parsed from path in practice
    const int dir_i = rand() % CNODE_CHILDREN_;
    const int dir_j = rand() % CNODE_CHILDREN_;
    const int dir_k = rand() % CNODE_CHILDREN_;
    if (fs_j <= dir_j && fs_k <= dir_k) {
      --i;
      continue;
    }    
    // lookup path "/fs_name/dir[1]/dir[2]/dir[3]"
    sprintf(dir[0], "%s", fs_name); // the first segment is special
    sprintf(dir[1], "%x", dir_i);   // the rest are normal ones under root
    sprintf(dir[2], "%s.%x", dir[1], dir_j);
    sprintf(dir[3], "%s.%x", dir[2], dir_k);
    
    // Omits dcache lookup.
    // Dentry cache should be firstly used in practice.
    
    struct dentry* const dir_dent = do_lookup_(droot, (void *)dir, k_num_seg);
    if (!dir_dent || strcmp(fs_name, d_fs(dir_dent)->fs_name) == 0) {
      --i;
      continue;
    }
    
    /* Example for invoking cinq_create */
    struct inode *dir_inode = dir_dent->d_inode;
    int mode = (CINQ_MERGE << CINQ_MODE_SHIFT) | S_IFREG;
    char file_name[MAX_NAME_LEN + 1];
    sprintf(file_name, "%d", rand());
    const struct qstr q_file_name =
        { .name = (unsigned char *)file_name, .len = strlen(file_name) };
    struct dentry* const file_dent = d_alloc(dir_dent, &q_file_name);
    // The fsnode can be determined by various ways.
    // Note that this is who takes the operation,
    // NOT always be d_fs(found_dent) who can be an ancestor.
    file_dent->d_fsdata = req_fs;
    
    if (dir_inode->i_op->create(dir_inode, file_dent, mode, NULL)) {
      DEBUG_("[Error@rand_create_link_] failed to create %s@%s.\n",
             file_name, i_cnode(dir_inode)->ci_name);
      continue;
    } 
    
    /* Example for invoking cinq_link */
    // reuse container and file_dent
    char link_name[MAX_NAME_LEN + 1];
    sprintf(link_name, "__%s", file_name); // adds some prefix to denote link
    const struct qstr q_link_name =
        { .name = (unsigned char *)link_name, .len = strlen(link_name) };
    struct dentry* const link_dent = d_alloc(dir_dent, &q_link_name);
    link_dent->d_fsdata = req_fs;
    
    if (dir_inode->i_op->link(file_dent, dir_inode, link_dent)) {
      DEBUG_("[Error@rand_create_link_] failed to link to %s@%s.\n",
             file_name, i_cnode(dir_inode)->ci_name);
      continue;
    }
    
    // repeat lookup to check
    // the following part is only for test purpose
    int pass = 1;
    char dir_file[k_num_seg + 1][MAX_NAME_LEN + 1];
    memcpy(dir_file, dir, sizeof(dir));
    sprintf(dir_file[k_num_seg], "%s", file_name);
    struct dentry *d_file = do_lookup_(droot, (void *)dir_file, k_num_seg + 1);
    if (!d_file || d_fs(d_file) != req_fs) {
      pass = 0;
    }
    fprintf(stdout, "rand_create_: %s(%s) -> %s(%s)\t%s\n",
            dir[k_num_seg - 1], fs_name,
            file_name, d_file ? d_fs(d_file)->fs_name : "-",
            pass ? "OK" : "WRONG");
    if (pass) ++num_create_ok;
    else continue;
    
    pass = 1;
    sprintf(dir_file[k_num_seg], "__%s", file_name);
    struct dentry *d_link = do_lookup_(droot, (void *)dir_file, k_num_seg + 1);
    if (!d_link || d_file->d_inode != d_link->d_inode) {
      pass = 0;
    }
    fprintf(stdout, "rand_link_: %s@%s -> %s@%s\t%s\n",
            dir_file[k_num_seg], dir_file[k_num_seg - 1],
            d_link ? i_cnode(d_link->d_inode)->ci_name : "-",
            d_link ? i_cnode(d_link->d_parent->d_inode)->ci_name : "-",
            pass ? "OK" : "WRONG");
    if (pass) ++num_link_ok;
    
    /* Example for invoking cinq_unlink */
    if (dir_inode->i_op->unlink(dir_inode, file_dent)) {
      DEBUG_("[Error@rand_create_link_] failed to unlink %s@%s.\n",
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
    fprintf(stdout, "rand_unlink_: %s@%s\t%s\n",
            dir_file[k_num_seg], dir_file[k_num_seg - 1],
            pass ? "OK" : "WRONG");
    if (pass) ++num_unlink_ok;
    
    /* Example for invoking cinq_rmdir */
    struct inode *i_parent = dir_dent->d_parent->d_inode;
    if (i_parent->i_op->rmdir(i_parent, dir_dent) != -ENOTEMPTY) {
      DEBUG_("[Error@rand_create_link_] removed non-emtpy dir: %s\n",
             i_cnode(dir_inode)->ci_name);
      continue;
    }
    if (dir_inode->i_op->unlink(dir_inode, link_dent)) {
      DEBUG_("[Error@rand_create_link_] failed to delete link file: %s@%s\n",
             link_dent->d_name.name, i_cnode(dir_inode)->ci_name);
      continue;
    }
    if (i_parent->i_op->rmdir(i_parent, dir_dent)) {
      DEBUG_("[Error@rand_create_link_] failed to remove dir: %s\n",
             i_cnode(dir_inode)->ci_name);
      continue;
    }
    fprintf(stdout, "rand_rmdir_: %s\tOK\n",
            dir_file[k_num_seg - 1]);
    ++num_rmdir_ok;
    
  } // for
  spin_lock(&create_link_num_lock_);
  create_num_ok_ += num_create_ok;
  link_num_ok_ += num_link_ok;
  unlink_num_ok_ += num_unlink_ok;
  rmdir_num_ok_ += num_rmdir_ok;
  spin_unlock(&create_link_num_lock_);
  pthread_exit(NULL);
}

int main(int argc, const char * argv[]) {
  struct cinq_fsnode *fsroot = fsnode_new("0_0_0", NULL);
  
  // Constructs a basic balanced file system tree
  make_fs_tree(fsroot);
  fprintf(stdout, "\nConstruct a file system tree:\n");
  print_fs_tree(fsroot);
  
  
  /* The following two steps simulate the process of file system update. */
  // Moves a subtree
  struct cinq_fsnode *extra = fsnode_new("extra", fsroot);
  fsnode_move(fsroot->fs_children, extra); // moves its first child
  fprintf(stdout, "\nAfter adding 'extra' and moving:\n");
  print_fs_tree(fsroot);
  fprintf(stderr, "\nTry wrong operation. \n"
          "(with error message if using -DCINQ_DEBUG)\n");
  fsnode_move(fsroot, extra); // try invalid operation
  
  // Bridges the extra created above.
  fprintf(stdout, "\nCross out 'extra':\n");
  fsnode_bridge(extra);
  print_fs_tree(fsroot);
  
  
  // Prepare cnode tree
  struct dentry *droot = cinqfs.mount((struct file_system_type *)&cinqfs,
                                       0, NULL, NULL);
  struct inode *iroot = droot->d_inode;
  // Register file systems by making first-level directories
  struct cinq_fsnode *fs;
  int mode = (CINQ_MERGE << CINQ_MODE_SHIFT) | S_IFDIR;
  for (fs = file_systems.fs_table; fs != NULL; fs = fs->fs_member.next) {
    struct qstr dname = { .name = (unsigned char *)fs->fs_name,
                          .len = strlen(fs->fs_name) };
    struct dentry *den = d_alloc(NULL, &dname);
    den->d_fsdata = (void *)fs->fs_id;
    iroot->i_op->mkdir(iroot, den, mode); // first parameter is root inode.
  }
  fprintf(stdout, "\nAfter file system registeration:\n");
  print_dir_tree(i_cnode(iroot));
  
  // Generates balanced dir/file tree on each file system
  const int k_fsn = HASH_CNT(fs_member, file_systems.fs_table);
  pthread_t mkdir_t[k_fsn];
  memset(mkdir_t, 0, sizeof(mkdir_t));
  int ti, err;
  for (ti = 0, fs = file_systems.fs_table; fs != NULL;
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
  print_dir_tree(i_cnode(iroot));

  fprintf(stdout, "\nTest lookup:\n");
  pthread_t lookup_thr[LOOKUP_THR_NUM_];
  memset(lookup_thr, 0, sizeof(lookup_thr));
  for (ti = 0; ti < LOOKUP_THR_NUM_; ++ti) {
    err = pthread_create(&lookup_thr[ti], NULL, rand_lookup_, droot);
    DEBUG_ON_(err, "[Error@main] error code of pthread_create: %d.\n", err);
  }
  for (ti = 0; ti < LOOKUP_THR_NUM_; ++ti) {
    err = pthread_join(lookup_thr[ti], &status);
    DEBUG_ON_(err, "[Error@main] error code of pthread_join: %d.\n", err);
  }
  
  fprintf(stdout, "\nTest create:\n");
  pthread_t create_thr[CREATE_THR_NUM_];
  memset(create_thr, 0, sizeof(create_thr));
  for (ti = 0; ti < CREATE_THR_NUM_; ++ti) {
    err = pthread_create(&create_thr[ti], NULL, rand_create_link_, droot);
    DEBUG_ON_(err, "[Error@main] error code of pthread_create: %d.\n", err);
  }
  for (ti = 0; ti < CREATE_THR_NUM_; ++ti) {
    err = pthread_join(create_thr[ti], &status);
    DEBUG_ON_(err, "[Error@main] error code of pthread_join: %d.\n", err);
  }
  
  int max_inode_num = atomic_read(&num_inodes_);
  
  // Kill file systems
  cinqfs.kill_sb(droot->d_sb);
  fsnode_evict_all(fsroot);
  
  // Show results
  int expected_num = NUM_LOOKUP_ * LOOKUP_THR_NUM_;
  fprintf(stdout, "\nmkdir/lookup: %d/%d checked OK [%s].\n",
          lookup_num_ok_, expected_num,
          lookup_num_ok_ < expected_num ? "NOT PASSED" : "PASSED");
  
  expected_num = NUM_CREATE_ * CREATE_THR_NUM_;
  fprintf(stdout, "create: %d/%d checked OK [%s].\n",
          create_num_ok_, expected_num,
          create_num_ok_ < expected_num ? "NOT PASSED" : "PASSED");
  fprintf(stdout, "link: %d/%d checked OK [%s].\n",
          link_num_ok_, expected_num,
          link_num_ok_ < expected_num ? "NOT PASSED" : "PASSED");
  fprintf(stdout, "unlink: %d/%d checked OK [%s].\n",
          unlink_num_ok_, expected_num,
          unlink_num_ok_ < expected_num ? "NOT PASSED" : "PASSED");
  fprintf(stdout, "rmdir: %d/%d checked OK [%s].\n",
          rmdir_num_ok_, expected_num,
          rmdir_num_ok_ < expected_num ? "NOT PASSED" : "PASSED");
  
  int final_inode_num = atomic_read(&num_inodes_);
  fprintf(stdout, "inode leak test: %d -> %d\t[%s].\n",
          max_inode_num, final_inode_num,
          final_inode_num ? "NOT PASSED" : "PASSED");
  
  return 0;
}

