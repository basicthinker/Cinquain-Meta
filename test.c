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

static void __print_fstree(const int depth, const int no,
                         struct cinq_fsnode *subroot) {
  int i;
  for (i = 0; i < depth; ++i) {
    fprintf(stdout, "  ");
  }
  fprintf(stdout, "%d. ID=%u\n", no, subroot->fs_id);
  
  i = 0;
  struct cinq_fsnode *p;
  for (p = subroot->children; p != NULL; p = p->hh.next) {
    __print_fstree(depth + 1, ++i, p);
  }
}

static inline void print_fstree(struct cinq_fsnode *root) {
  __print_fstree(0, 0, root);
}

int main (int argc, const char * argv[])
{
  root = fsnode_alloc(NULL);
  
  // Constructs a basic balanced tree
  const int num_children = 3;
  int i, j;
  for (i = 0; i < num_children; ++i) {
    struct cinq_fsnode *child = fsnode_alloc(root);
    for (j = 0; j < num_children; ++j) {
      fsnode_alloc(child);
    }
  }
  print_fstree(root);
  
  // Moves a subtree
  struct cinq_fsnode *extra = fsnode_alloc(root);
  fsnode_move(root->children, extra); // moves its first child
  print_fstree(root);
  fsnode_move(root, extra); // try invalid operation
  
  fsnode_free_all(root);
  return 0;
}

