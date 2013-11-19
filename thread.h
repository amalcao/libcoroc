#ifndef _TSC_CORE_THREAD_H_
#define _TSC_CORE_THREAD_H_

#include <stdint.h>
#include "support.h"
#include "context.h"
#include "queue.h"

typedef int32_t (* thread_handler_t) (void *args);

typedef int32_t thread_id_t;

struct vpu;

typedef struct thread_attributes {
    uint32_t stack_size;
    uint32_t timeslice;
    uint32_t affinity;
    // TODO : anything else ??
} thread_attributes_t;

typedef struct thread {
    thread_id_t id;
    char name[TSC_NAME_LENGTH];

    uint32_t type;
    uint32_t status;
    uint32_t vpu_affinity;
    uint32_t vpu_id;

    queue_item_t status_link;
    queue_item_t sched_link;
    queue_t wait;
    queue_t message_queue;

    uint32_t init_timeslice;
    uint32_t rem_timeslice;

	uint32_t detachstate;
    void * stack_base;
    size_t stack_size;
    thread_handler_t entry;
    void * arguments;
    int32_t retval;
    TSC_CONTEXT ctx;
} * thread_t;

enum thread_status {
    TSC_THREAD_SLEEP    = 0xBAFF,
    TSC_THREAD_READY    = 0xFACE,
    TSC_THREAD_RUNNING  = 0xBEEF,
    TSC_THREAD_WAIT     = 0xBADD,
    TSC_THREAD_EXIT     = 0xDEAD,
};

enum thread_type {
    TSC_THREAD_IDLE     = 0x0,
    TSC_THREAD_NORMAL   = 0x1,
    TSC_THREAD_MAIN     = 0x2,
};

enum thread_deallocate {
    TSC_THREAD_UNDETACH = 0x0,
    TSC_THREAD_DETACH = 0x1,
};

extern thread_t thread_allocate (thread_handler_t entry, void * arguments, const char * name, uint32_t type, thread_attributes_t * attr);
extern void thread_deallocate (thread_t thread);
extern void thread_exit (int value);
extern void thread_yeild (void);
extern thread_t thread_self (void);
#ifdef TSC_ENABLE_THREAD_JOIN
extern status_t thread_join (thread_t thread);
extern void thread_detach (void);
#endif // TSC_ENABLE_THREAD_JOIN

// -- interfaces for thread_attributes --
extern void thread_attr_init (thread_attributes_t * attr);

static inline void thread_attr_set_stacksize (thread_attributes_t * attr, uint32_t sz)
{
    attr -> stack_size = sz;
}
static inline void thread_attr_set_timeslice (thread_attributes_t * attr, uint32_t slice)
{
    attr -> timeslice = slice;
}
static inline void thread_attr_set_affinity (thread_attributes_t * attr, uint32_t affinity)
{
    attr -> affinity = affinity;
}


#endif // _TSC_CORE_THREAD_H_
