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

struct cinq_fsnode *root;
extern struct cinq_fsnode *file_systems;

static void __print_fstree(const int depth, const int no,
                         struct cinq_fsnode *subroot) {
  int i;
  for (i = 0; i < depth; ++i) {
    fprintf(stdout, "  ");
  }
  // check global list
  struct cinq_fsnode *fsnode;
  HASH_FIND_BY_STR(fs_member, file_systems, subroot->fs_name, fsnode);
  if (fsnode != subroot) {
    fprintf(stderr, "Error locating fsnode %d: %p != %p\n",
            subroot->fs_id, fsnode, subroot);
  } else {
    fprintf(stdout, "%d. ID=%u name=%s\n",
            no, subroot->fs_id, subroot->fs_name);
  }
  
  i = 0;
  struct cinq_fsnode *p;
  for (p = subroot->fs_children; p != NULL; p = p->fs_child.next) {
    __print_fstree(depth + 1, ++i, p);
  }
}

static inline void print_fstree(struct cinq_fsnode *root) {
  __print_fstree(0, 0, root);
}

int main (int argc, const char * argv[])
{
  root = fsnode_new("template", NULL);
  
  // Constructs a basic balanced tree
  const int num_children = 3;
  int i, j;
  char buffer[MAX_NAME_LEN + 1];
  for (i = 1; i <= num_children; ++i) {
    sprintf(buffer, "customer_%x", i); // produces unique name
    struct cinq_fsnode *child = fsnode_new(buffer, root);
    for (j = 1; j <= num_children; ++j) {
      sprintf(buffer, "customer_%x", i * num_children + j); // produces unique name
      fsnode_new(buffer, child);
    }
  }
  fprintf(stdout, "\nConstruct a file system tree:\n");
  print_fstree(root);
  
  // Moves a subtree
  struct cinq_fsnode *extra = fsnode_new("customer", root);
  fsnode_move(root->fs_children, extra); // moves its first child
  fprintf(stdout, "\nAfter move:\n");
  print_fstree(root);
  fprintf(stderr, "\nTry wrong operation with an error expected:\n");
  fsnode_move(root, extra); // try invalid operation
  
  fsnode_free_all(root);
  return 0;
}

