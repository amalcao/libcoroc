#include <string.h>
#include <assert.h>

#include "coroutine.h"
#include "message.h"

static tsc_msg_item_t __tsc_alloc_msg_item (tsc_msg_t *m)
{
    tsc_msg_item_t item = TSC_ALLOC(sizeof (*item));
    memcpy (item, m, sizeof(*m));
    queue_item_init (& item -> link, item);
    return item;
}

static bool __tsc_copy_to_mque (tsc_chan_t chan, void *buf)
{
    tsc_async_chan_t achan = (tsc_async_chan_t)chan;
    tsc_msg_item_t msg = __tsc_alloc_msg_item((tsc_msg_t*)buf);
    
    queue_add (& achan -> mque, & msg -> link);
    return true;
}

static bool __tsc_copy_from_mque (tsc_chan_t chan, void *buf)
{
    tsc_async_chan_t achan = (tsc_async_chan_t)chan;
    tsc_msg_item_t msg = queue_rem (& achan -> mque);

    if (msg != NULL) {
        memcpy (buf, msg, sizeof(struct tsc_msg));
        // TODO : add a free msg_item list ..
        free (msg); 
        return true;
    }

    return false;
}

int tsc_async_chan_init (tsc_async_chan_t achan)
{
    tsc_chan_init ((tsc_chan_t)achan, sizeof(struct tsc_msg),
        __tsc_copy_to_mque, __tsc_copy_from_mque);
    atomic_queue_init (& achan -> mque);
}

int tsc_send (tsc_coroutine_t target, void *buf, int32_t size)
{
   assert (target != NULL);
   struct tsc_msg _msg = { size, buf };
   _tsc_chan_send ((tsc_chan_t)target, & _msg, true);
   return CHAN_SUCCESS;
}

int tsc_recv (void **buf, int32_t *size, bool block) 
{
    struct tsc_msg _msg;
    tsc_coroutine_t self = tsc_coroutine_self();

    int ret = _tsc_chan_recv((tsc_chan_t)self, & _msg, block);
    *buf = _msg . msg;
    *size = _msg . size;

    return ret;
}
