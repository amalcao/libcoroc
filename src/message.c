#include <string.h>
#include <assert.h>

#include "coroutine.h"
#include "message.h"

static tsc_msg_item_t __tsc_alloc_msg_item(tsc_msg_t *m) {
  tsc_msg_item_t item = TSC_ALLOC(sizeof(*item));
  memcpy(item, m, sizeof(*m));

  queue_item_init(&item->link, item);
  return item;
}

static bool __tsc_copy_to_mque(tsc_chan_t chan, void *buf) {
  tsc_async_chan_t achan = (tsc_async_chan_t)chan;
  tsc_msg_item_t msg = __tsc_alloc_msg_item((tsc_msg_t *)buf);

  queue_add(&achan->mque, &msg->link);
  return true;
}

static bool __tsc_copy_from_mque(tsc_chan_t chan, void *buf) {
  tsc_async_chan_t achan = (tsc_async_chan_t)chan;
  tsc_msg_item_t msg = queue_rem(&achan->mque);

  if (msg != NULL) {
    __chan_memcpy(buf, msg, sizeof(struct tsc_msg));
    // TODO : add a free msg_item list ..
    free(msg);
    return true;
  }

  return false;
}

void tsc_async_chan_init(tsc_async_chan_t achan) {
  tsc_chan_init((tsc_chan_t)achan, sizeof(struct tsc_msg), __tsc_copy_to_mque,
                __tsc_copy_from_mque);
  tsc_refcnt_init((tsc_refcnt_t)achan, TSC_DEALLOC);
  atomic_queue_init(&achan->mque);
}

void tsc_async_chan_fini(tsc_async_chan_t achan) {
  lock_acquire(&achan->_chan.lock);
  tsc_msg_item_t msg = 0;
  while ((msg = queue_rem(&achan->mque)) != NULL) {
    // FIXME : memory leak may happen here !!
    free(msg);
  }
  achan->_chan.close = true;  // !!
  lock_release(&achan->_chan.lock);
}

int tsc_send(tsc_coroutine_t target, void *buf, int32_t size) {
  assert(target != NULL);

  struct tsc_msg _msg;
  char *tmp = TSC_ALLOC(size);
  __chan_memcpy(tmp, buf, size);

  _msg.size = size;
  _msg.msg = tmp;

  _tsc_chan_send((tsc_chan_t)target, &_msg, true);
  return CHAN_SUCCESS;
}

int tsc_recv(void *buf, int32_t size, bool block) {
  struct tsc_msg _msg;
  int ret = _tsc_chan_recv(NULL, &_msg, block);

  // TODO : note the memory has been allocated and copied twice,
  // sometimes we can optimize this by just copying a pointer, maybe ..
  __chan_memcpy(buf, _msg.msg, (size < _msg.size) ? size : _msg.size);
  free(_msg.msg);

  return ret;
}

int tsc_sendp(tsc_coroutine_t target, void *ptr, int32_t size) {
  assert(target != NULL);
  struct tsc_msg _msg = {size, ptr};

  _tsc_chan_send((tsc_chan_t)target, &_msg, true);
  return CHAN_SUCCESS;
}

int tsc_recvp(void **ptr, int32_t *size, bool block) {
  struct tsc_msg _msg;
  int ret = _tsc_chan_recv(NULL, &_msg, block);
  *ptr = _msg.msg;
  *size = _msg.size;

  return ret;
}
