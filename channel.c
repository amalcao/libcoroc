
#include <stdint.h>
#include <string.h>
#include "channel.h"
#include "vpu.h"

TSC_SIGNAL_MASK_DECLARE

static bool message_inspector (void * p0, void * p1)
{
    message_t msg = (message_t)p0;
    thread_t thread = (thread_t)p1;

	if (thread == NULL) return (msg != NULL); // NULL sender match any .. 
    return (msg -> send_tid == thread);
}

static message_t message_allocate (size_t size, void * buff)
{
    message_t msg = TSC_ALLOC (sizeof (struct message));
    msg -> type = TSC_MSG_SOFT;
    msg -> size = size;
    msg -> buff = buff;

    msg -> send_tid = NULL;
    msg -> recv_tid = NULL;

    queue_item_init (& msg -> message_link, msg);

    return msg;
}

static message_t message_clone (message_t msg)
{
    message_t _msg = message_allocate (msg -> size, NULL);

    _msg -> send_tid = msg -> send_tid;
    _msg -> recv_tid = msg -> recv_tid;
    _msg -> type = TSC_MSG_HARD;
    _msg -> buff = TSC_ALLOC (msg -> size);
    memcpy (_msg -> buff, msg -> buff, msg -> size);

    return _msg;
}

static status_t message_send (message_t msg, thread_t to)
{
    msg -> send_tid = thread_self ();
    msg -> recv_tid = to;

    TSC_SIGNAL_MASK();
    
    message_t _msg = message_clone (msg);
    atomic_queue_add (& to -> message_queue, & _msg -> message_link);

    lock_acquire (to -> wait . lock);
    if (to -> status == TSC_THREAD_WAIT &&
        queue_lookup (& to -> wait, general_inspector, to) ) {
        
        queue_extract (& to -> wait, & to -> status_link);
#if defined(ENABLE_QUICK_RESPONSE)  
        // FIXME : enable this option will cause segmentation fault, why ?
        vpu_switch (to, to -> wait . lock);
#else
        vpu_ready (to);
#endif
    }
    lock_release (to -> wait . lock);

    TSC_SIGNAL_UNMASK ();
    return 0;
}

static status_t message_recv (message_t * msg, thread_t from, bool block)
{
    thread_t self = thread_self ();
    * msg = NULL;
 
    TSC_SIGNAL_MASK ();

    lock_acquire (self -> message_queue . lock);

    for (;;) {
        if (* msg = queue_lookup(& self -> message_queue, message_inspector, from)) {
            queue_extract (& self -> message_queue,  & ((*msg) -> message_link));
            break;
        } 
        
        if (!block) break;

        vpu_suspend (& self -> wait, self -> message_queue . lock);
    }

    lock_release (self -> message_queue . lock);
    TSC_SIGNAL_UNMASK ();
    return (*msg != NULL) ? 0 : -1;
}

static void message_deallocate (message_t msg)
{
    if (msg -> type == TSC_MSG_HARD) TSC_DEALLOC (msg -> buff);
    TSC_DEALLOC (msg);
}

//exported APIs
status_t send (thread_t to, size_t size, void * buff)
{
    message_t msg = message_allocate (size, buff);
    status_t ret = message_send (msg, to);
    message_deallocate (msg);
    return ret;
}

status_t recv (thread_t from, size_t * size, void * buff, bool block)
{
    message_t msg = NULL;
    status_t ret = message_recv (& msg, from, block);

    if (ret == 0) {
        if (* size > msg -> size) * size = msg -> size;
        memcpy (buff, msg -> buff, * size);
    }

    if (msg != NULL) message_deallocate (msg);

    return ret;
}
