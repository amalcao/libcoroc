// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <sys/time.h>

#include "coroc_time.h"
#include "notify.h"

void coroc_notify_wakeup(coroc_notify_t *note) {
  if (TSC_CAS(&note->key, 0, 1))
    _coroc_futex_wakeup(&note->key, 1);
}

void __coroc_notify_tsleep(void *arg) {
  coroc_notify_t *note = (coroc_notify_t *)arg;
  int64_t ns = note->ns;
  int64_t now = coroc_getnanotime();
  int64_t deadline = ns + now;

  for (;;) {
    _coroc_futex_sleep(&note->key, 0, ns);
    if (TSC_ATOMIC_READ(note->key) != 0) break;
    now = coroc_getnanotime();
    if (now >= deadline) break;
    ns = deadline - now;
  }
}

// ----------------------------------

void __coroc_nanosleep(void *pns) {
  uint64_t ns = *(uint64_t *)pns;
  struct timespec req, rem;

  req.tv_sec = ns / 1000000000;
  req.tv_nsec = ns % 1000000000;

  nanosleep(&req, &rem);
}
