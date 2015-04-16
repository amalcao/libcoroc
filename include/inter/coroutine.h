#ifndef _TSC_CORE_COROUTINE_H_
#define _TSC_CORE_COROUTINE_H_

#include <stdint.h>

#include "support.h"
#include "context.h"
#include "message.h"
#include "tsc_queue.h"

typedef int32_t (*tsc_coroutine_handler_t)(void*);
typedef int32_t (*tsc_coroutine_cleanup_t)(void*, int);

typedef int32_t tsc_coroutine_id_t;

struct vpu;

typedef struct tsc_coroutine_attributes {
  uint32_t stack_size;
  uint32_t timeslice;
  uint32_t affinity;
  // TODO : anything else ??
} tsc_coroutine_attributes_t;

typedef struct tsc_coroutine {
  struct tsc_async_chan _chan;

  tsc_coroutine_id_t id;
  tsc_coroutine_id_t pid;  // DEBUG
  char name[TSC_NAME_LENGTH];

  uint32_t type:8;
  uint32_t vpu_id:8;
  uint32_t vpu_affinity:8;
  uint32_t async_wait:8;

  uint32_t status;
  unsigned priority;

  queue_item_t status_link;
  queue_item_t trace_link;
  bool syscall;
  bool backtrace;  // wakeup for backtrace

  void* qtag;  // used by channel_select

  uint32_t init_timeslice;
  uint32_t rem_timeslice;

  int sigmask_nest;

  uint32_t detachstate;
  void* stack_base;
  size_t stack_size;
  tsc_coroutine_handler_t entry;
  tsc_coroutine_cleanup_t cleanup;
  void* arguments;
  int32_t retval;

  TSC_CONTEXT ctx;
}* tsc_coroutine_t;

enum tsc_coroutine_status {
  TSC_COROUTINE_SLEEP = 0xBAFF,
  TSC_COROUTINE_READY = 0xFACE,
  TSC_COROUTINE_RUNNING = 0xBEEF,
  TSC_COROUTINE_WAIT = 0xBADD,
  TSC_COROUTINE_IOWAIT = 0xBADE,
  TSC_COROUTINE_EXIT = 0xDEAD,
};

enum tsc_coroutine_type {
  TSC_COROUTINE_IDLE = 0x0,
  TSC_COROUTINE_NORMAL = 0x1,
  TSC_COROUTINE_MAIN = 0x2,
};

enum {
  TSC_PRIO_HIGHEST = 0,
  TSC_PRIO_HIGH = 1,
  TSC_PRIO_NORMAL = 2,
  TSC_PRIO_LOW = 3,
};

#define TSC_DEFAULT_PRIO TSC_PRIO_NORMAL

enum tsc_coroutine_deallocate {
  TSC_COROUTINE_UNDETACH = 0x0,
  TSC_COROUTINE_DETACH = 0x1,
};

extern tsc_coroutine_t 
tsc_coroutine_allocate(tsc_coroutine_handler_t entry,
                       void* arguments, const char* name,
                       uint32_t type, unsigned priority,
                       tsc_coroutine_cleanup_t cleanup);
extern void tsc_coroutine_deallocate(tsc_coroutine_t);
extern void tsc_coroutine_exit(int value);
extern void tsc_coroutine_yield(void);
extern tsc_coroutine_t tsc_coroutine_self(void);
extern void tsc_coroutine_backtrace(tsc_coroutine_t);
#ifdef TSC_ENABLE_COROUTINE_JOIN
extern status_t tsc_coroutine_join(tsc_coroutine_t);
extern void tsc_coroutine_detach(void);
#endif  // TSC_ENABLE_COROUTINE_JOIN

#define tsc_coroutine_spawn(entry, args, name)                            \
  tsc_coroutine_allocate((tsc_coroutine_handler_t)(entry), (void*)(args), \
                         (name), TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, NULL)


static inline void 
tsc_coroutine_set_priority(unsigned priority) {
  assert(priority < TSC_PRIO_NUM);
  tsc_coroutine_t me = tsc_coroutine_self();
  me->priority = priority;
}

static inline void 
tsc_coroutine_set_name(const char *name) {
  assert(name != NULL);
  tsc_coroutine_t me = tsc_coroutine_self();
  strncpy(me->name, name, sizeof(me->name) - 1);
}

#endif  // _TSC_CORE_COROUTINE_H_
