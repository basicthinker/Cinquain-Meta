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

struct cinq_fsnode *fs_root;
extern struct cinq_fsnode *file_systems;

static void print_fstree_(const int depth, const int no,
                          struct cinq_fsnode *root) {
  int i;
  for (i = 0; i < depth; ++i) {
    fprintf(stdout, "  ");
  }
  // check global list
  struct cinq_fsnode *fsnode;
  HASH_FIND_BY_STR(fs_member, file_systems, root->fs_name, fsnode);
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
    print_fstree_(depth + 1, ++i, p);
  }
}

static void print_cnode_tree_(const int depth, const int no,
                              struct cinq_inode *root) {
  int i;
  for (i = 0; i < depth; ++i) {
    fprintf(stdout, "  ");
  }
  fprintf(stdout, "%d. ID=%lu name=%s with", no, root->ci_id, root->ci_name);
  struct cinq_tag *cur, *tmp;
  HASH_ITER(hh, root->ci_tags, cur, tmp) {
    fprintf(stdout, " %lu", cur->t_fs->fs_id);
  }
  fprintf(stdout, "\n");
  i = 0;
  struct cinq_inode *p;
  for (p = root->ci_children; p != NULL; p = p->ci_child.next) {
    print_cnode_tree_(depth + 1, ++i, p);
  }
}

static inline void print_fstree(struct cinq_fsnode *root) {
  print_fstree_(0, 1, root);
}

static inline void print_cnode_tree(struct cinq_inode *root) {
  print_cnode_tree_(0, 1, root);
}

int main (int argc, const char * argv[])
{
  fs_root = fsnode_new("template", NULL);
  
  // Constructs a basic balanced tree
  const int num_children = 3;
  int i, j;
  char buffer[MAX_NAME_LEN + 1];
  for (i = 1; i <= num_children; ++i) {
    sprintf(buffer, "customer_%x", i); // produces unique name
    struct cinq_fsnode *child = fsnode_new(buffer, fs_root);
    for (j = 1; j <= num_children; ++j) {
      sprintf(buffer, "customer_%x", i * num_children + j); // produces unique name
      fsnode_new(buffer, child);
    }
  }
  fprintf(stdout, "\nConstruct a file system tree:\n");
  print_fstree(fs_root);
  
  // Moves a subtree
  struct cinq_fsnode *extra = fsnode_new("customer_extra", fs_root);
  fsnode_move(fs_root->fs_children, extra); // moves its first child
  fprintf(stdout, "\nAfter move:\n");
  print_fstree(fs_root);
  fprintf(stderr, "\nTry wrong operation. \n"
          "(with error message if using -DCINQ_DEBUG)\n");
  fsnode_move(fs_root, extra); // try invalid operation
  
  // Bridges the extra created above
  fprintf(stdout, "\nCross out the extra:\n");
  fsnode_bridge(extra);
  print_fstree(fs_root);
  
  // Generates balanced dir/file tree on each file system
  
  
  fsnode_free_all(fs_root);
  return 0;
}

