#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "support.h"
#include "channel.h"

TSC_SIGNAL_MASK_DECLARE

static inline void quantum_init (quantum * q, thread_t thread, void * buf)
{
	q -> thread = thread;
	q -> itembuf = buf;
	queue_item_init (& q -> link, q);
}

channel_t channel_allocate (int32_t elemsize, int32_t bufsize)
{
	struct channel * chan = TSC_ALLOC (sizeof(struct channel) + elemsize * bufsize);
	assert (chan != NULL);

	chan -> elemsize = elemsize;
	chan -> bufsize = bufsize;	
	chan -> buf = (uint8_t *)(chan + 1);
	chan -> nbuf = 0;
	chan -> recvx = chan -> sendx = 0; // TODO
	chan -> lock = lock_allocate ();

	assert (chan -> lock != NULL);

	queue_init (& chan -> recv_que);
	queue_init (& chan -> send_que);

	return chan;
}

void channel_dealloc (channel_t chan)
{
	/* TODO: awaken the sleeping threads */
	lock_deallocate (chan -> lock);
	TSC_DEALLOC (chan);
}

static int _channel_send (channel_t chan, void * buf, bool block)
{
	assert (chan != NULL);
	TSC_SIGNAL_MASK ();

	int ret = 0;
	thread_t self = thread_self();

	lock_acquire (chan -> lock);
	// check if there're any waiting threads ..
	quantum * qp = queue_rem (& chan -> recv_que);
	if (qp != NULL) {
		memcpy (qp -> itembuf, buf, chan -> elemsize);
		vpu_ready (qp -> thread);
		
		lock_release (chan -> lock);
		return 0;
	}
	
	// check if there're any buffer slots ..
	if (chan -> nbuf < chan -> bufsize) {
		uint8_t * p = chan -> buf;
		p += (chan -> elemsize) * (chan -> sendx ++);
		memcpy (p, buf, chan -> elemsize);
		(chan -> sendx) %= (chan -> bufsize);
		chan -> nbuf ++;
	} else {
		if (block) {
			// the async way ..
			quantum q;
			quantum_init (& q, self, buf);
			queue_add (& chan -> send_que, & q . link);
			vpu_suspend (NULL, chan -> lock);
		} else {
			ret = -1;
		}
	}
	// awaken by some receiver ..
	lock_release (chan -> lock);

	TSC_SIGNAL_UNMASK ();

	return ret;
}

static int _channel_recv (channel_t chan, void * buf, bool block)
{
	assert (chan != NULL);
	TSC_SIGNAL_MASK ();

	int ret = 0;
	thread_t self = thread_self ();

	lock_acquire (chan -> lock);
	// check if there're any senders pending .
	quantum * qp = queue_rem (& chan -> send_que);
	if (qp != NULL) {
		memcpy (buf, qp -> itembuf, chan -> elemsize);
		vpu_ready (qp -> thread);

		lock_release (chan -> lock);
		return 0;
	}

	// check if there're any empty slots ..
	if (chan -> nbuf > 0) {
		uint8_t * p = chan -> buf;
		p += (chan -> elemsize) * (chan -> recvx ++);
		memcpy (buf, p, chan -> elemsize);
		(chan -> recvx) %= (chan -> bufsize);
		chan -> nbuf --;
	} else {
		if (block) {
			// async way ..
			quantum q;
			quantum_init (& q, self, buf);
			queue_add (& chan -> recv_que, & q . link);
			vpu_suspend (NULL, chan -> lock);

		} else {
			ret = -1;
		}
	}

	lock_release (chan -> lock);

	TSC_SIGNAL_UNMASK ();
	return ret;
}

/* -- the public APIs for channel -- */
int channel_send (channel_t chan, void * buf)
{
	return _channel_send (chan, buf, true);
}

int channel_recv (channel_t chan, void * buf)
{
	return _channel_recv (chan, buf, true);
}

int channel_nbsend (channel_t chan, void * buf)
{
	return _channel_send (chan, buf, false);
}

int channel_nbrecv (channel_t chan, void * buf)
{
	return _channel_recv (chan, buf, false);
}

