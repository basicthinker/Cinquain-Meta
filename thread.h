/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  thread.h
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 4/4/12.
//

#ifndef CINQUAIN_META_THREAD_H_
#define CINQUAIN_META_THREAD_H_

#ifdef __KERNEL__
/* Kernel-space definition */

#include <linux/kthread.h>
#include <linux/delay.h>

typedef struct task_struct thread_t;

#define THREAD_FUNC_(name) \
    int (name)
#define THREAD_RETURN_ return 0

#define thread_should_stop() pthread_should_stop()

#else
/* User-space definition */

#include <pthread.h>

typedef pthread_t thread_t;

#define THREAD_FUNC_(name) \
    void *(name)
#define THREAD_RETURN_ pthread_exit(NULL)

#define set_current_state(state)
#define thread_should_stop() 0
#define msleep(msec) sleep(msec / 1000)

#endif // __KERNEL__

struct thread_task {
  thread_t *thread;
  THREAD_FUNC_(*func)(void *);
  void *data;
  char *name;
};

static void thread_init(struct thread_task *thr_task,
                        THREAD_FUNC_(*func)(void *data),
                        void *data, char *name);
static void thread_run(struct thread_task *thr_task);

static int thread_stop(struct thread_task *thr_task);

#ifdef __KERNEL__
/* Kernel-space functions */

static inline void thread_init(struct thread_task *thr_task,
                                THREAD_FUNC_(*func)(void *data),
                                void *data, char *name) {
  thr_task->thread = kthread_create(func, data, name);
}

static inline void thread_run(struct thread_task *thr_task) {
  if (!IS_ERR(thr_task->thread)) {
    wake_up_process(thr_task->thread);
  } else {
    DEBUG_("[Error@thread_run_] failed to wake up.\n");
  }
}

static inline int thread_stop(struct thread_task *thr_task) {
  return kthread_stop(thr_task->thread);
}

#else
/* User-space functions */

static inline void thread_init(struct thread_task *thr_task,
                         THREAD_FUNC_(*func)(void *data),
                         void *data, char *name) {
  thr_task->thread = (thread_t *)malloc(sizeof(thread_t));
  memset(thr_task->thread, 0, sizeof(thread_t));
  thr_task->func = func;
  thr_task->data = data;
  thr_task->name = name;
}

static inline void thread_run(struct thread_task *thr_task) {
  int err = pthread_create(thr_task->thread, NULL,
                           thr_task->func, thr_task->data);
  DEBUG_ON_(err,
            "[Error@thread_run_] error code of pthread_create: %d.\n", err);
}

static inline int thread_stop(struct thread_task *thr_task) {
  return pthread_cancel(*thr_task->thread);
}

#endif // __KERNEL__

#endif // CINQUAIN_META_THREAD_H_
