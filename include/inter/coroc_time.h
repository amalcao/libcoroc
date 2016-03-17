// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_TIME_H_
#define _TSC_TIME_H_

#include <time.h>
#include <sys/time.h>

#include "support.h"
#include "vpu.h"
#include "channel.h"
#include "coroutine.h"

struct coroc_timer;

/* internal timer type */
typedef struct {
  uint64_t when;
  uint32_t period;
  int32_t index;
  void (*func)(void *);
  void *args;
  struct coroc_timer *owner;
} coroc_inter_timer_t;

/* userspace timer type */
typedef struct coroc_timer {
  struct coroc_buffered_chan _chan;
  uint64_t _buffer;
  coroc_inter_timer_t timer;
} *coroc_timer_t;

/* userspace api */

coroc_timer_t coroc_timer_allocate(uint32_t period, void (*func)(void));
void coroc_timer_dealloc(coroc_timer_t);
coroc_chan_t coroc_timer_after(coroc_timer_t, uint64_t);
coroc_chan_t coroc_timer_at(coroc_timer_t, uint64_t);

int coroc_timer_start(coroc_timer_t);
int coroc_timer_stop(coroc_timer_t);
int coroc_timer_reset(coroc_timer_t, uint64_t);

/* internal api */
int coroc_add_intertimer(coroc_inter_timer_t *);
int coroc_del_intertimer(coroc_inter_timer_t *);

/* get current time */
static inline int64_t coroc_getmicrotime(void) {
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

static inline int64_t coroc_getnanotime(void) {
  struct timespec abstime;
  clock_gettime(CLOCK_REALTIME, &abstime);
  return abstime.tv_sec * 1000000000LL + abstime.tv_nsec;
}

void coroc_udelay(uint64_t us);

#endif  // _TSC_TIME_H_
