
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

			vpu -> current_thread = candidate; // important !!
			/* swap to the candidate's context */
			TSC_CONTEXT_LOAD(& candidate -> ctx);
		}
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
        sigaddset (& scheduler -> ctx . uc_sigmask, TSC_CLOCK_SIGNAL);
    }
#endif // ENABLE_TIMER

	// Spawn 
	core_sched ();
}

void vpu_initialize (int vpu_mp_count)
{
	// schedulers initialization
	vpu_manager . xt_index = vpu_mp_count;
	vpu_manager . last_pid = 0;

	vpu_manager . vpu = (vpu_t*)TSC_ALLOC(vpu_mp_count*sizeof(vpu_t));
	vpu_manager . xt = (queue_t*)TSC_ALLOC((vpu_mp_count+1)*sizeof(vpu_t));

	// global queues initialization
	atomic_queue_init (& vpu_manager . xt[vpu_manager . xt_index]);

	TSC_BARRIER_INIT(vpu_manager . xt_index + 1);
	TSC_TLS_INIT();
	TSC_SIGNAL_MASK_INIT();

	TSC_SIGNAL_MASK();

	// VPU initialization
	int index = 0;
	for (; index < vpu_manager . xt_index; ++index) {
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
	atomic_queue_add (& vpu_manager . xt[thread -> vpu_affinity], & thread -> status_link);
}

void vpu_syscall (int (*pfn)(void *)) 
{
	vpu_t * vpu = TSC_TLS_GET();
	thread_t self = vpu -> current_thread;

	assert (self != NULL);

	self -> syscall = true;
	TSC_CONTEXT_SAVE(& self -> ctx);

	/* trick : use `syscall' to distinguish if already returned from syscall */
	if (self && self -> syscall) {
		thread_t scheduler = vpu -> scheduler;
		scheduler -> entry = pfn;
		scheduler -> arguments = self;

		vpu -> current_thread = scheduler;
		// swap to the scheduler context,
		TSC_CONTEXT_LOAD(& scheduler -> ctx);

		assert (0); 
	}

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

    /* this case should not happen !! */
    assert (vpu -> current_thread != vpu -> scheduler);

    /* increase the watchdog tick,
     * and do re-schedule if the number reaches the threshold */
    if (++(vpu -> watchdog) > TSC_RESCHED_THRESHOLD)
        vpu_syscall (core_yield);
}
