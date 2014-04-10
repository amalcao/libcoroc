#ifndef _TSC_CORE_MESSAGE_H_
#define _TSC_CORE_MESSAGE_H_

#include "channel.h"

typedef struct tsc_async_chan {
    struct tsc_chan _chan;
    queue_t mque;
} * tsc_async_chan_t;

typedef struct tsc_msg {
    int32_t size;
    void * msg;
} tsc_msg_t;

typedef struct {
    struct tsc_msg _msg;
    queue_item_t link;
} * tsc_msg_item_t;

struct tsc_coroutine;

extern void tsc_async_chan_init (tsc_async_chan_t);
extern void tsc_async_chan_fini (tsc_async_chan_t);

extern int tsc_send (struct tsc_coroutine*, void *, int32_t);
extern int tsc_recv (void *, int32_t , bool);

extern int tsc_sendp (struct tsc_coroutine*, void *, int32_t);
extern int tsc_recvp (void **, int32_t*, bool);

#endif // _TSC_CORE_MESSAGE_H_
