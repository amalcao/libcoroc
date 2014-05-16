
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "vpu.h"
#include "vfs.h"
#include "netpoll.h"
#include "lock.h"

#define MAX_SPIN_LOOP_NUM 1
#define WAKEUP_THRESHOLD (vpu_manager.alive << 1)

#define MAX_STEALING_FAIL_NUM (20 * (vpu_manager.xt_index))

// the VPU manager instance.
vpu_manager_t vpu_manager;

TSC_BARRIER_DEFINE
TSC_TLS_DEFINE
TSC_SIGNAL_MASK_DEFINE

// added by ZHJ {{
/*
 * Used the pseudo-random generator defined by the congruence
 *  S' = 69070 * S % (2^32 - 5).
 * Marsaglia, George.  "Remarks on choosing and implementingrandom number
 * generators",
 * Communications of the ACM v 36n 7 (July 1993), p 105-107.
 * http://www.firstpr.com.au/dsp/rand31/p105-crawford.pdf
 */
static const unsigned RNGMOD = ((1ULL << 32) - 5);
static const unsigned RNGMUL = 69070U;

static inline unsigned __myrand(vpu_t* vpu) {
  unsigned state = vpu->rand_seed;
  state = (unsigned)((RNGMUL * (unsigned long long)state) % RNGMOD);
  vpu->rand_seed = state;
  return state;
}

/* initialize per-vpu rand_seed */
static inline void __mysrand(vpu_t* vpu, unsigned seed) {
  seed %= RNGMOD;
  seed +=
      (seed ==
       0); /* 0 does not belong to the multiplicative group.  Use 1 instead */
  vpu->rand_seed = seed;
}

static inline void* __random_steal(vpu_t* vpu) {
  // randomly select a victim to steal
  int victim_id = __myrand(vpu) % (vpu_manager.xt_index);

#if 0
  // ignore it if the victim is the current vpu ..
  if (victim_id == vpu -> id) 
    return NULL;
#endif
  // try to steal a work ..
  return atomic_queue_rem(&vpu_manager.xt[victim_id]);
}
// }}

static inline tsc_coroutine_t core_elect(vpu_t* vpu) {
#if 0
  if (TSC_ATOMIC_READ(vpu_manager . total_ready) == 0)
    return NULL;
#endif

  tsc_coroutine_t candidate =
      atomic_queue_rem(&vpu_manager.xt[vpu_manager.xt_index]);

#ifdef ENABLE_WORKSTEALING
  // update by ZHJ {{
  if (candidate == NULL) {
    int failure_time = 0;
    while ((candidate = __random_steal(vpu)) == NULL) {
      if (failure_time++ > MAX_STEALING_FAIL_NUM) break;
    }
  }
// }}
#endif  // ENABLE_WORKSTEALING

  return candidate;
}

// try to find and schedule a runnable coroutine.
// search order is :
// 1) the private running queue of current VPU
// 2) the global running queue
// 3) stealing other VPUs' running queues
// 4) try to polling the async net IO
//
// TODO : since the busy loop will make CPUs busy
// and may cause the performance degradation,
// so if no corouine is found for a relative "long" time,
// the VPU thread should go to sleep and wait for other
// to wakeup it.
//
static void core_sched(void) {
  vpu_t* vpu = TSC_TLS_GET();
  tsc_coroutine_t candidate = NULL;
  int idle_loops = 0;

  // atomic inc the all idle thread number
  TSC_ATOMIC_INC(vpu_manager.idle);

  // clean the watchdog tick
  vpu->watchdog = 0;

  /* --- the actual loop -- */
  while (true) {
    candidate = atomic_queue_try_rem(&vpu_manager.xt[vpu->id]);

    if (candidate == NULL) {
      // polling the async net IO ..
      __tsc_netpoll_polling(false);

      // try to fetch tasks from the global queue,
      // or stealing from other VPUs' queue ..
      candidate = core_elect(vpu);
#if ENABLE_VFS
      if (candidate == NULL) candidate = tsc_vfs_get_coroutine();
#endif  // ENABLE_VFS
    }

    if (candidate != NULL) {
#if 0 /* the timeslice is not used in this version */
          if (candidate -> rem_timeslice == 0)
            candidate -> rem_timeslice = candidate -> init_timeslice;
#endif
      // atomic dec the total idle number
      TSC_ATOMIC_DEC(vpu_manager.idle);

      // atomic dec the total ready jobs' number
      TSC_ATOMIC_DEC(vpu_manager.total_ready);

      candidate->syscall = false;
      candidate->status = TSC_COROUTINE_RUNNING;
      candidate->vpu_id = vpu->id;
      // FIXME : how to decide the affinity ?
      candidate->vpu_affinity = vpu->id;

      /* restore the sigmask nested level */
      TSC_SIGNAL_STATE_LOAD(&candidate->sigmask_nest);

      vpu->current = candidate;  // important !!
#ifdef ENABLE_SPLITSTACK
      TSC_STACK_CONTEXT_LOAD(&candidate->ctx);
#endif
      /* swap to the candidate's context */
      TSC_CONTEXT_LOAD(&candidate->ctx);
    }
    if (++idle_loops > MAX_SPIN_LOOP_NUM) {
      pthread_mutex_lock(&vpu_manager.lock);
      vpu_manager.alive--;

      if (vpu_manager.alive == 0 && vpu_manager.total_ready == 0 &&
          !tsc_vfs_working()) {
#ifdef ENABLE_DEADLOCK_DETECT
        pthread_mutex_unlock(&vpu_manager.lock);
        /* wait until one net job coming ..*/
        if (__tsc_netpoll_size() > 0) {
            __tsc_netpoll_polling(true);
            pthread_mutex_lock(&vpu_manager.lock);
        }
        /* no any ready coroutines, just halt .. */
        else
          vpu_backtrace(vpu->id);
#endif
      } else if (vpu_manager.alive > 0) {
        TSC_ATOMIC_DEC(vpu_manager.idle);
        pthread_cond_wait(&vpu_manager.cond, &vpu_manager.lock);
        // wake up by other vpu ..
        TSC_ATOMIC_INC(vpu_manager.idle);
      }

      idle_loops = 0;
      vpu_manager.alive++;
      pthread_mutex_unlock(&vpu_manager.lock);
    }
  }
}

// destroy a given coroutine and its stack context.
// since the stack may alwayvpu_managers be used during the lifetime
// of the coroutine, so this destruction must be a syscall
// which called by system (idle) corouine on the system context.
int core_exit(void* args) {
  vpu_t* vpu = TSC_TLS_GET();
  tsc_coroutine_t garbage = (tsc_coroutine_t)args;

  if (garbage->type == TSC_COROUTINE_MAIN) {
    // TODO : more works to halt the system
    exit(0);
  }
  tsc_coroutine_deallocate(garbage);

  return 0;
}

// called on scheduler's context
// let the given coroutine waiting for some event
int core_wait(void* args) {
  vpu_t* vpu = TSC_TLS_GET();
  tsc_coroutine_t victim = (tsc_coroutine_t)args;

  victim->status = TSC_COROUTINE_WAIT;

  if (vpu->hold != NULL) {
    TSC_DEBUG("[core_wait unlock %p] vid is %d, coid is %ld\n", vpu->hold,
              vpu->id, (uint64_t)(victim->id));
    (vpu->unlock_handler)(vpu->hold);
    vpu->hold = NULL;
    vpu->unlock_handler = NULL;
  }

  return 0;
}

// stop current coroutine and try to schedule others.
// NOTE : must link current one to the global running queue!
int core_yield(void* args) {
  vpu_t* vpu = TSC_TLS_GET();
  tsc_coroutine_t victim = (tsc_coroutine_t)args;

  victim->status = TSC_COROUTINE_READY;
  atomic_queue_add(&vpu_manager.xt[vpu_manager.xt_index], &victim->status_link);
  TSC_ATOMIC_INC(vpu_manager.total_ready);

  return 0;
}

// init every vpu thread, and make current stack context
// as the system (idle) corouine's stack context.
// all system calls must run on those contexts
// via the `vpu_syscall()' .
static void* per_vpu_initalize(void* vpu_id) {
  tsc_coroutine_t scheduler;
  tsc_coroutine_attributes_t attr;

  vpu_t* vpu = &vpu_manager.vpu[((tsc_word_t)vpu_id)];
  vpu->id = (int)((tsc_word_t)vpu_id);
  vpu->ticks = 0;
  vpu->watchdog = 0;

  // add by zhj, init the rand seed.
  __mysrand(vpu, vpu->id + 1);

  TSC_TLS_SET(vpu);

  // initialize the system scheduler coroutine ..
  tsc_coroutine_attr_init(&attr);
  tsc_coroutine_attr_set_stacksize(&attr, 0);  // trick, using current stack !
  scheduler = tsc_coroutine_allocate(NULL, NULL, "sys/scheduler",
                                     TSC_COROUTINE_IDLE, &attr);

  vpu->current = vpu->scheduler = scheduler;
  vpu->initialized = true;

  TSC_SIGNAL_MASK();
  TSC_BARRIER_WAIT();

#ifdef ENABLE_SPLITSTACK
  TSC_STACK_CONTEXT_SAVE(&scheduler->ctx);
#endif

  // trick: use current context to init the scheduler's context ..
  TSC_CONTEXT_SAVE(&scheduler->ctx);

  // vpu_syscall will go here !!
  // because the savecontext() has no return value like setjmp,
  // we could only distinguish the status by the `entry' field !!
  if (vpu->scheduler->entry != NULL) {
    int (*pfn)(void*);
    pfn = scheduler->entry;
    (*pfn)(scheduler->arguments);
  }
#ifdef ENABLE_TIMER
  // set the sigmask of the system coroutine !!
  else {
    TSC_CONTEXT_SIGADDMASK(&scheduler->ctx, TSC_CLOCK_SIGNAL);
  }
#endif  // ENABLE_TIMER

  // Spawn
  core_sched();

  return NULL;  // NEVER REACH HERE!!
}

// init the vpu sub-system with the hint of
// how many OS threads will be used.
void tsc_vpu_initialize(int vpu_mp_count, tsc_coroutine_handler_t entry) {
  // schedulers initialization
  size_t stacksize = 1024 + PTHREAD_STACK_MIN;
  vpu_manager.xt_index = vpu_mp_count;
  vpu_manager.last_pid = 0;

  vpu_manager.vpu = (vpu_t*)TSC_ALLOC(vpu_mp_count * sizeof(vpu_t));
  vpu_manager.xt = (queue_t*)TSC_ALLOC((vpu_mp_count + 1) * sizeof(vpu_t));

  // global queues initialization
  atomic_queue_init(&vpu_manager.xt[vpu_manager.xt_index]);
  atomic_queue_init(&vpu_manager.coroutine_list);

#ifdef ENABLE_DEADLOCK_DETECT

  vpu_manager.alive = vpu_mp_count;
  vpu_manager.idle = 0;
  vpu_manager.total_ready = 0;

  pthread_cond_init(&vpu_manager.cond, NULL);
  pthread_mutex_init(&vpu_manager.lock, NULL);
#endif

  TSC_BARRIER_INIT(vpu_manager.xt_index + 1);
  TSC_TLS_INIT();
  TSC_SIGNAL_MASK_INIT();

  // create the first coroutine, "init" ..
  tsc_coroutine_t init =
      tsc_coroutine_allocate(entry, NULL, "init", TSC_COROUTINE_MAIN, 0);

  // VPU initialization
  tsc_word_t index = 0;
  for (; index < vpu_manager.xt_index; ++index) {
    TSC_OS_THREAD_ATTR attr;
    TSC_OS_THREAD_ATTR_INIT(&attr);
    TSC_OS_THREAD_ATTR_SETSTACKSZ(&attr, stacksize);

    atomic_queue_init(&vpu_manager.xt[index]);
    TSC_OS_THREAD_CREATE(&(vpu_manager.vpu[index].os_thr), NULL,
                         per_vpu_initalize, (void*)index);
  }

  TSC_BARRIER_WAIT();
}

// make the given coroutine runnable,
// change its state and link it to the running queue.
void vpu_ready(tsc_coroutine_t coroutine) {
  assert(coroutine != NULL);

  coroutine->status = TSC_COROUTINE_READY;
  atomic_queue_add(&vpu_manager.xt[coroutine->vpu_affinity],
                   &coroutine->status_link);
  TSC_ATOMIC_INC(vpu_manager.total_ready);

  vpu_wakeup_one();
}

// call the core functions on a system (idle) coroutine's stack context,
// in order to prevent the conpetition among the VPUs.
void vpu_syscall(int (*pfn)(void*)) {
  vpu_t* vpu = TSC_TLS_GET();
  tsc_coroutine_t self = vpu->current;

  assert(self != NULL);

  self->syscall = true;
  TSC_SIGNAL_STATE_SAVE(&self->sigmask_nest);

#ifdef ENABLE_SPLITSTACK
  TSC_STACK_CONTEXT_SAVE(&self->ctx);
#endif

  TSC_CONTEXT_SAVE(&self->ctx);

  /* trick : use `syscall' to distinguish if already returned from syscall */
  if (self && self->syscall) {
    tsc_coroutine_t scheduler = vpu->scheduler;

#ifdef ENABLE_SPLITSTACK
    TSC_STACK_CONTEXT_LOAD(&scheduler->ctx);
#endif
    scheduler->entry = pfn;
    scheduler->arguments = self;

    vpu->current = scheduler;
    // swap to the scheduler context,
    TSC_CONTEXT_LOAD(&scheduler->ctx);

    assert(0);
  }

#if ENABLE_DEADLOCK_DETECT
  /* check if grouine is returned for backtrace */
  if (self && self->backtrace) {
    tsc_coroutine_backtrace(self);
  }
#endif

  return;
}

// suspend current coroutine on a given wait queue,
// the `lock' must be hold until the context be saved completely,
// in order to prevent other VPU to load the context in an ill state.
// the `handler' tells the VPU how to release the `lock'.
void vpu_suspend(volatile void* lock, unlock_handler_t handler) {
  vpu_t* vpu = TSC_TLS_GET();
  vpu->hold = (void*)lock;
  vpu->unlock_handler = handler;

  vpu_syscall(core_wait);
}

void vpu_clock_handler(int signal) {
  vpu_t* vpu = TSC_TLS_GET();

  vpu->ticks++;  // 0.5ms per tick ..

/* FIXME: It is unstable for OS X because of the implementation
 * 			of the ucontext API from libtask ignoring the sigmask ..
 */
#if defined(__APPLE__)
  if (vpu->current == vpu->scheduler) return;
#else
  /* this case should not happen !! */
  assert(vpu->current != vpu->scheduler);
#endif

  /* increase the watchdog tick,
   * and do re-schedule if the number reaches the threshold */
  if (++(vpu->watchdog) > TSC_RESCHED_THRESHOLD) vpu_syscall(core_yield);
}

void vpu_wakeup_one(void) {
  // wakeup a VPU thread who waiting the pthread_cond_t.
  pthread_mutex_lock(&vpu_manager.lock);
  if (vpu_manager.alive < vpu_manager.xt_index &&
      TSC_ATOMIC_READ(vpu_manager.idle) == 0 &&
      TSC_ATOMIC_READ(vpu_manager.total_ready) > WAKEUP_THRESHOLD) {
    pthread_cond_signal(&vpu_manager.cond);
  }
  pthread_mutex_unlock(&vpu_manager.lock);
}

#ifdef ENABLE_DEADLOCK_DETECT
void vpu_backtrace(int id) {
  fprintf(stderr, "All threads are sleep, daedlock may happen!\n\n");

  // the libc's `backtrace()' can only trace the frame of the caller,
  // so we must schedule the suspended coroutine to a running OS coroutine
  // and then calling the `backtrace()' ..
  tsc_coroutine_t wait_thr;
  while ((wait_thr = atomic_queue_rem(&vpu_manager.coroutine_list)) != NULL) {
    if (wait_thr != vpu_manager.main) {
      wait_thr->backtrace = true;
      // schedule the rescent suspended one,
      // in order to traceback whose call stack..
      atomic_queue_add(&vpu_manager.xt[id], &wait_thr->status_link);
    }
  }

  // schedule the main coroutine at last,
  // because the main coroutine exits will kill the program.
  vpu_manager.main->backtrace = true;
  atomic_queue_add(&vpu_manager.xt[id], &(vpu_manager.main->status_link));
}
#endif
