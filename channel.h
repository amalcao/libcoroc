#ifndef _TSC_CORE_CHANNEL_H_
#define _TSC_CORE_CHANNEL_H_

#include <stdint.h>
#include "support.h"
#include "queue.h"
#include "thread.h"
#include "lock_chain.h"

enum {
	CHAN_SUCCESS,
	CHAN_AWAKEN,
	CHAN_BUSY,
};

typedef struct channel {
	bool	select;
	int32_t elemsize;
	int32_t bufsize;
	int32_t nbuf;
	int32_t recvx;
	int32_t sendx;
	queue_t recv_que;
	queue_t send_que;
	uint8_t * buf;
	lock lock;
} * channel_t;

channel_t channel_allocate (int32_t elemsize, int32_t bufsize);
void channel_dealloc (channel_t chan);

extern int _channel_send (channel_t chan, void * buf, bool block);
extern int _channel_recv (channel_t chan, void * buf, bool block);

#define channel_send(chan, buf) _channel_send(chan, buf, true)
#define channel_recv(chan, buf) _channel_recv(chan, buf, true)
#define channel_nbsend(chan, buf) _channel_send(chan, buf, false)
#define channel_nbrecv(chan, buf) _channel_recv(chan, buf, false)

#if defined(ENABLE_CHANNEL_SELECT)
typedef struct chan_set {
	queue_t sel_que;
	lock_chain_t locks;
} * chan_set_t;

/* multi-channel send / recv , like select clause in GoLang .. */
chan_set_t chan_set_allocate (void);
void chan_set_dealloc (chan_set_t set);

void chan_set_send (chan_set_t set, channel_t chan, void * buf);
void chan_set_recv (chan_set_t set, channel_t chan, void * buf);

extern int _chan_set_select (chan_set_t set, bool block, channel_t * active);

#define chan_set_select(set, pchan) _chan_set_select(set, true, pchan)
#define chan_set_nbselect(set, pchan) _chan_set_select(set, false, pchan)

#endif // ENABLE_CHANNEL_SELECT

#endif // _TSC_CORE_CHANNEL_H_
