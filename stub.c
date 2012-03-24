/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  stub.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/11/12.
//

#include "util.h"

// Supplement of kernel functions to user space.
// Subject to user-space adjustment.

/**
 * d_alloc	-	allocate a dcache entry
 * @parent: parent of entry to allocate
 * @name: qstr of the name
 *
 * Allocates a dentry. It returns %NULL if there is insufficient memory
 * available. On a success the dentry is returned. The name passed in is
 * copied and the copy passed in may be reused after this call.
 */

struct dentry *d_alloc(struct dentry * parent, const struct qstr *name) {
	struct dentry *dentry;
	char *dname;
  
	dentry = (struct dentry *)malloc(sizeof(struct dentry));
	if (!dentry)
		return NULL;
  
	dname = malloc(name->len + 1);
  if (!dname) {
    free(dentry); 
    return NULL;
  }	
	dentry->d_name.name = (unsigned char *)dname;
	dentry->d_name.len = name->len;
	dentry->d_name.hash = name->hash;
	memcpy(dname, name->name, name->len);
	dname[name->len] = 0;
  
	dentry->d_count = 1;
	dentry->d_flags = 0;
	spin_lock_init(&dentry->d_lock);
	// seqcount_init(&dentry->d_seq);
	dentry->d_inode = NULL;
	dentry->d_parent = NULL;
	dentry->d_sb = NULL;
	// dentry->d_op = NULL;
	dentry->d_fsdata = NULL;
  // INIT_HLIST_BL_NODE(&dentry->d_hash);
  // INIT_LIST_HEAD(&dentry->d_lru);
  INIT_LIST_HEAD(&dentry->d_subdirs);
  INIT_LIST_HEAD(&dentry->d_alias);
  INIT_LIST_HEAD(&dentry->d_u.d_child);
  
	if (parent) {
		spin_lock(&parent->d_lock);
		/*
		 * don't need child lock because it is not subject
		 * to concurrency here
		 */
		
    // __dget_dlock(parent);
    parent->d_count++; // expands the above
    
		dentry->d_parent = parent;
		dentry->d_sb = parent->d_sb;
		// d_set_d_op(dentry, dentry->d_sb->s_d_op);
		list_add(&dentry->d_u.d_child, &parent->d_subdirs);
		spin_unlock(&parent->d_lock);
	}
  
	// this_cpu_inc(nr_dentry);
  
	return dentry;
}

/**
 *      dget -      get a reference to a dentry
 *      @dentry: dentry to get a reference to
 *
 *      Given a dentry or %NULL pointer increment the reference count
 *      if appropriate and return the dentry. A dentry will not be 
 *      destroyed when it has references.
 */
struct dentry *dget(struct dentry *dentry) {
  if (dentry) {
    spin_lock(&dentry->d_lock);
    // dget_dlock(dentry);
    dentry->d_count++; // expands the above
    spin_unlock(&dentry->d_lock);
  }
  return dentry;
}


/**
 * d_instantiate - fill in inode information for a dentry
 * @entry: dentry to complete
 * @inode: inode to attach to this dentry
 *
 * Fill in inode information in the entry.
 *
 * This turns negative dentries into productive full members
 * of society.
 *
 * NOTE! This assumes that the inode count has been incremented
 * (or otherwise set) by the caller to indicate that it is now
 * in use by the dcache.
 */
void d_instantiate(struct dentry *dentry, struct inode * inode) {
	if (inode) {
		spin_lock(&inode->i_lock);
    
    // __d_instantiate(entry, inode);
    // expanded as following
    spin_lock(&dentry->d_lock);
    list_add(&dentry->d_alias, &inode->i_dentry);
    dentry->d_inode = inode;
    // dentry_rcuwalk_barrier(dentry);
    spin_unlock(&dentry->d_lock);
    
		spin_unlock(&inode->i_lock);
  }
	// security_d_instantiate(entry, inode);
}


/* DCACHE_DISCONNECTED
 * This dentry is possibly not currently connected to the dcache tree, in
 * which case its parent will either be itself, or will have this flag as
 * well.  nfsd will not use a dentry with this bit set, but will first
 * endeavour to clear the bit either by discovering that it is connected,
 * or by performing lookup operations.   Any filesystem which supports
 * nfsd_operations MUST have a lookup function which, if it finds a
 * directory inode with a DCACHE_DISCONNECTED dentry, will d_move that
 * dentry into place and return that dentry rather than the passed one,
 * typically using d_splice_alias. */

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
struct dentry *d_splice_alias(struct inode *inode,
                              struct dentry *dentry) {
  // Hook for user-space dentry cache implementation.
  
	// struct dentry *new = NULL;
  //	if (inode && S_ISDIR(inode->i_mode)) {
  //		spin_lock(&inode->i_lock);
  //		new = __d_find_alias(inode, 1);
  //		if (new) {
  //			BUG_ON(!(new->d_flags & DCACHE_DISCONNECTED));
  //			spin_unlock(&inode->i_lock);
  //			d_move(new, dentry);
  //			iput(inode);
  //		} else {
  //			/* already taking inode->i_lock, so d_add() by hand */
  //			__d_instantiate(dentry, inode);
  //			spin_unlock(&inode->i_lock);
  //			d_rehash(dentry);
  //		}
  //	} else
  //		d_add(dentry, inode);
  //  }
  // return new;

  // simplified, only when no alias is found
  d_instantiate(dentry, inode);
  // d_rehash(entry); // requires user-space implementation
	return NULL;
}

// Invoked when super block is killed
void d_genocide(struct dentry *root) {
  
}

// Returns the current uid who is taking some file system operation
unsigned int current_fsuid() {
  // return current->cred->fsuid;
  return 0;
}

// Returns the current gid who is taking some file system operation
unsigned int current_fsgid() {
  // return current->cred->fsuid;
  return 0;
}
