#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syscall.h>
#include <linux/futex.h>

#include "coroutine.h"

void _tsc_futex_sleep(uint32_t *addr, uint32_t val, int64_t ns) {
  struct timespec ts;
  int32_t nsec;

  if (ns < 0) {
    syscall(__NR_futex, addr, FUTEX_WAIT, val, NULL, NULL, 0);
    return;
  }

  ts.tv_sec = ns / 1000000000LL;
  ts.tv_nsec = ns % 1000000000LL;
  syscall(__NR_futex, addr, FUTEX_WAIT, val, &ts, NULL, 0);
}

void _tsc_futex_wakeup(uint32_t *addr, uint32_t cnt) {
  int64_t ret;

  ret = syscall(__NR_futex, addr, FUTEX_WAKE, cnt, NULL, NULL, 0);

#ifdef ENABLE_DEBUG
  tsc_coroutine_t self = tsc_coroutine_self();
  uint64_t coid = self ? self->id : 0;
  TSC_DEBUG("[futex wakeup %p] ret = %ld coid = %ld\n", addr, ret, coid);
#endif

  if (ret >= 0) return;

  // can never reach here!!
  assert(0);
}
