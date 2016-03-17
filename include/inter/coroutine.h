// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_CORE_COROUTINE_H_
#define _TSC_CORE_COROUTINE_H_

#include <stdint.h>

#include "support.h"
#include "context.h"
#include "message.h"
#include "coroc_queue.h"

typedef int32_t (*coroc_coroutine_handler_t)(void*);
typedef int32_t (*coroc_coroutine_cleanup_t)(void*, int);

typedef int32_t coroc_coroutine_id_t;

struct vpu;

typedef struct coroc_coroutine_attributes {
  uint32_t stack_size;
  uint32_t timeslice;
  uint32_t affinity;
  // TODO : anything else ??
} coroc_coroutine_attributes_t;

typedef struct coroc_coroutine {
  struct coroc_async_chan _chan;

  coroc_coroutine_id_t id;
  coroc_coroutine_id_t pid;  // DEBUG
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
  coroc_coroutine_handler_t entry;
  coroc_coroutine_cleanup_t cleanup;
  void* arguments;
  int32_t retval;

  TSC_CONTEXT ctx;
}* coroc_coroutine_t;

enum coroc_coroutine_status {
  TSC_COROUTINE_SLEEP = 0xBAFF,
  TSC_COROUTINE_READY = 0xFACE,
  TSC_COROUTINE_RUNNING = 0xBEEF,
  TSC_COROUTINE_WAIT = 0xBADD,
  TSC_COROUTINE_IOWAIT = 0xBADE,
  TSC_COROUTINE_EXIT = 0xDEAD,
};

enum coroc_coroutine_type {
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

enum coroc_coroutine_deallocate {
  TSC_COROUTINE_UNDETACH = 0x0,
  TSC_COROUTINE_DETACH = 0x1,
};

extern coroc_coroutine_t 
coroc_coroutine_allocate(coroc_coroutine_handler_t entry,
                       void* arguments, const char* name,
                       uint32_t type, unsigned priority,
                       coroc_coroutine_cleanup_t cleanup);
extern void coroc_coroutine_deallocate(coroc_coroutine_t);
extern void coroc_coroutine_exit(int value);
extern void coroc_coroutine_yield(void);
extern coroc_coroutine_t coroc_coroutine_self(void);
extern void coroc_coroutine_backtrace(coroc_coroutine_t);
#ifdef TSC_ENABLE_COROUTINE_JOIN
extern status_t coroc_coroutine_join(coroc_coroutine_t);
extern void coroc_coroutine_detach(void);
#endif  // TSC_ENABLE_COROUTINE_JOIN

#define coroc_coroutine_spawn(entry, args, name)                            \
  coroc_coroutine_allocate((coroc_coroutine_handler_t)(entry), (void*)(args), \
                         (name), TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, NULL)


static inline void 
coroc_coroutine_set_priority(unsigned priority) {
  assert(priority < TSC_PRIO_NUM);
  coroc_coroutine_t me = coroc_coroutine_self();
  me->priority = priority;
}

static inline void 
coroc_coroutine_set_name(const char *name) {
  assert(name != NULL);
  coroc_coroutine_t me = coroc_coroutine_self();
  strncpy(me->name, name, sizeof(me->name) - 1);
}

#endif  // _TSC_CORE_COROUTINE_H_
