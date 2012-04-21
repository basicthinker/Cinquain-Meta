/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  journal.h
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 4/3/12.
//

#ifndef CINQUAIN_META_LOG_H_
#define CINQUAIN_META_LOG_H_

#include "util.h"

enum journal_action {
  CREATE = 0,
  UPDATE = 1,
  DELETE = 2
};

struct cinq_jentry {
  void *key;
  void *data;
  enum journal_action action;
  struct list_head list;
};

#ifdef __KERNEL__

extern struct kmem_cache *cinq_jentry_cachep;

#define jentry_malloc_() \
    ((struct cinq_jentry *)kmem_cache_alloc(cinq_jentry_cachep, GFP_KERNEL))
#define jentry_free_(p) (kmem_cache_free(cinq_jentry_cachep, p))

#else

#define jentry_malloc_() \
    ((struct cinq_jentry *)malloc(sizeof(struct cinq_jentry)))
#define jentry_free_(p) (free(p))

#endif // __KERNEL__

static inline struct cinq_jentry *journal_entry_new(void *key, void *data,
                                                      enum journal_action action) {
  struct cinq_jentry *new_entry = jentry_malloc_();
  new_entry->key = key;
  new_entry->data = data;
  new_entry->action = action;
  return new_entry;
}

struct cinq_journal {
  char *name;
  struct list_head list;
  spinlock_t lock;
};

static inline void journal_init(struct cinq_journal *journal, char *name) {
  INIT_LIST_HEAD(&journal->list);
  spin_lock_init(&journal->lock);
  journal->name = name;
}

static inline int journal_empty_syn(struct cinq_journal *journal) {
  int ret;
  spin_lock(&journal->lock);
  ret = list_empty(&journal->list);
  spin_unlock(&journal->lock);
  return ret;
}

static inline struct cinq_jentry *journal_get_syn(struct cinq_journal *journal) {
  struct list_head *head;
  spin_lock(&journal->lock);
  head = journal->list.next;
  list_del(head);
  spin_unlock(&journal->lock);
  return list_entry(head, struct cinq_jentry, list);
}

static inline void journal_add_syn(struct cinq_journal *journal,
                                   struct cinq_jentry *entry) {
  spin_lock(&journal->lock);
  list_add_tail(&entry->list, &journal->list);
  spin_unlock(&journal->lock);
}


#ifdef __KERNEL__

static int init_jentry_cache(void) {
  cinq_jentry_cachep = kmem_cache_create(
      "cinq_jentry_cache", sizeof(struct cinq_jentry), 0,
      (SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD), NULL);
  if (cinq_jentry_cachep == NULL)
    return -ENOMEM;
  return 0;
}

static void destroy_jentry_cache(void) {
  kmem_cache_destroy(cinq_jentry_cachep);
}

#endif // __KERNEL__

#endif // CINQUAIN_META_LOG_H_
