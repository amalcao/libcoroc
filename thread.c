#include <string.h>
#include <assert.h>
#include "thread.h"
#include "vpu.h"

#ifdef __APPLE__
# define TSC_DEFAULT_STACK_SIZE (2 * 1024 * 1024) // 4MB
#else
# define TSC_DEFAULT_STACK_SIZE  (1 * 1024 * 1024) // 1MB
#endif

#define TSC_DEFAULT_AFFINITY    (vpu_manager . xt_index)
#define TSC_DEFAULT_TIMESLICE   5
#define TSC_DEFAULT_DETACHSTATE TSC_THREAD_UNDETACH

TSC_TLS_DECLARE
TSC_SIGNAL_MASK_DECLARE

void thread_attr_init (thread_attributes_t * attr)
{
    attr -> stack_size = TSC_DEFAULT_STACK_SIZE;
    attr -> timeslice = TSC_DEFAULT_TIMESLICE;
    attr -> affinity = TSC_DEFAULT_AFFINITY;
}

thread_t thread_allocate (thread_handler_t entry, void * arguments, 
 const char * name, uint32_t type, thread_attributes_t * attr)
{
    vpu_t * vpu = TSC_TLS_GET();
    thread_t thread = TSC_ALLOC(sizeof (struct thread)) ;
	memset (thread, 0, sizeof(struct thread));
	memset (& thread -> ctx, 0, sizeof (struct ucontext));

    if (thread != NULL) {
        strcpy (thread -> name, name);
        thread -> type = type;
        thread -> id = TSC_ALLOC_TID();
        thread -> entry = entry;
        thread -> arguments = arguments;
		thread -> syscall = false;
		thread -> wait = NULL;

        if (attr != NULL) {
            thread -> stack_size = attr -> stack_size;
            thread -> vpu_affinity = attr -> affinity;
            thread -> init_timeslice = attr -> timeslice;
        } else {
            thread -> stack_size = TSC_DEFAULT_STACK_SIZE;
            thread -> vpu_affinity = TSC_DEFAULT_AFFINITY;
            thread -> init_timeslice = TSC_DEFAULT_TIMESLICE;
        }

		if (thread -> stack_size > 0) {
			thread -> stack_base = TSC_ALLOC(thread -> stack_size);
			assert (thread -> stack_base != NULL);
		}

		thread -> rem_timeslice = thread -> init_timeslice;

		queue_item_init (& thread -> status_link, thread);
	}

	if (thread -> type != TSC_THREAD_IDLE) { 
		TSC_CONTEXT_INIT (& thread -> ctx, thread -> stack_base, thread -> stack_size, thread);
		atomic_queue_add (& vpu_manager . xt[thread -> vpu_affinity], & thread -> status_link);
		TSC_SIGNAL_UNMASK();
	}

	return thread;
}

void thread_deallocate (thread_t thread)
{
	// TODO : reclaim the thread elements ..
	if (thread -> stack_size > 0)
		TSC_DEALLOC (thread -> stack_base);
	TSC_DEALLOC (thread);
}

void thread_exit (int value)
{
	TSC_SIGNAL_MASK();
	vpu_syscall (core_exit);
	TSC_SIGNAL_UNMASK();
}

void thread_yield (void)
{
	TSC_SIGNAL_MASK();
	vpu_syscall (core_yield);
	TSC_SIGNAL_UNMASK();
}

thread_t thread_self (void)
{   
	thread_t self = NULL;
	TSC_SIGNAL_MASK ();
	vpu_t * vpu = TSC_TLS_GET();
	self = vpu -> current_thread;
	TSC_SIGNAL_UNMASK ();

	return self;
}

