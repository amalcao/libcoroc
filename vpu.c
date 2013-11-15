
#include <assert.h>
#include "vpu.h"
#include "lock.h"


// the VPU manager instance.
vpu_manager_t vpu_manager;

TSC_BARRIER_DEFINE
TSC_TLS_DEFINE
TSC_SIGNAL_MASK_DEFINE

static int core_idle (void * args)
{
    vpu_t * vpu = TSC_TLS_GET();
    thread_t thread = NULL;

    vpu -> initialized = true;
    TSC_BARRIER_WAIT();

    /* --- the actual loop -- */
    while (true) if ((thread = vpu_elect ()) != NULL) {
        vpu_switch (thread);
    }
}

static void * per_vpu_initalize (void * vpu_id)
{
    thread_t thread;
    vpu_t * vpu = & vpu_manager . vpu[((int)vpu_id)];

    TSC_TLS_SET(vpu);
    
    vpu -> id = (int)vpu_id;
    vpu -> initialized = true;

    // initialize the idle thread
    thread = thread_allocate (core_idle, NULL, "idle_loop", TSC_THREAD_IDLE, 0);
    vpu -> idle_thread = thread;

    // Spawn 
    vpu_spawn (thread); // never return ..
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
    // atomic_queue_init (& vpu_manager . team_list);
    atomic_queue_init (& vpu_manager . thread_list);
   
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


void vpu_switch (struct thread * thread)
{
    vpu_t * vpu = TSC_TLS_GET();
    thread_t self = vpu -> current_thread;

    if (thread == NULL) thread = vpu -> idle_thread;

    if (thread -> rem_timeslice == 0) 
        thread -> rem_timeslice = thread -> init_timeslice;
    thread -> status = TSC_THREAD_RUNNING;
    thread -> vpu_id = vpu -> id;
    vpu -> current_thread = thread;
    
	if (self -> status == TSC_THREAD_RUNNING &&
		self -> type != TSC_THREAD_IDLE) {
		self -> status = TSC_THREAD_READY;
		atomic_queue_add (& vpu_manager . xt[self -> vpu_id], & self -> status_link);
	}

	TSC_CONTEXT_SWAP((& self -> ctx), (& thread -> ctx));
}

struct thread * vpu_elect (void)
{
	vpu_t * vpu = TSC_TLS_GET();
	thread_t target = NULL;

	target = atomic_queue_rem (& vpu_manager . xt[vpu -> id]);
	if (target != NULL) return target;

	target = atomic_queue_rem (& vpu_manager . xt[vpu_manager . xt_index]);

#ifdef TSC_ENABLE_WORKSTEALING
	if (target != NULL) return target;

	int index = 0;
	for (; index < vpu_manager . xt_index; ++index) {
		if (index == vpu -> id) continue;
		target = atomic_queue_rem (& vpu_manager . xt[index]);
		if (target != NULL)
			break;
	}
#endif // TSC_ENABLE_WORKSTEALING

	return target;
}

void vpu_spawn (thread_t thread)
{
	vpu_t *vpu = TSC_TLS_GET();

	thread -> status = TSC_THREAD_RUNNING;
    thread -> vpu_id = vpu -> id;
	vpu -> current_thread = thread;

	TSC_CONTEXT_LOAD(& thread -> ctx);
}

void vpu_yield (void)
{
	vpu_t *vpu = TSC_TLS_GET();
	thread_t target = NULL;
	thread_t self = vpu -> current_thread;

	target = vpu_elect ();
	vpu_switch (target);
}

void vpu_suspend (lock_t lock)
{
	vpu_t *vpu = TSC_TLS_GET();
	thread_t target = NULL;
	thread_t self = vpu -> current_thread;

	self -> status = TSC_THREAD_WAIT;
	atomic_queue_add (& self -> wait, & self -> status_link); // FIXME !!

	target = vpu_elect ();
	if (lock != NULL) lock_release (lock);

	vpu_switch (target);
	if (lock != NULL) lock_acquire (lock);
}

void vpu_ready (struct thread * thread)
{
    assert (thread != NULL);

    thread -> status = TSC_THREAD_READY;
    atomic_queue_add (& vpu_manager . xt[thread -> vpu_affinity], & thread -> status_link);
}

void vpu_clock_handler (int signal)
{
	vpu_t * vpu = TSC_TLS_GET();
	assert (vpu != NULL);

	thread_t current = vpu -> current_thread;
	assert (current != NULL);

    if (current == vpu -> idle_thread) return;

	if ((-- (current -> rem_timeslice)) == 0) {
		thread_t candidate = vpu_elect ();
		if (candidate == NULL) {
			current -> rem_timeslice = current -> init_timeslice;
			return;
		} 

		vpu_switch (candidate);
	}
}
