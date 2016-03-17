// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.


#ifndef _LIBTSC_DNA_CORE_VPU_H_
#define _LIBTSC_DNA_CORE_VPU_H_

#include <stdint.h>
#include "coroutine.h"
#include "support.h"
#include "coroc_queue.h"

// The unlock handler type
typedef void (*unlock_handler_t)(volatile void *lock);

// Type of the private task queue
typedef struct private_task_queue {
  unsigned prio; // the priority level for this pq
  coroc_coroutine_t runq[TSC_TASK_NUM_PERPRIO];
  uint32_t runqhead;
  uint32_t runqtail;
} p_task_que;

// Type of VPU information,
//  It's a OS Thread here!!
typedef struct vpu {
  TSC_OS_THREAD_T os_thr;
  uint32_t id;
  bool initialized;
  uint32_t watchdog;
  uint32_t ticks;
  unsigned rand_seed;
  coroc_coroutine_t current;
  coroc_coroutine_t scheduler;
  
  // the private queues for each priority level:
  p_task_que xt[TSC_PRIO_NUM];

  void *hold;
  unlock_handler_t unlock_handler;
} vpu_t;

// Type of the VPU manager
typedef struct vpu_manager {
  vpu_t *vpu;
  
  // the global running queue for each priority
  queue_t xt[TSC_PRIO_NUM];
  // total ready tasks for each priority
  uint32_t ready[TSC_PRIO_NUM + 1];

  uint32_t xt_index;
  uint32_t last_pid;
  coroc_coroutine_t main;
  queue_t coroutine_list;
  pthread_cond_t cond;
  pthread_mutex_t lock;
  uint16_t alive;
  uint16_t idle;
  uint32_t total_ready;
  uint32_t total_iowait;
} vpu_manager_t;

extern vpu_manager_t vpu_manager;

extern int core_wait(void *);
extern int core_yield(void *);
extern int core_exit(void *);

extern void coroc_vpu_initialize(int cpu_mp_count, coroc_coroutine_handler_t entry);

extern void vpu_suspend(volatile void *lock, unlock_handler_t handler);
extern void vpu_ready(coroc_coroutine_t coroutine, bool);
extern void vpu_syscall(int (*pfn)(void *));
extern void vpu_clock_handler(int);
extern void vpu_wakeup_one(void);
extern void vpu_backtrace(vpu_t*);

#define TSC_ALLOC_TID() TSC_ATOMIC_INC(vpu_manager.last_pid)

#endif  // _LIBTSC_DNA_CORE_VPU_H_
