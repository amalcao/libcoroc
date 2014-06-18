#ifndef _TSC_TIME_H_
#define _TSC_TIME_H_

#include <sys/time.h>

#include "support.h"
#include "vpu.h"
#include "channel.h"
#include "coroutine.h"

struct tsc_timer;

/* internal timer type */
typedef struct {
  uint64_t when;
  uint32_t period;
  int32_t index;
  void (*func)(void *);
  void *args;
  struct tsc_timer *owner;
} tsc_inter_timer_t;

/* userspace timer type */
typedef struct tsc_timer {
  struct tsc_buffered_chan _chan;
  uint64_t _buffer;
  tsc_inter_timer_t timer;
} *tsc_timer_t;

/* userspace api */

tsc_timer_t tsc_timer_allocate(uint32_t period, void (*func)(void));
void tsc_timer_dealloc(tsc_timer_t);
tsc_chan_t tsc_timer_after(tsc_timer_t, uint64_t);
tsc_chan_t tsc_timer_at(tsc_timer_t, uint64_t);

int tsc_timer_start(tsc_timer_t);
int tsc_timer_stop(tsc_timer_t);
int tsc_timer_reset(tsc_timer_t, uint64_t);

/* internal api */
int tsc_add_intertimer(tsc_inter_timer_t *);
int tsc_del_intertimer(tsc_inter_timer_t *);

/* get current time */
static inline int64_t tsc_getmicrotime(void) {
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

static inline int64_t tsc_getnanotime(void) {
  struct timespec abstime;
  clock_gettime(CLOCK_REALTIME, &abstime);
  return abstime.tv_sec * 1000000000LL + abstime.tv_nsec;
}

#endif  // _TSC_TIME_H_
