/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  util.h
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/10/12.
//

#ifndef CINQUAIN_META_UTIL_H_
#define CINQUAIN_META_UTIL_H_

/* All kernel portability issues are addressed in this header. */

#ifdef __KERNEL__ // intended for Linux
/* Kernel (exchangable) */

#else
/* User space (exchangable) */
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#define log(...) (fprintf(stderr, __VA_ARGS__))

#define fsnode_malloc() \
    ((struct cinq_fsnode *)malloc(sizeof(struct cinq_fsnode)))
#define fsnode_mfree(p) (free(p))
#define cinq_free free

#endif // __KERNEL__


#ifndef __KERNEL__
// Private: those in kernel but not user-space

typedef unsigned int umode_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef uint32_t __u32;
typedef long long loff_t;

struct qstr { // include/linux/dcache.h
  unsigned int hash;
  unsigned int len;
  const unsigned char *name;
};

typedef struct cinq_inode inode_t;

#endif // __KERNEL__

#endif // CINQUAIN_META_UTIL_H_
