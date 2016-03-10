// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <sys/time.h>

#include "tsc_time.h"
#include "notify.h"

void tsc_notify_wakeup(tsc_notify_t *note) {
  if (TSC_CAS(&note->key, 0, 1))
    _tsc_futex_wakeup(&note->key, 1);
}

void __tsc_notify_tsleep(void *arg) {
  tsc_notify_t *note = (tsc_notify_t *)arg;
  int64_t ns = note->ns;
  int64_t now = tsc_getnanotime();
  int64_t deadline = ns + now;

  for (;;) {
    _tsc_futex_sleep(&note->key, 0, ns);
    if (TSC_ATOMIC_READ(note->key) != 0) break;
    now = tsc_getnanotime();
    if (now >= deadline) break;
    ns = deadline - now;
  }
}

// ----------------------------------

void __tsc_nanosleep(void *pns) {
  uint64_t ns = *(uint64_t *)pns;
  struct timespec req, rem;

  req.tv_sec = ns / 1000000000;
  req.tv_nsec = ns % 1000000000;

  nanosleep(&req, &rem);
}
