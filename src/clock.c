// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include "vpu.h"
#include "tsc_clock.h"

#define TSC_CLOCK_PERIOD_NANOSEC 500000  // 0.5 ms per signal

extern bool __tsc_netpoll_polling(bool);

clock_manager_t clock_manager;

void tsc_clock_initialize(void) {
#ifdef ENABLE_TIMESHARE
  struct sigaction act;

  sigaddset(&act.sa_mask, TSC_CLOCK_SIGNAL);

  act.sa_handler = vpu_clock_handler;
  act.sa_flags = SA_RESTART;

  sigaction(TSC_CLOCK_SIGNAL, &act, NULL);
#endif
}

#define AVG(avg, cur, samples) \
  ((avg) * (double)(samples) + (double)(cur)) / ((double)(samples) + 1.0);

double avg_alive = 0;
double avg_idle = 0;
double avg_ready = 0;
double avg_total = 0;
double *avg_ready_pervpu;

static bool do_profile = false;

static uint32_t __tsc_get_vpu_ready(vpu_t *vpu) {
  uint64_t ready = 0;
  int prio;
  for (prio = 0; prio < TSC_PRIO_NUM; ++prio) {
    p_task_que *pq = & vpu->xt[prio];
    if (pq->runqtail > pq->runqhead)
      ready += (pq->runqtail - pq->runqhead);
  }
  return ready;
}

void tsc_profiler_print(int ret, void *p) {
  printf("\n\nThe average alive vpu number is %f\n", avg_alive);
  printf("The average idle vpu number is %f\n", avg_idle);
  printf("The average total tasks number is %f\n", avg_total);
  printf("The average total ready tasks number is %f\n", avg_ready);
  int i;
  for (i = 0; i < vpu_manager.xt_index; ++i) {
    printf("\tThe vpu %d avg ready tasks is %f\n", i, avg_ready_pervpu[i]);
  }

  //-----------------------------------------
  printf("\nThe current alive vpu number is %d\n", vpu_manager.alive);
  printf("The current idle vpu number is %d\n", vpu_manager.idle);
  printf("The current total tasks number is %d\n", vpu_manager.coroutine_list.status);
  printf("The current total ready tasks number is %d\n", vpu_manager.total_ready);
  for (i = 0; i < vpu_manager.xt_index; ++i) {
    printf("\tThe vpu %d cur ready tasks is %d\n", i, 
            __tsc_get_vpu_ready(& vpu_manager.vpu[i]));
  }

}



void tsc_profiler_handler(int signum) {
  exit(-1);
}

void tsc_profiler_initialize(int p) {
  
  do_profile = (p != 0);

  if (do_profile) {
    on_exit(tsc_profiler_print, NULL);

    signal(SIGINT, tsc_profiler_handler);
    signal(SIGPIPE, SIG_IGN);
  }
}

void clock_routine(void) {
  sigset_t sigmask;
  sigfillset(&sigmask);

  uint64_t samples = 0;

  avg_ready_pervpu = calloc(sizeof(double), vpu_manager.xt_index);

  for ( ;; samples++) {
    struct timespec period = {0, TSC_CLOCK_PERIOD_NANOSEC};
    pselect(0, NULL, NULL, NULL, &period, &sigmask);

    int index = 0;
#ifdef ENABLE_TIMESHARE
    for (; index < vpu_manager.xt_index; ++index) {
      TSC_OS_THREAD_SENDSIG(vpu_manager.vpu[index].os_thr, TSC_CLOCK_SIGNAL);
    }
    __tsc_netpoll_polling(0);
#endif  // ENABLE_TIMESHARE
    
    if (!do_profile) continue;

    // try to profiling the current system
    uint64_t cur = TSC_ATOMIC_READ(vpu_manager.total_ready);
    avg_ready = AVG(avg_ready, cur, samples);

    cur = vpu_manager.coroutine_list.status;
    avg_total = AVG(avg_total, cur, samples);

    cur = TSC_ATOMIC_READ(vpu_manager.alive);
    avg_alive = AVG(avg_alive, cur, samples);

    cur = TSC_ATOMIC_READ(vpu_manager.idle);
    avg_idle = AVG(avg_idle, cur, samples);
        
    for (index = 0; index < vpu_manager.xt_index; ++index) {
      cur = __tsc_get_vpu_ready(& vpu_manager.vpu[index]);
      avg_ready_pervpu[index] = 
        AVG(avg_ready_pervpu[index], cur, samples);
    }

    if (samples % 2000 == 0)
      tsc_profiler_print(0, NULL);
  }
}
