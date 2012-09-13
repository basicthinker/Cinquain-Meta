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

#define NUM_WAY 128
#define WAY_MASK 0xff

enum journal_action {
  mknod = 0 // covers cinq_mknod, cinq_create
};

struct cinq_jentry {
  unsigned int sn;
  enum journal_action action;
  union data {
    struct {
      unsigned long ci_id;
      unsigned long fs_id;
      char *name;
      int mode;
    } mknod;
  };
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

static atomic_t g_counter = ATOMIC_INIT(0);

static inline struct cinq_jentry *jentry_new() {
	struct cinq_jentry *jentry = jentry_malloc_();
	jentry->sn = atomic_inc_return(&g_counter);
	return jentry;
}

static inline void jentry_free(struct cinq_jentry *jentry) {
  jentry_free_(jentry);
}

struct cinq_journal {
  char *name;
  struct list_head list[NUM_WAY];
  spinlock_t lock[NUM_WAY];
};

static inline void journal_init(struct cinq_journal *journal, char *name) {
  int i = 0;
  for (i = 0; i < NUM_WAY; ++i) {
	INIT_LIST_HEAD(&journal->list[i]);
    spin_lock_init(&journal->lock[i]);
  }
  journal->name = name;
}

static inline int journal_empty_syn(struct cinq_journal *journal, int way) {
  int ret;
  spin_lock(&journal->lock[way]);
  ret = list_empty(&journal->list[way]);
  spin_unlock(&journal->lock[way]);
  return ret;
}

static inline struct cinq_jentry *journal_get_syn(struct cinq_journal *journal, int way) {
  struct list_head *head;
  spin_lock(&journal->lock[way]);
  head = journal->list[way].next;
  list_del(head);
  spin_unlock(&journal->lock[way]);
  return list_entry(head, struct cinq_jentry, list);
}

static inline void journal_add_syn(struct cinq_journal *journal,
                                   struct cinq_jentry *entry) {
  int way = entry->sn & WAY_MASK;
  spin_lock(&journal->lock[way]);
  list_add_tail(&entry->list, &journal->list[way]);
  spin_unlock(&journal->lock[way]);
}


#ifdef __KERNEL__

static inline int init_jentry_cache(void) {
  cinq_jentry_cachep = kmem_cache_create(
      "cinq_jentry_cache", sizeof(struct cinq_jentry), 0,
      (SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD), NULL);
  if (cinq_jentry_cachep == NULL)
    return -ENOMEM;
  return 0;
}

static inline void destroy_jentry_cache(void) {
  kmem_cache_destroy(cinq_jentry_cachep);
}

#endif // __KERNEL__

#endif // CINQUAIN_META_LOG_H_
