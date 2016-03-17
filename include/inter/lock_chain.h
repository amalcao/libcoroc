// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_LOCK_CHAIN_H_
#define _TSC_LOCK_CHAIN_H_

#include <stdlib.h>
#include "coroc_lock.h"

#define TSC_LOCKCHAIN_DEFAULT_VOLUME 8

typedef struct {
  bool sorted;
  int volume;
  int size;
  lock_t *locks;
} lock_chain_t;

static int lock_addr_comp(const void *lock1, const void *lock2) {
  return *(uint64_t *)(lock1) > *(uint64_t *)(lock2) ? 1 : -1;
}

static void lock_chain_acquire(lock_chain_t *chain) {
  if (!chain->sorted) {
    qsort(chain->locks, chain->size, sizeof(lock_t), lock_addr_comp);
    chain->sorted = true;
  }

  int i = 0;
  for (; i < chain->size; i++) lock_acquire(chain->locks[i]);
}

static void lock_chain_release(lock_chain_t *chain) {
  int i = chain->size - 1;
  for (; i >= 0; i--) lock_release(chain->locks[i]);
}

#endif  // _TSC_LOCK_CHAIN_H_
