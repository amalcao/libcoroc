#ifndef _TSC_CORE_CHANNEL_H_
#define _TSC_CORE_CHANNEL_H_

#include <stdint.h>
#include "support.h"
#include "queue.h"
#include "thread.h"

typedef struct quantum {
	thread_t thread;
	uint8_t * itembuf;
	queue_item_t link;
} quantum;

typedef struct channel {
	int32_t elemsize;
	int32_t bufsize;
	int32_t nbuf;
	int32_t recvx;
	int32_t sendx;
	queue_t recv_que;
	queue_t send_que;
	uint8_t * buf;
	lock_t lock;
} * channel_t;

channel_t channel_allocate (int32_t elemsize, int32_t bufsize);
void channel_dealloc (channel_t chan);

int channel_send (channel_t chan, void * buf);
int channel_recv (channel_t chan, void * buf);
int channel_nbsend (channel_t chan, void * buf);
int channel_nbrecv (channel_t chan, void * buf);

#if defined(ENABLE_CHANNEL_SELECT)
/* TODO : multi-channel send / recv , like select clause in GoLang .. */
int channel_select (selector_t sel, void * buf);
int channel_nbselect (selector_t sel, void * buf);

int selector_send (selector_t sel, channel_t chan, chan_callback_t call);
int selector_recv (selector_t sel, channel_t chan, chan_callback_t call);
#endif

#endif // _TSC_CORE_CHANNEL_H_
