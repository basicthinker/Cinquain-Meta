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
#define NUM_LOOKUPS_ 50

static void print_fs_tree_(const int depth, const int no,
                          struct cinq_fsnode *root) {
  int i;
  for (i = 0; i < depth; ++i) {
    fprintf(stdout, "  ");
  }
  // check global list
  struct cinq_fsnode *fsnode;
  HASH_FIND_BY_STR(fs_member, file_systems.fs_table, root->fs_name, fsnode);
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

// Example for invoking cinq_mkdir
static void *make_dir_tree(void *fsnode) {
  struct cinq_fsnode *fs = (struct cinq_fsnode *)fsnode;
  struct dentry *root = fs->fs_root;
  int i, j, k;
  int mi, mj, mk;
  sscanf(fs->fs_name, "%x_%x_%x", &mi, &mj, &mk);
  char buffer[MAX_NAME_LEN + 1];
  int mode = (CINQ_MERGE << CINQ_MODE_SHIFT) | S_IFDIR;
  
  // make three-layer directory tree
  for (i = mi; i < CNODE_CHILDREN_; ++i) {
    struct inode *dir = root->d_inode;
    sprintf(buffer, "%x", i);
    struct qstr dname =
        { .name = (unsigned char *)buffer, .len = strlen(buffer) };
    struct dentry *den = d_alloc(root, &dname);
    den->d_fsdata = (void *)fs->fs_id;
    if (dir->i_op->mkdir(dir, den, mode)) {
      DEBUG_("[Error@make_dir_tree] failed to make dir '%s' by fs %lx(%s).\n",
             buffer, fs->fs_id, fs->fs_name);
      pthread_exit(NULL);
    }
    
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

static spinlock_t num_ok_lock;
static int total_num_ok = 0;

// Example for invoking cinq_lookup
static void *rand_lookup(void *droot) {
  // randomly choose client file system
  char fs_name[MAX_NAME_LEN + 1];
  const int fs_j = rand() % FS_CHILDREN_ + 1;
  const int fs_k = rand() % (FS_CHILDREN_ + 1);
  sprintf(fs_name, "0_%x_%x", fs_j, fs_k);
  
  const int k_num_seg = 4;
  char dir[k_num_seg][MAX_NAME_LEN + 1];
  char *result = "OK";
  int num_ok = 0;
  int i, j;
  for (i = 0; i < NUM_LOOKUPS_; ++i) {
    // manually fills dir segments
    // which should be parsed from path in practice
    const int dir_i = rand() % (CNODE_CHILDREN_ + 1);
    const int dir_j = rand() % (CNODE_CHILDREN_ + 1);
    const int dir_k = rand() % (CNODE_CHILDREN_ + 1);
    // lookup path "/fs_name/dir[1]/dir[2]/dir[3]"
    sprintf(dir[0], "%s", fs_name);
    sprintf(dir[1], "%x", dir_i);
    sprintf(dir[2], "%s.%x", dir[1], dir_j);
    sprintf(dir[3], "%s.%x", dir[2], dir_k);
    
    // Omits dcache lookup.
    // Dentry cache should be firstly used in practice.
    struct dentry *den = (struct dentry *)droot;
    struct inode *inode = den->d_inode;
    for (j = 0; j < k_num_seg; ++j) {
      struct qstr dname =
          { .name = (unsigned char *)dir[j], .len = strlen(dir[j]) };
      struct dentry *subden = d_alloc(den, &dname);
      subden->d_fsdata = den->d_fsdata;
      inode->i_op->lookup(inode, subden, NULL);
      if (!subden->d_inode) { // when target path is not found
        if (dir_i >= CNODE_CHILDREN_ || dir_j >= CNODE_CHILDREN_ ||
            dir_k >= CNODE_CHILDREN_) { // when it should be not-found
          result = "OK";
        } else {
          result = "WRONG!";
        }
        fprintf(stdout, "%s finds %s\t->\t - \t%s\n", fs_name, dir[j], result);
        break;
      } else {
        den = subden;
        inode = den->d_inode;
      }
    } // for
    if (j == k_num_seg) { // successful lookup
      result = "WRONG!";
      char *cur_fs_name = ((struct cinq_fsnode *)den->d_fsdata)->fs_name;
      if (strcmp(fs_name, cur_fs_name)) { // not identical
        // when parent file system is used
        if (fs_j > dir_j) { // root file system should be used
          if (strcmp(cur_fs_name, "0_0_0") == 0) {
            result = "OK";
          }
        } else if (fs_k > dir_k) { // second layer file system shoud be used
          int cur_j, cur_k;
          sscanf(cur_fs_name, "0_%x_%x", &cur_j, &cur_k);
          if (cur_j == fs_j && cur_k == 0) {
            result = "OK";
          }
        }
      } else if (fs_j <= dir_j && fs_k <= dir_k) {
        result = "OK"; // when file system not changed
      }
      fprintf(stdout, "%s finds %s\t->\t%s\t%s\n", fs_name, dir[j - 1],
              cur_fs_name, result);
    }
    if (strcmp(result, "OK") == 0) ++num_ok;
  } // for
  spin_lock(&num_ok_lock);
  total_num_ok += num_ok;
  spin_unlock(&num_ok_lock);
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
  print_dir_tree(cnode(iroot));
  
  // Generates balanced dir/file tree on each file system
  const int k_fsn = HASH_CNT(fs_member, file_systems.fs_table);
  pthread_t mkdir_t[k_fsn];
  int ti = 0;
  for (fs = file_systems.fs_table; fs != NULL; fs = fs->fs_member.next) {
    int err = pthread_create(&mkdir_t[ti], NULL, make_dir_tree, fs);
    if (err) {
      DEBUG_("[Error@main] return code from pthread_create() is %d.\n", err);
    }
  }
  fprintf(stdout, "\nWith three-layer dir tree:\n");
  void *status;
  for (ti = 0; ti < k_fsn; ++ti) {
    pthread_join(mkdir_t[ti], &status);
  }
  print_dir_tree(cnode(iroot));

  fprintf(stdout, "\nTest lookup:\n");
  const int k_tn = 5; // number of threads
  pthread_t lookup_t[k_tn];
  int i;
  for (i = 0; i < k_tn; ++i) {
    int err = pthread_create(&lookup_t[i], NULL, rand_lookup, droot);
    if (err) {
      DEBUG_("[Error@main] return code from pthread_create() is %d.\n", err);
    }
  }
  for (i = 0; i < k_tn; ++i) {
    pthread_join(lookup_t[i], &status);
  }
  fprintf(stdout, "\n%d/%d checked OK.\n", total_num_ok, NUM_LOOKUPS_ * k_tn);
  
  // Kill file systems
  cinqfs.kill_sb(droot->d_sb);
  fsnode_free_all(fsroot);
  
  return 0;
}

