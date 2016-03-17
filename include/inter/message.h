// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_CORE_MESSAGE_H_
#define _TSC_CORE_MESSAGE_H_

#include "channel.h"

typedef struct coroc_async_chan {
  struct coroc_chan _chan;
  queue_t mque;
} *coroc_async_chan_t;

typedef struct coroc_msg {
  int32_t size;
  void *msg;
} coroc_msg_t;

typedef struct {
  struct coroc_msg _msg;
  queue_item_t link;
} *coroc_msg_item_t;

struct coroc_coroutine;

extern void coroc_async_chan_init(coroc_async_chan_t);
extern void coroc_async_chan_fini(coroc_async_chan_t);

extern int coroc_send(struct coroc_coroutine *, void *, int32_t);
extern int coroc_recv(void *, int32_t, bool);

extern int coroc_sendp(struct coroc_coroutine *, void *, int32_t);
extern int coroc_recvp(void **, int32_t *, bool);

#endif  // _TSC_CORE_MESSAGE_H_
