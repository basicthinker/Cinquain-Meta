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

#ifndef __KERNEL__

#include "vfs.h"
#include "util.h"

#define IS_ROOT(x) ((x) == (x)->d_parent)

// Supplement of kernel functions to user space.
// Subject to user-space adjustment.

// ALL comments and implementation are borrowed from linux source.

/* Compute the hash for a name string. */
// Used for user-space dcache
unsigned int full_name_hash(const unsigned char *name, unsigned int len)
{
//  unsigned long hash = init_name_hash();
//  while (len--)
//    hash = partial_name_hash(*name++, hash);
//  return end_name_hash(hash);
  return 0;
}

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
#ifdef CINQ_DEBUG
  atomic_inc(&num_dentry_);
#endif // CINQ_DEBUG
  
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
  DEBUG_ON_(!list_empty(&dentry->d_alias), "[Error@d_instantiate] "
            "dentry already associated with another inode.");
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

/*
 * Release the dentry's inode, using the filesystem
 * d_iput() operation if defined. Dentry has no refcount
 * and is unhashed.
 */
static void dentry_iput_(struct dentry * dentry)
  __releases(dentry->d_lock)
  __releases(dentry->d_inode->i_lock) {

	struct inode *inode = dentry->d_inode;
	if (inode) {
		dentry->d_inode = NULL;
		list_del_init(&dentry->d_alias);
		spin_unlock(&dentry->d_lock);
		spin_unlock(&inode->i_lock);
//		if (!inode->i_nlink)
//			fsnotify_inoderemove(inode);
//		if (dentry->d_op && dentry->d_op->d_iput)
//			dentry->d_op->d_iput(dentry, inode);
//		else
			iput(inode);
	} else {
		spin_unlock(&dentry->d_lock);
	}
}

/**
 * d_kill - kill dentry and return parent
 * @dentry: dentry to kill
 * @parent: parent dentry
 *
 * The dentry must already be unhashed and removed from the LRU.
 *
 * If this is the root of the dentry tree, return NULL.
 *
 * dentry->d_lock and parent->d_lock must be held by caller, and are dropped by
 * d_kill.
 */
static struct dentry *d_kill_(struct dentry *dentry, struct dentry *parent)
  __releases(dentry->d_lock)
  __releases(parent->d_lock)
  __releases(dentry->d_inode->i_lock) {

	list_del(&dentry->d_u.d_child);
//	dentry->d_flags |= DCACHE_DISCONNECTED;
	if (parent)
		spin_unlock(&parent->d_lock);
	dentry_iput_(dentry);
  
	// d_free(dentry); // expanded as below
  DEBUG_ON_(dentry->d_count, "[Error@d_kill_] to free dentry with reference:"
            " %d\n", dentry->d_count);
//	this_cpu_dec(nr_dentry);
//	if (dentry->d_op && dentry->d_op->d_release)
//		dentry->d_op->d_release(dentry);
//  
//	/* if dentry was never visible to RCU, immediate free is OK */
//	if (!(dentry->d_flags & DCACHE_RCUACCESS))
//		__d_free(&dentry->d_u.d_rcu);
//	else
//		call_rcu(&dentry->d_u.d_rcu, __d_free);
  free((char *)dentry->d_name.name);
  free(dentry);
  
#ifdef CINQ_DEBUG
  atomic_dec(&num_dentry_);
#endif // CINQ_DEBUG
  
	return parent;
}

/*
 * Finish off a dentry we've decided to kill.
 * dentry->d_lock must be held, returns with it unlocked.
 * If ref is non-zero, then decrement the refcount too.
 * Returns dentry requiring refcount drop, or NULL if we're done.
 */
static inline struct dentry *dentry_kill_(struct dentry *dentry, int ref)
	__releases(dentry->d_lock) {

	struct inode *inode;
	struct dentry *parent;
  
	inode = dentry->d_inode;
	if (inode && !spin_trylock(&inode->i_lock)) {
relock:
		spin_unlock(&dentry->d_lock);
//		cpu_relax();
		return dentry; /* try again with same dentry */
	}
  
	if (IS_ROOT(dentry))
		parent = NULL;
	else
		parent = dentry->d_parent;
  
	if (parent && !spin_trylock(&parent->d_lock)) {
		if (inode)
			spin_unlock(&inode->i_lock);
		goto relock;
	}
  
	if (ref)
		dentry->d_count--;
  
//	dentry_lru_del(dentry);
//	__d_drop(dentry);

	return d_kill_(dentry, parent);
}

/* 
 * This is dput
 *
 * This is complicated by the fact that we do not want to put
 * dentries that are no longer on any hash chain on the unused
 * list: we'd much rather just get rid of them immediately.
 *
 * However, that implies that we have to traverse the dentry
 * tree upwards to the parents which might _also_ now be
 * scheduled for deletion (it may have been only waiting for
 * its last child to go away).
 *
 * This tail recursion is done by hand as we don't want to depend
 * on the compiler to always get this right (gcc generally doesn't).
 * Real recursion would eat up our stack space.
 */

/**
 *      dget, dget_locked       -       get a reference to a dentry
 *      @dentry: dentry to get a reference to
 *
 *      Given a dentry or %NULL pointer increment the reference count
 *      if appropriate and return the dentry. A dentry will not be 
 *      destroyed when it has references. dget() should never be
 *      called for dentries with zero reference counter. For these cases
 *      (preferably none, functions in dcache.c are sufficient for normal
 *      needs and they take necessary precautions) you should hold dcache_lock
 *      and call dget_locked() instead of dget().
 */  
struct dentry *dget(struct dentry *dentry) {
  if (dentry) {
    spin_lock(&dentry->d_lock);
    dentry->d_count++;
    spin_unlock(&dentry->d_lock);
  }
  return dentry;
}

/*
 * dput - release a dentry
 * @dentry: dentry to release 
 *
 * Release a dentry. This will drop the usage count and if appropriate
 * call the dentry unlink method as well as removing it from the queues and
 * releasing its resources. If the parent dentries were scheduled for release
 * they too may now get deleted.
 */
void dput(struct dentry *dentry) {
	if (!dentry)
		return;
  
repeat:
//	if (dentry->d_count == 1)
//		might_sleep();
	spin_lock(&dentry->d_lock);
	DEBUG_ON_(!dentry->d_count, "[Error@dput_] put bad dentry: %d.\n",
          dentry->d_count);
	if (dentry->d_count > 1) {
		dentry->d_count--;
		spin_unlock(&dentry->d_lock);
		return;
	}
  
//	if (dentry->d_flags & DCACHE_OP_DELETE) {
//		if (dentry->d_op->d_delete(dentry))
//			goto kill_it;
//	}
  
// 	if (d_unhashed(dentry))
//		goto kill_it;
  
	/* Otherwise leave it cached and ensure it's on the LRU */
//	dentry->d_flags |= DCACHE_REFERENCED;
//	dentry_lru_add(dentry);
//  
//	dentry->d_count--;
//	spin_unlock(&dentry->d_lock);
//	return;
  
//kill_it:
	dentry = dentry_kill_(dentry, 1);
	if (dentry)
		goto repeat;
}

// Invoked when super block is killed
void d_genocide(struct dentry *root) {
  if (!root) return;
  // Deal with its children... This is only a reference implementation
  // Kernel avoids recursive function. Recomended.
  struct dentry *cur, *tmp;
  list_for_each_entry_safe(cur, tmp, &root->d_subdirs, d_u.d_child) {
    d_genocide(cur);
  }
  dput(root);
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

/* Find an unused file structure and return a pointer to it.
 * Returns NULL, if there are no more free file structures or
 * we run out of memory.
 *
 * Be very careful using this.  You are responsible for
 * getting write access to any mount that you might assign
 * to this filp, if it is opened for write.  If this is not
 * done, you will imbalance int the mount's writer count
 * and a warning at __fput() time.
 */
struct file *get_empty_filp() {
//	const struct cred *cred = current_cred();
//	static long old_max;
	struct file * f;
  
	/*
	 * Privileged users can go above max_files
	 */
//	if (get_nr_files() >= files_stat.max_files && !capable(CAP_SYS_ADMIN)) {
//		/*
//		 * percpu_counters are inaccurate.  Do an expensive check before
//		 * we go and fail.
//		 */
//		if (percpu_counter_sum_positive(&nr_files) >= files_stat.max_files)
//			goto over;
//	}
  
	// f = kmem_cache_zalloc(filp_cachep, GFP_KERNEL);
  f = (struct file *)malloc(sizeof(struct file));
	if (f == NULL)
		goto fail;
  
	INIT_LIST_HEAD(&f->f_u.fu_list);
//	atomic_long_set(&f->f_count, 1);
//	rwlock_init(&f->f_owner.lock);
	spin_lock_init(&f->f_lock);
//	eventpoll_init_file(f);
	/* f->f_version: 0 */
	return f;
  
//over:
//	/* Ran out of filps - report that */
//	if (get_nr_files() > old_max) {
//		pr_info("VFS: file-max limit %lu reached\n", get_max_files());
//		old_max = get_nr_files();
//	}
//	goto fail;
  
//fail_sec:
//	file_free(f);
fail:
	return NULL;
}

void put_filp(struct file *file)
{
//  if (atomic_long_dec_and_test(&file->f_count)) {
//    security_file_free(file);
//    file_sb_list_del(file);
//    file_free(file);
      free(file);
//  }
}

#endif // __KERNEL__
