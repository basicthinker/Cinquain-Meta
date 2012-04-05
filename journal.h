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
#include "list.h"

enum journal_action {
  CREATE = 0,
  UPDATE = 1,
  DELETE = 2
};

struct journal_entry {
  void *key;
  void *data;
  enum journal_action action;
  struct list_head list;
};

static inline struct journal_entry *journal_entry_new(void *key, void *data,
                                                      enum journal_action action) {
  struct journal_entry *new_entry = journal_entry_malloc_();
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
  spin_lock(&journal->lock);
  int ret = list_empty(&journal->list);
  spin_unlock(&journal->lock);
  return ret;
}

static inline struct journal_entry *journal_get_syn(struct cinq_journal *journal) {
  spin_lock(&journal->lock);
  struct list_head *head = journal->list.next;
  list_del(head);
  spin_unlock(&journal->lock);
  return list_entry(head, struct journal_entry, list);
}

static inline void journal_add_syn(struct cinq_journal *journal,
                                   struct journal_entry *entry) {
  spin_lock(&journal->lock);
  list_add_tail(&entry->list, &journal->list);
  spin_unlock(&journal->lock);
}

#endif // CINQUAIN_META_LOG_H_
