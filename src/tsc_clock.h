#ifndef _TSC_CORE_CLOCK_H
#define _TSC_CORE_CLOCK_H

#include "support.h"
#include "tsc_queue.h"

typedef struct clock_manager {
  queue_t sleep_queue;  // TODO
} clock_manager_t;

extern clock_manager_t clock_manager;

extern void tsc_clock_initialize(void);
extern void clock_wait(uint32_t us);                          // TODO
extern void clock_deadline_inspector(void* item, int value);  // TODO
extern void clock_routine(void);

#endif  // _TSC_CORE_CLOCK_H
