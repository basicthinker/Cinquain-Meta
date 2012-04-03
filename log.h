/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  log.h
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 4/3/12.
//

#ifndef CINQUAIN_META_LOG_H_
#define CINQUAIN_META_LOG_H_

#include "util.h"
#include "list.h"

enum log_action {
  CREATE = 0,
  UPDATE = 1,
  DELETE = 2
};

struct log_entry {
  void *key;
  void *data;
  enum log_action action;
  struct list_head list;
};

static inline struct log_entry *log_entry_new(void *key, void *data,
                                               enum log_action action) {
  struct log_entry *new_entry = log_entry_malloc_();
  new_entry->key = key;
  new_entry->data = data;
  new_entry->action = action;
  return new_entry;
}

struct cinq_log {
  char *name;
  int key_size;
  int data_size;
  struct list_head list;
  spinlock_t lock;
};

static inline void log_new(char *name, int key_size, int data_size) {
  struct cinq_log *new_log = log_malloc_();
  INIT_LIST_HEAD(&new_log->list);
  spin_lock_init(&new_log->lock);
  new_log->name = name;
  new_log->key_size = key_size;
  new_log->data_size = data_size;
}

static inline int log_empty(struct cinq_log *log) {
  spin_lock(&log->lock);
  int ret = list_empty(&log->list);
  spin_unlock(&log->lock);
  return ret;
}

static inline struct log_entry *log_get_syn(struct cinq_log *log) {
  spin_lock(&log->lock);
  struct list_head *head = log->list.next;
  list_del(head);
  spin_unlock(&log->lock);
  return list_entry(head, struct log_entry, list);
}

static inline void log_add_syn(struct cinq_log *log,
                                     struct log_entry *entry) {
  spin_lock(&log->lock);
  list_add_tail(&entry->list, &log->list);
  spin_unlock(&log->lock);
}

THREAD_FUNC_(log_writeback)(void *data) {
  struct cinq_log *log = data;
  struct log_entry *entry;
  
  while (1) {
    set_current_state(TASK_RUNNING);
    
    while (!log_empty(log)) {
      entry = log_get_syn(log);
      char key_str[log->key_size + 1];
      strncpy(key_str, entry->key, log->key_size);
      key_str[log->key_size] = '\0';
      switch (entry->action) {
        case CREATE:
          fprintf(stdout, "log for %s - CREATE key '%s'.\n",
                  log->name, key_str);
          break;
        case UPDATE:
          fprintf(stdout, "log for %s - UPDATE key '%s'.\n",
                  log->name, key_str);
          break;
        case DELETE:
          fprintf(stdout, "log for %s - UPDATE key '%s'.\n",
                  log->name, key_str);
          break;
        default:
          DEBUG_("[Error@log_writeback] log entry action is NOT valid: %d.\n",
                 entry->action);
      }
    }
    
    set_current_state(TASK_INTERRUPTIBLE);
    msleep(1000);
  }
  THREAD_RETURN_;
}

#endif // CINQUAIN_META_LOG_H_
