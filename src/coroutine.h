#ifndef _TSC_CORE_COROUTINE_H_
#define _TSC_CORE_COROUTINE_H_

#include <stdint.h>

#include "support.h"
#include "context.h"
#include "queue.h"
#include "message.h"

typedef int32_t (*tsc_coroutine_handler_t)(void* args);

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

  uint32_t type;
  uint32_t status;
  uint32_t vpu_affinity;
  uint32_t vpu_id;

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
  void* arguments;
  int32_t retval;
  TSC_CONTEXT ctx;
}* tsc_coroutine_t;

enum tsc_coroutine_status {
  TSC_COROUTINE_SLEEP = 0xBAFF,
  TSC_COROUTINE_READY = 0xFACE,
  TSC_COROUTINE_RUNNING = 0xBEEF,
  TSC_COROUTINE_WAIT = 0xBADD,
  TSC_COROUTINE_EXIT = 0xDEAD,
};

enum tsc_coroutine_type {
  TSC_COROUTINE_IDLE = 0x0,
  TSC_COROUTINE_NORMAL = 0x1,
  TSC_COROUTINE_MAIN = 0x2,
};

enum tsc_coroutine_deallocate {
  TSC_COROUTINE_UNDETACH = 0x0,
  TSC_COROUTINE_DETACH = 0x1,
};

extern tsc_coroutine_t tsc_coroutine_allocate(tsc_coroutine_handler_t entry,
                                              void* arguments, const char* name,
                                              uint32_t type,
                                              tsc_coroutine_attributes_t* attr);
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
                         (name), TSC_COROUTINE_NORMAL, NULL)

// -- interfaces for tsc_coroutine_attributes --
extern void tsc_coroutine_attr_init(tsc_coroutine_attributes_t* attr);

static inline void tsc_coroutine_attr_set_stacksize(
    tsc_coroutine_attributes_t* attr, uint32_t sz) {
  attr->stack_size = sz;
}
static inline void tsc_coroutine_attr_set_timeslice(
    tsc_coroutine_attributes_t* attr, uint32_t slice) {
  attr->timeslice = slice;
}
static inline void tsc_coroutine_attr_set_affinity(
    tsc_coroutine_attributes_t* attr, uint32_t affinity) {
  attr->affinity = affinity;
}

#endif  // _TSC_CORE_COROUTINE_H_
