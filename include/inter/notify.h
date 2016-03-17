// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_SUPPORT_NOTIFY_H_
#define _TSC_SUPPORT_NOTIFY_H_

#include <pthread.h>

#include "support.h"
#include "async.h"

// the notify API, just wrap the pthread_cont_t now!
typedef struct {
  uint32_t key;
  int64_t ns;
} coroc_notify_t;

// the internal sleep / tsleep

void coroc_notify_wakeup(coroc_notify_t *note);
void __coroc_notify_tsleep(void *);

static inline void coroc_notify_clear(coroc_notify_t *note) {
  note->key = 0;
  note->ns = 0;
}

static inline void coroc_notify_tsleep(coroc_notify_t *note, int64_t ns) {
  if (ns > 0) {
    note->ns = ns;
    __coroc_notify_tsleep(note);
  }
}

static inline void coroc_notify_tsleep_u(coroc_notify_t *note, int64_t ns) {
  // check the waiting time first
  if (ns > 0) {
    // waiting for the signal ..
    note->ns = ns;
    coroc_async_request_submit(__coroc_notify_tsleep, note);
  }
}

// user task space nanosleep call
extern void __coroc_nanosleep(void *);
static inline void coroc_nanosleep(uint64_t ns) {
  if (ns == 0) return;
  coroc_async_request_submit(__coroc_nanosleep, &ns);
}

#endif  // _TSC_SUPPORT_NOTIFY_H_
