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

#define log(...) (fprintf(stderr, __VA_ARGS__))

#define fsnode_malloc() \
    ((struct cinq_fsnode *)malloc(sizeof(struct cinq_fsnode)))
#define fsnode_mfree(p) (free(p))

#define cinq_free free

/* Private */

#endif // __KERNEL__

#endif // CINQUAIN_META_UTIL_H_
