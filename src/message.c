// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <string.h>
#include <assert.h>

#include "coroutine.h"
#include "message.h"

static coroc_msg_item_t __coroc_alloc_msg_item(coroc_msg_t *m) {
  coroc_msg_item_t item = TSC_ALLOC(sizeof(*item));
  memcpy(item, m, sizeof(*m));

  queue_item_init(&item->link, item);
  return item;
}

static bool __coroc_copy_to_mque(coroc_chan_t chan, void *buf) {
  coroc_async_chan_t achan = (coroc_async_chan_t)chan;
  coroc_msg_item_t msg = __coroc_alloc_msg_item((coroc_msg_t *)buf);

  queue_add(&achan->mque, &msg->link);
  return true;
}

static bool __coroc_copy_from_mque(coroc_chan_t chan, void *buf) {
  coroc_async_chan_t achan = (coroc_async_chan_t)chan;
  coroc_msg_item_t msg = queue_rem(&achan->mque);

  if (msg != NULL) {
    __chan_memcpy(buf, msg, sizeof(struct coroc_msg));
    // TODO : add a free msg_item list ..
    free(msg);
    return true;
  }

  return false;
}

void coroc_async_chan_init(coroc_async_chan_t achan) {
  coroc_chan_init((coroc_chan_t)achan, sizeof(struct coroc_msg), false, __coroc_copy_to_mque,
                __coroc_copy_from_mque);
  coroc_refcnt_init((coroc_refcnt_t)achan, TSC_DEALLOC);
  atomic_queue_init(&achan->mque);
}

void coroc_async_chan_fini(coroc_async_chan_t achan) {
  lock_acquire(&achan->_chan.lock);
  coroc_msg_item_t msg = 0;
  while ((msg = queue_rem(&achan->mque)) != NULL) {
    // FIXME : memory leak may happen here !!
    free(msg);
  }
  achan->_chan.close = true;  // !!
  lock_release(&achan->_chan.lock);
}

int coroc_send(coroc_coroutine_t target, void *buf, int32_t size) {
  assert(target != NULL);

  struct coroc_msg _msg;
  char *tmp = TSC_ALLOC(size);
  __chan_memcpy(tmp, buf, size);

  _msg.size = size;
  _msg.msg = tmp;

  _coroc_chan_send((coroc_chan_t)target, &_msg, true);
  return CHAN_SUCCESS;
}

int coroc_recv(void *buf, int32_t size, bool block) {
  struct coroc_msg _msg;
  int ret = _coroc_chan_recv(NULL, &_msg, block);

  // TODO : note the memory has been allocated and copied twice,
  // sometimes we can optimize this by just copying a pointer, maybe ..
  __chan_memcpy(buf, _msg.msg, (size < _msg.size) ? size : _msg.size);
  free(_msg.msg);

  return ret;
}

int coroc_sendp(coroc_coroutine_t target, void *ptr, int32_t size) {
  assert(target != NULL);
  struct coroc_msg _msg = {size, ptr};

  _coroc_chan_send((coroc_chan_t)target, &_msg, true);
  return CHAN_SUCCESS;
}

int coroc_recvp(void **ptr, int32_t *size, bool block) {
  struct coroc_msg _msg;
  int ret = _coroc_chan_recv(NULL, &_msg, block);
  *ptr = _msg.msg;
  *size = _msg.size;

  return ret;
}
