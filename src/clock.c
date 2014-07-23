#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include "clock.h"
#include "vpu.h"

#define TSC_CLOCK_PERIOD_NANOSEC 500000  // 0.5 ms per signal

extern bool __tsc_netpoll_polling(bool);

clock_manager_t clock_manager;

void tsc_clock_initialize(void) {
#ifdef ENABLE_TIMER
  struct sigaction act;

  sigaddset(&act.sa_mask, TSC_CLOCK_SIGNAL);

  act.sa_handler = vpu_clock_handler;
  act.sa_flags = SA_RESTART;

  sigaction(TSC_CLOCK_SIGNAL, &act, NULL);
#endif
}

void clock_routine(void) {
  sigset_t sigmask;
  sigfillset(&sigmask);

  while (true) {
#ifdef ENABLE_TIMER
    struct timespec period = {0, TSC_CLOCK_PERIOD_NANOSEC};
    pselect(0, NULL, NULL, NULL, &period, &sigmask);

    int index = 0;
    for (; index < vpu_manager.xt_index; ++index) {
      TSC_OS_THREAD_SENDSIG(vpu_manager.vpu[index].os_thr, TSC_CLOCK_SIGNAL);
    }
    __tsc_netpoll_polling(false);
#else
    __tsc_netpoll_polling(true);
#endif  // ENABLE_TIMER
  }
}
