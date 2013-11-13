
#ifndef _LIBTSC_DNA_CORE_VPU_H_
#define _LIBTSC_DNA_CORE_VPU_H_

#include <stdint.h>
#include "thread.h"
#include "queue.h"
#include "support.h"

// Type of VPU information,
//  It's a OS Thread here!!
typedef struct vpu {
    TSC_OS_THREAD_T os_thr;
    uint32_t id;
    bool initialized;
    // team_t current_team; // TODO
    thread_t current_thread;
    thread_t idle_thread;
} vpu_t;

// Type of the VPU manager
typedef struct vpu_manager {
    vpu_t *vpu;
    queue_t *xt;
    uint32_t xt_index;
    uint32_t last_pid;
    // queue_t team_list; // TODO
    queue_t thread_list;
} vpu_manager_t;

extern vpu_manager_t vpu_manager;

extern void vpu_initialize (int cpu_mp_count);
extern void vpu_switch (struct thread * thread);
extern struct thread * vpu_elect (void);
extern void vpu_resume (struct thread * thread);
extern void vpu_suspend (lock_t lock); // TODO
extern void vpu_spawn (struct thread * thread);
extern void vpu_yield (void);
extern void vpu_clock_handler (int);

#define TSC_ALLOC_TID() __sync_fetch_and_add(& vpu_manager.last_pid, 1)

#endif // _LIBTSC_DNA_CORE_VPU_H_
