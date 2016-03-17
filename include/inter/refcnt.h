// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_REFCNT_H_
#define _TSC_REFCNT_H_

#include <stdint.h>
#include <stdbool.h>

typedef void (*release_handler_t)(void *);

typedef struct coroc_refcnt {
  uint32_t count;
  release_handler_t release;
} *coroc_refcnt_t;

static inline void coroc_refcnt_init(coroc_refcnt_t ref,
                                   release_handler_t release) {
  ref->count = 1;
  ref->release = release;
}

static inline void *__coroc_refcnt_get(coroc_refcnt_t ref) {
  TSC_ATOMIC_INC(ref->count);
  return (void *)ref;
}

static inline bool __coroc_refcnt_put(coroc_refcnt_t ref) {
  if (ref != NULL && TSC_ATOMIC_DEC(ref->count) == 0) {
    (ref->release)(ref);
    return true;
  }
  return false;
}

static inline coroc_refcnt_t 
__coroc_refcnt_assign(coroc_refcnt_t* to, coroc_refcnt_t from) {
  __coroc_refcnt_put(*to);
  *to = (from == NULL) ? NULL : __coroc_refcnt_get(from);
  return *to;
}

static inline coroc_refcnt_t
__coroc_refcnt_assign_expr(coroc_refcnt_t* to, coroc_refcnt_t from) {
  __coroc_refcnt_put(*to);
  return (*to = from);
}


#define coroc_refcnt_get(ref) (typeof(ref)) __coroc_refcnt_get((coroc_refcnt_t)(ref))
#define coroc_refcnt_put(ref) __coroc_refcnt_put((coroc_refcnt_t)(ref))

#define coroc_refcnt_assign(to, from) \
         (typeof(to)) __coroc_refcnt_assign((coroc_refcnt_t*)(&(to)), (coroc_refcnt_t)(from))

#define coroc_refcnt_assign_expr(to, from) \
        (typeof(to)) __coroc_refcnt_assign_expr((coroc_refcnt_t*)(&(to)), (coroc_refcnt_t)(from))

#endif  // _TSC_REFCNT_H_
