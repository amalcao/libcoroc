#ifndef _TSC_CORE_CHANNEL_H_
#define _TSC_CORE_CHANNEL_H_

#include <stdint.h>
#include "support.h"
#include "queue.h"
#include "lock_chain.h"

enum {
    CHAN_SUCCESS = 0,
    CHAN_AWAKEN,
    CHAN_BUSY,
};

struct tsc_chan;
typedef bool (*tsc_chan_handler)(struct tsc_chan*, void*);

// the general channel type ..
typedef struct tsc_chan {
    bool    select;
    lock    lock;
    int32_t elemsize;
    queue_t recv_que;
    queue_t send_que;
    tsc_chan_handler copy_to_buff;
    tsc_chan_handler copy_from_buff;
} * tsc_chan_t;

// the buffered channel ..
typedef struct tsc_buffered_chan {
    struct  tsc_chan _chan;
    int32_t bufsize;
    int32_t nbuf;
    int32_t recvx;
    int32_t sendx;
    uint8_t *buf;
} * tsc_buffered_chan_t;

// init the general channel ..
static inline void tsc_chan_init (
    tsc_chan_t ch, int32_t elemsize,
    tsc_chan_handler to, tsc_chan_handler from) {
    ch -> select = false;
    ch -> elemsize = elemsize;
    ch -> copy_to_buff = to;
    ch -> copy_from_buff = from;

    lock_init (& ch -> lock);
    queue_init (& ch -> recv_que);
    queue_init (& ch -> send_que);
}

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
