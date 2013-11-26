
#include <stdint.h>
#include <string.h>
#include <assert.h>
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

	assert (msg != NULL);

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

	assert (_msg -> buff != NULL);
    memcpy (_msg -> buff, msg -> buff, msg -> size);

    return _msg;
}

static int message_send (message_t msg, thread_t to)
{
#ifdef ENABLE_QUICK_RESPONSE
    bool spawn_to = false;
#endif
    msg -> send_tid = thread_self ();
    msg -> recv_tid = to;

    TSC_SIGNAL_MASK();
    
    message_t _msg = message_clone (msg);
    atomic_queue_add (& to -> message_queue, & _msg -> message_link);

    lock_acquire (to -> wait . lock);
    if (to -> status == TSC_THREAD_WAIT &&
        queue_lookup (& to -> wait, general_inspector, to) ) {
        
        queue_extract (& to -> wait, & to -> status_link);
#ifdef ENABLE_QUICK_RESPONSE 
        spawn_to = true;
#else
        vpu_ready (to);
#endif
    }
    lock_release (to -> wait . lock);

#ifdef ENABLE_QUICK_RESPONSE
    if (spawn_to) vpu_switch (to, NULL);
#endif

    TSC_SIGNAL_UNMASK ();
    return 0;
}

static int message_recv (message_t * msg, thread_t from, bool block)
{
    thread_t self = thread_self ();
    * msg = NULL;
 
    TSC_SIGNAL_MASK ();

    for (;;) {
		lock_acquire (self -> message_queue . lock);
		if (* msg = queue_lookup(& self -> message_queue, message_inspector, from)) {
			queue_extract (& self -> message_queue,  & ((*msg) -> message_link));
		} 
		lock_release (self -> message_queue . lock);

		if (*msg != NULL || !block) break;

		vpu_suspend (& self -> wait, NULL);
	}

	TSC_SIGNAL_UNMASK ();
	return (*msg != NULL) ? 0 : -1;
}

static void message_deallocate (message_t msg)
{
	if (msg -> type == TSC_MSG_HARD) TSC_DEALLOC (msg -> buff);
	TSC_DEALLOC (msg);
}

//exported APIs
int send (thread_t to, size_t size, void * buff)
{
	message_t msg = message_allocate (size, buff);
	if (message_send (msg, to) != 0) 
		return -1;

	message_deallocate (msg);
	return (int)size;
}

int recv (thread_t from, size_t size, void * buff, bool block)
{
	message_t msg = NULL;

	if (message_recv (& msg, from, block) != 0)
		return -1;

	if (size > msg -> size) 
		size = msg -> size;

	memcpy (buff, msg -> buff, size);
	message_deallocate (msg);

	return (int)size;
}
