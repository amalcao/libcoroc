#ifndef _TSC_CORE_CHANNEL_H_
#define _TSC_CORE_CHANNEL_H_

#include <stdint.h>
#include "support.h"
#include "queue.h"
#include "coroutine.h"
#include "lock_chain.h"

enum {
	CHAN_SUCCESS = 0,
	CHAN_AWAKEN,
	CHAN_BUSY,
};

typedef struct tsc_chan {
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
} * tsc_chan_t;

tsc_chan_t tsc_chan_allocate (int32_t elemsize, int32_t bufsize);
void tsc_chan_dealloc (tsc_chan_t chan);

extern int _tsc_chan_send (tsc_chan_t chan, void * buf, bool block);
extern int _tsc_chan_recv (tsc_chan_t chan, void * buf, bool block);

#define tsc_chan_send(chan, buf) _tsc_chan_send(chan, buf, true)
#define tsc_chan_recv(chan, buf) _tsc_chan_recv(chan, buf, true)
#define tsc_chan_nbsend(chan, buf) _tsc_chan_send(chan, buf, false)
#define tsc_chan_nbrecv(chan, buf) _tsc_chan_recv(chan, buf, false)

#if defined(ENABLE_CHANNEL_SELECT)
typedef struct tsc_chan_set {
	queue_t sel_que;
	lock_chain_t locks;
} * tsc_chan_set_t;

/* multi-channel send / recv , like select clause in GoLang .. */
tsc_chan_set_t tsc_chan_set_allocate (void);
void tsc_chan_set_dealloc (tsc_chan_set_t set);

void tsc_chan_set_send (tsc_chan_set_t set, tsc_chan_t chan, void * buf);
void tsc_chan_set_recv (tsc_chan_set_t set, tsc_chan_t chan, void * buf);

extern int _tsc_chan_set_select (tsc_chan_set_t set, bool block, tsc_chan_t * active);

#define tsc_chan_set_select(set, pchan) _tsc_chan_set_select(set, true, pchan)
#define tsc_chan_set_nbselect(set, pchan) _tsc_chan_set_select(set, false, pchan)

#endif // ENABLE_CHANNEL_SELECT

#endif // _TSC_CORE_CHANNEL_H_
