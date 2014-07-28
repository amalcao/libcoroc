#ifndef _DARWIN_TIME_H_
#define _DARWIN_TIME_H_
#include <sys/time.h>

enum {
  CLOCK_REALTIME,
  CLOCK_REALTIME_COARSE,
  CLOCK_MONOTONIC,
  CLOCK_MONOTONIC_COARSE,
  CLOCK_MONOTONIC_RAW,
  CLOCK_BOOTTIME,
  CLOCK_PROCESS_CPUTIME_ID,
  CLOCK_THREAD_CPUTIME_ID
};

/// clock_gettime is not implemented in OS X
static int clock_gettime(int clk_id, struct timespec* t) {
  struct timeval now;
  int rv = gettimeofday(&now, NULL);
  if (rv) return rv;
  t->tv_sec = now.tv_sec;
  t->tv_nsec = now.tv_usec * 1000;
  return 0;
}

#endif // _DARWIN_TIME_H_
