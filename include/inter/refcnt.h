// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_REFCNT_H_
#define _TSC_REFCNT_H_

#include <stdint.h>
#include <stdbool.h>

typedef void (*release_handler_t)(void *);

typedef struct tsc_refcnt {
  uint32_t count;
  release_handler_t release;
} *tsc_refcnt_t;

static inline void tsc_refcnt_init(tsc_refcnt_t ref,
                                   release_handler_t release) {
  ref->count = 1;
  ref->release = release;
}

static inline void *__tsc_refcnt_get(tsc_refcnt_t ref) {
  TSC_ATOMIC_INC(ref->count);
  return (void *)ref;
}

static inline bool __tsc_refcnt_put(tsc_refcnt_t ref) {
  if (ref != NULL && TSC_ATOMIC_DEC(ref->count) == 0) {
    (ref->release)(ref);
    return true;
  }
  return false;
}

static inline tsc_refcnt_t 
__tsc_refcnt_assign(tsc_refcnt_t* to, tsc_refcnt_t from) {
  __tsc_refcnt_put(*to);
  *to = (from == NULL) ? NULL : __tsc_refcnt_get(from);
  return *to;
}

static inline tsc_refcnt_t
__tsc_refcnt_assign_expr(tsc_refcnt_t* to, tsc_refcnt_t from) {
  __tsc_refcnt_put(*to);
  return (*to = from);
}


#define tsc_refcnt_get(ref) (typeof(ref)) __tsc_refcnt_get((tsc_refcnt_t)(ref))
#define tsc_refcnt_put(ref) __tsc_refcnt_put((tsc_refcnt_t)(ref))

#define tsc_refcnt_assign(to, from) \
         (typeof(to)) __tsc_refcnt_assign((tsc_refcnt_t*)(&(to)), (tsc_refcnt_t)(from))

#define tsc_refcnt_assign_expr(to, from) \
        (typeof(to)) __tsc_refcnt_assign_expr((tsc_refcnt_t*)(&(to)), (tsc_refcnt_t)(from))

#endif  // _TSC_REFCNT_H_
