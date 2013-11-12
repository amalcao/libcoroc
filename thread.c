#include "thread.h"
#include "vpu.h"

#define TSC_DEFAULT_STACK_SIZE  (1 * 1024 * 1024) // 1MB
#define TSC_DEFAULT_AFFINITY    (vpu_manager . xt_index)
#define TSC_DEFAULT_TIMESLICE   10

TSC_TLS_DECLARE
TSC_SIGNAL_MASK_DECLARE

thread_t thread_allocate (thread_handler_t entry, void * arguments, 
 const char * name, uint32_t type, thread_attributes_t * attr)
{
    vpu_t * vpu = TSC_TLS_GET();
    thread_t thread = TSC_ALLOC(sizeof (struct thread)) ;
    void * stack_base = TSC_ALLOC(TSC_DEFAULT_STACK_SIZE);

    if (thread != NULL) {
        strcpy (thread -> name, name);
        thread -> type = type;
        thread -> id = TSC_ALLOC_TID();
        // thread -> vpuid = vpu_manager . xt_index;
        thread -> entry = entry;
        thread -> arguments = arguments;

        if (attr != NULL) {
            thread -> stack_size = attr -> stack_size;
            thread -> vpu_affinity = attr -> affinity;
            thread -> init_timeslice = attr -> timeslice;
        } else {
            thread -> stack_size = TSC_DEFAULT_STACK_SIZE;
            thread -> vpu_affinity = TSC_DEFAULT_AFFINITY;
            thread -> init_timeslice = TSC_DEFAULT_TIMESLICE;
        }

        thread -> stack_base = TSC_ALLOC(thread -> stack_size);
        thread -> rem_timeslice = thread -> init_timeslice;

        atomic_queue_init (& thread -> wait);
        queue_item_init (& thread -> status_link, thread);
        queue_item_init (& thread -> sched_link, thread);
    }

    TSC_CONTEXT_INIT (& thread -> ctx, thread -> stack_base, thread -> stack_size, thread);
    
    if (thread -> type != TSC_THREAD_IDLE) {
        // atomic_queue_add (& vpu -> thead_list, & thread -> sched_link);
        atomic_queue_add (& vpu_manager . xt[thread -> vpu_affinity], & thread -> status_link);
    }

    TSC_SIGNAL_UNMASK();
    return thread;
}

# ifdef TSC_ENABLE_THREAD_JOIN
void thread_exit (int value)
{
    TSC_SIGNAL_MASK();

    vpu_t * vpu = TSC_TLS_GET();
    thread_t target = NULL, self = vpu -> current_thread;
    thread_t p = NULL;

    self -> status = TSC_THREAD_EXIT;
    self -> retval = value;

    if (self -> wait . status) while ((p = queue_rem (& (self -> wait))) != NULL) {
        p -> status = TSC_THREAD_READY;
        atomic_queue_add (& vpu_manager . xt[vpu_manager . xt_index], & p -> status_link);
    }

    target = vpu_elect ();
    vpu_switch (target);
}

status_t thread_join (thread_t thread, int * value)
{
    TSC_SIGNAL_MASK();

    vpu_t * vpu = TSC_TLS_GET();
    thread_t self = vpu -> current_thread;

    if (thread == NULL) return TSC_ERROR;

    if (thread -> status != TSC_THREAD_EXIT) {
        queue_add (& (thread -> wait), & self -> status_link);
        vpu_suspend ();
    }

    *value = thread -> retval;

    TSC_SIGNAL_UNMASK();

    return TSC_OK;
}

# else

void thread_exit (int value)
{
    TSC_SIGNAL_MASK();

    vpu_t * vpu = TSC_TLS_GET();
    thread_t target = NULL, self = vpu -> current_thread;

    self -> status = TSC_THREAD_EXIT;
    self -> retval = value;

    target = vpu_elect ();
    vpu_switch (target);
}

# endif


void thread_yeild (void)
{
    TSC_SIGNAL_MASK();
    vpu_yield ();
    TSC_SIGNAL_UNMASK();
}
