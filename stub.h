/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  stub.h
//  cinquain-meta
//
//  Created by Jinglei Ren  <jinglei.ren@gmail.com> on 3/11/12.
//

#ifndef CINQUAIN_META_STUB_H_
#define CINQUAIN_META_STUB_H_

#ifndef __KERNEL__

/**
 * d_add - add dentry to hash queues
 * @entry: dentry to add
 * @inode: the inode to attach to this dentry
 *
 * This adds the entry to the hash queues and initializes @inode.
 * The entry was actually filled in earlier during d_alloc().
*/
static inline void d_add(struct dentry *entry, inode_t *inode) {
  // User-space implementation
}

/**
 * d_splice_alias - splice a disconnected dentry into the tree if one exists
 * @inode:  the inode which may have a disconnected dentry
 * @dentry: a negative dentry which we want to point to the inode.
 *
 * If inode is a directory and has a 'disconnected' dentry (i.e. IS_ROOT and
 * DCACHE_DISCONNECTED), then d_move that in place of the given dentry
 * and return it, else simply d_add the inode to the dentry and return NULL.
 *
 * This is needed in the lookup routine of any filesystem that is exportable
 * (via knfsd) so that we can build dcache paths to directories effectively.
 *
 * If a dentry was found and moved, then it is returned.  Otherwise NULL
 * is returned.  This matches the expected return value of ->lookup.
 *
*/
static inline struct dentry *d_splice_alias(inode_t *inode,
                                            struct dentry *dentry) {
  // Needs complete user-space implementation
  dentry->d_inode = inode;
  return dentry;
}

#endif // __KERNEL__

#endif // CINQUAIN_META_STUB_H_
