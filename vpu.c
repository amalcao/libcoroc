
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "vpu.h"
#include "lock.h"


// the VPU manager instance.
vpu_manager_t vpu_manager;

TSC_BARRIER_DEFINE
TSC_TLS_DEFINE
TSC_SIGNAL_MASK_DEFINE

static inline thread_t core_elect (vpu_t * vpu)
{
  thread_t candidate = atomic_queue_rem (& vpu_manager . xt[vpu_manager . xt_index]);

#ifdef ENABLE_WORKSTEALING
  if (candidate == NULL) {
      int index = 0;
      for (; index < vpu_manager . xt_index; index++) {
          if (index == vpu -> id) continue;
          if (candidate = atomic_queue_rem (& vpu_manager . xt[index]))
            break;
      }
  }
#endif // ENABLE_WORKSTEALING 

  return candidate;
}

static void core_sched (void)
{
  vpu_t * vpu = TSC_TLS_GET();
  thread_t candidate = NULL;
  int idle_loops = 0;

  /* clean the watchdog tick */
  vpu -> watchdog = 0;

  /* --- the actual loop -- */
  while (true) {
      candidate = atomic_queue_rem (& vpu_manager . xt[vpu -> id]);
      if (candidate == NULL) 
        candidate = core_elect (vpu); 

      if (candidate != NULL) {
#if 0   /* the timeslice is not used in this version */
          if (candidate -> rem_timeslice == 0)
            candidate -> rem_timeslice = candidate -> init_timeslice;
#endif
          candidate -> syscall = false;
          candidate -> status = TSC_THREAD_RUNNING;
          candidate -> vpu_id = vpu -> id;

          /* restore the sigmask nested level */
          TSC_SIGNAL_STATE_LOAD(& candidate -> sigmask_nest);

          vpu -> current_thread = candidate; // important !!
#ifdef ENABLE_SPLITSTACK
          TSC_STACK_CONTEXT_LOAD(& candidate -> ctx);
#endif
          /* swap to the candidate's context */
          TSC_CONTEXT_LOAD(& candidate -> ctx);
      }
#ifdef ENABLE_DAEDLOCK_DETECT
      if (++idle_loops > 10000000) {
          pthread_mutex_lock (& vpu_manager . lock);
          vpu_manager . alive --;

          if (vpu_manager . alive == 0) {
              vpu_backtrace (vpu -> id);
          } else {
              pthread_cond_wait (& vpu_manager . cond, & vpu_manager . lock);
              // wake up by other vpu ..
              vpu_manager . alive ++;
              idle_loops = 0;
              pthread_mutex_unlock (& vpu_manager . lock);
          }
      }
#endif
  }
}

int core_exit (void * args)
{
  vpu_t * vpu = TSC_TLS_GET();
  thread_t garbage = (thread_t)args;

  if (garbage -> type == TSC_THREAD_MAIN) {
      // TODO : more works to halt the system
      exit (0);
  }
  thread_deallocate (garbage);

  return 0;
}

/* called on scheduler's context 
 * let the given thread waiting for some event */
int core_wait (void * args)
{
  vpu_t * vpu = TSC_TLS_GET();
  thread_t victim = (thread_t)args;

#if ENABLE_DAEDLOCK_DETECT
  // add this victim into a global waiting queue
  atomic_queue_add (& vpu_manager . wait_list, & victim -> wait_link);
#endif

  victim -> status = TSC_THREAD_WAIT;
  if (victim -> wait != NULL) {
      atomic_queue_add (victim -> wait, & victim -> status_link);
      victim -> wait = NULL;
  }
  if (victim -> hold != NULL) {
      unlock_hander_t unlock = victim -> unlock_handler;
      (* unlock) (victim -> hold);
      victim -> unlock_handler = NULL;
      victim -> hold = NULL;
  }
  /* victim -> vpu_id = -1; */

  return 0;
}

int core_yield (void * args)
{
  vpu_t * vpu = TSC_TLS_GET();
  thread_t victim = (thread_t)args;

  victim -> status = TSC_THREAD_READY;
  atomic_queue_add (& vpu_manager . xt[vpu_manager . xt_index], & victim -> status_link);
  return 0;
}

static void * per_vpu_initalize (void * vpu_id)
{
  thread_t scheduler;
  thread_attributes_t attr;

  vpu_t * vpu = & vpu_manager . vpu[((int)vpu_id)];
  vpu -> id = (int)vpu_id;
  vpu -> ticks = 0;
  vpu -> watchdog = 0;

  TSC_TLS_SET(vpu);

  // initialize the system scheduler thread ..
  thread_attr_init (& attr);
  thread_attr_set_stacksize (& attr, 0); // trick, using current stack !
  scheduler = thread_allocate (NULL, NULL, "sys/scheduler", TSC_THREAD_IDLE, & attr);

  vpu -> current_thread = vpu -> scheduler = scheduler;
  vpu -> initialized = true;

  TSC_SIGNAL_MASK();
  TSC_BARRIER_WAIT();

#ifdef ENABLE_SPLITSTACK
  TSC_STACK_CONTEXT_SAVE(& scheduler -> ctx);
#endif

  // trick: use current context to init the scheduler's context ..
  TSC_CONTEXT_SAVE(& scheduler -> ctx);

  // vpu_syscall will go here !!
  // because the savecontext() has no return value like setjmp,
  // we could only distinguish the status by the `entry' field !!
  if (vpu -> scheduler -> entry != NULL) {
      int (*pfn)(void *);
      pfn = scheduler -> entry;
      (* pfn) (scheduler -> arguments);
  }
#ifdef ENABLE_TIMER
  // set the sigmask of the system thread !!
  else {
      TSC_CONTEXT_SIGADDMASK(& scheduler -> ctx, TSC_CLOCK_SIGNAL);
  }
#endif // ENABLE_TIMER

  // Spawn 
  core_sched ();
}

void vpu_initialize (int vpu_mp_count)
{
  // schedulers initialization
  size_t stacksize = 1024 + PTHREAD_STACK_MIN;
  vpu_manager . xt_index = vpu_mp_count;
  vpu_manager . last_pid = 0;

  vpu_manager . vpu = (vpu_t*)TSC_ALLOC(vpu_mp_count*sizeof(vpu_t));
  vpu_manager . xt = (queue_t*)TSC_ALLOC((vpu_mp_count+1)*sizeof(vpu_t));

  // global queues initialization
  atomic_queue_init (& vpu_manager . xt[vpu_manager . xt_index]);

#ifdef ENABLE_DAEDLOCK_DETECT
  atomic_queue_init (& vpu_manager . wait_list);

  vpu_manager . alive = vpu_mp_count;

  pthread_cond_init (& vpu_manager . cond, NULL);
  pthread_mutex_init (& vpu_manager . lock, NULL);
#endif

  TSC_BARRIER_INIT(vpu_manager . xt_index + 1);
  TSC_TLS_INIT();
  TSC_SIGNAL_MASK_INIT();

  // TSC_SIGNAL_MASK();

  // VPU initialization
  int index = 0;
  for (; index < vpu_manager . xt_index; ++index) {
      TSC_OS_THREAD_ATTR attr;
      TSC_OS_THREAD_ATTR_INIT (&attr);
      TSC_OS_THREAD_ATTR_SETSTACKSZ (&attr, stacksize);

      atomic_queue_init (& vpu_manager . xt[index]);
      TSC_OS_THREAD_CREATE(
                           & (vpu_manager . vpu[index] . os_thr),
                           NULL, per_vpu_initalize, (void *)index );
  }

  TSC_BARRIER_WAIT();
}

void vpu_ready (struct thread * thread)
{
  assert (thread != NULL);

  thread -> status = TSC_THREAD_READY;
#ifdef ENABLE_DAEDLOCK_DETECT
  atomic_queue_extract (& vpu_manager . wait_list, & thread -> wait_link);
#endif
  atomic_queue_add (& vpu_manager . xt[thread -> vpu_affinity], & thread -> status_link);

  vpu_wakeup_all ();
}

void vpu_syscall (int (*pfn)(void *)) 
{
  vpu_t * vpu = TSC_TLS_GET();
  thread_t self = vpu -> current_thread;

  assert (self != NULL);

  self -> syscall = true;
  TSC_SIGNAL_STATE_SAVE(& self -> sigmask_nest);

#ifdef ENABLE_SPLITSTACK
  TSC_STACK_CONTEXT_SAVE(& self -> ctx);
#endif

  TSC_CONTEXT_SAVE(& self -> ctx);

  /* trick : use `syscall' to distinguish if already returned from syscall */
  if (self && self -> syscall) {
      thread_t scheduler = vpu -> scheduler;

#ifdef ENABLE_SPLITSTACK
      TSC_STACK_CONTEXT_LOAD(& scheduler -> ctx);
#endif 
      scheduler -> entry = pfn;
      scheduler -> arguments = self;

      vpu -> current_thread = scheduler;
      // swap to the scheduler context,
      TSC_CONTEXT_LOAD(& scheduler -> ctx);

      assert (0); 
  }

#if ENABLE_DAEDLOCK_DETECT
  /* check if grouine is returned for backtrace */
  if (self && self -> backtrace) {
      thread_backtrace (self);
  }
#endif 

  return ;
}

void vpu_suspend (queue_t * queue, void * lock, unlock_hander_t handler)
{
  thread_t self = thread_self ();
  self -> wait = queue;
  self -> hold = lock;
  self -> unlock_handler = handler;
  vpu_syscall (core_wait);
}

void vpu_clock_handler (int signal)
{
  vpu_t * vpu = TSC_TLS_GET();

  vpu -> ticks ++;    // 0.5ms per tick ..

  /* FIXME: It is unstable for OS X because of the implementation
   * 			of the ucontext API from libtask ignoring the sigmask .. */
#if defined(__APPLE__)
  if (vpu -> current_thread == vpu -> scheduler)
    return ; 
#else
  /* this case should not happen !! */
  assert (vpu -> current_thread != vpu -> scheduler);
#endif

  /* increase the watchdog tick,
   * and do re-schedule if the number reaches the threshold */
  if (++(vpu -> watchdog) > TSC_RESCHED_THRESHOLD)
    vpu_syscall (core_yield);
}

void vpu_wakeup_all (void)
{
#ifdef ENABLE_DAEDLOCK_DETECT
  pthread_mutex_lock (& vpu_manager . lock);
  if (vpu_manager . alive < vpu_manager . xt_index) {
      pthread_cond_broadcast (& vpu_manager . cond);
  }
  pthread_mutex_unlock (& vpu_manager . lock);
#endif
}

#ifdef ENABLE_DAEDLOCK_DETECT
void vpu_backtrace (int id)
{
  fprintf (stderr, "All threads are sleep, daedlock may happen!\n\n");

  thread_t wait_thr;
  while (wait_thr = atomic_queue_rem (& vpu_manager . wait_list)) {
      if (wait_thr != vpu_manager . main_thread) {
          wait_thr -> backtrace = true;
          // schedule the rescent suspended one, 
          // in order to traceback whose call stack..
          atomic_queue_add(& vpu_manager . xt[id], & wait_thr -> status_link);
      }
  }

  // schedule the main thread at last ..
  vpu_manager . main_thread -> backtrace = true;
  atomic_queue_add(& vpu_manager . xt[id], 
                   & (vpu_manager . main_thread -> status_link) );
}
#endif
