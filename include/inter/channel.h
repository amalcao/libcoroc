// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_CORE_CHANNEL_H_
#define _TSC_CORE_CHANNEL_H_

#include <stdint.h>
#include <string.h>
#include "support.h"
#include "refcnt.h"
#include "coroc_queue.h"
#include "lock_chain.h"

enum { CHAN_SUCCESS = 0, CHAN_AWAKEN = 1, CHAN_BUSY = 2, CHAN_CLOSED = 4, };

struct coroc_chan;
typedef bool (*coroc_chan_handler)(struct coroc_chan *, void *);

// the general channel type ..
typedef struct coroc_chan {
  struct coroc_refcnt refcnt;
  bool close;
  bool select;
  coroc_lock lock;
  int32_t isref:1;
  int32_t elemsize:31;
  queue_t recv_que;
  queue_t send_que;
  coroc_chan_handler copy_to_buff;
  coroc_chan_handler copy_from_buff;
} *coroc_chan_t;

// the buffered channel ..
typedef struct coroc_buffered_chan {
  struct coroc_chan _chan;
  int32_t bufsize;
  int32_t nbuf;
  int32_t recvx;
  int32_t sendx;
  uint8_t *buf;
} *coroc_buffered_chan_t;

extern void _coroc_chan_dealloc(coroc_chan_t);

// init the general channel ..
static inline void coroc_chan_init(coroc_chan_t ch, int32_t elemsize, bool isref,
                                 coroc_chan_handler to, coroc_chan_handler from) {
  coroc_refcnt_init(&ch->refcnt, (release_handler_t)_coroc_chan_dealloc);

  ch->close = false;
  ch->select = false;
  ch->isref = isref ? 1 : 0;
  ch->elemsize = elemsize;
  ch->copy_to_buff = to;
  ch->copy_from_buff = from;

  lock_init(&ch->lock);
  queue_init(&ch->recv_que);
  queue_init(&ch->send_que);
}

extern bool __coroc_copy_to_buff(coroc_chan_t, void *);
extern bool __coroc_copy_from_buff(coroc_chan_t, void *);

// init the buffered channel ..
static inline void coroc_buffered_chan_init(coroc_buffered_chan_t ch,
                                          int32_t elemsize, 
                                          int32_t bufsize, bool isref) {
  coroc_chan_init((coroc_chan_t)ch, elemsize, isref, __coroc_copy_to_buff,
                __coroc_copy_from_buff);

  ch->bufsize = bufsize;
  ch->buf = (uint8_t *)(ch + 1);
  ch->nbuf = 0;
  ch->recvx = ch->sendx = 0;
}

coroc_chan_t _coroc_chan_allocate(int32_t elemsize, int32_t bufsize, bool isref);
void _coroc_chan_dealloc(coroc_chan_t chan);

#define coroc_chan_allocate(es, bs) _coroc_chan_allocate(es, bs, false)
#define coroc_chan_dealloc(chan) _coroc_chan_dealloc(chan)

extern int _coroc_chan_send(coroc_chan_t chan, void *buf, bool block);
extern int _coroc_chan_recv(coroc_chan_t chan, void *buf, bool block);

#define coroc_chan_send(chan, buf) _coroc_chan_send(chan, buf, true)
#define coroc_chan_recv(chan, buf) _coroc_chan_recv(chan, buf, true)
#define coroc_chan_nbsend(chan, buf) _coroc_chan_send(chan, buf, false)
#define coroc_chan_nbrecv(chan, buf) _coroc_chan_recv(chan, buf, false)

#if 0
extern int _coroc_chan_sendp(coroc_chan_t chan, void *ptr, bool block);
extern int _coroc_chan_recvp(coroc_chan_t chan, void **pptr, bool block);
#else
#define _coroc_chan_sende(chan, exp, block)          \
  ({                                               \
    typeof(exp) __temp = exp;                      \
    assert(sizeof(__temp) == chan->elemsize);      \
    int rc = _coroc_chan_send(chan, &__temp, block); \
    rc;                                            \
  })

#define coroc_chan_sende(chan, exp) _coroc_chan_sende(chan, exp, true)
#define coroc_chan_nbsende(chan, exp) _coroc_chan_sende(chan, exp, false)

#define _coroc_chan_sendp _coroc_chan_sende
#endif

#define coroc_chan_sendp(chan, ptr) _coroc_chan_sendp(chan, ptr, true)
#define coroc_chan_nbsendp(chan, ptr) _coroc_chan_sendp(chan, ptr, false)

extern int coroc_chan_close(coroc_chan_t chan);

enum { CHAN_SEND = 0, CHAN_RECV, };

typedef struct {
  int type;
  coroc_chan_t chan;
  void *buf;
} coroc_scase_t;

typedef struct coroc_chan_set {
  // this is a lock chain actrually ..
  struct {
    bool sorted;
    int volume;
    int size;
    lock_t *locks;
  };
  coroc_scase_t cases[0];
} *coroc_chan_set_t;

#define CHAN_SET_SIZE(n) \
  (sizeof(struct coroc_chan_set) + (n) * (sizeof(coroc_scase_t) + sizeof(lock_t)))

/* multi-channel send / recv , like select clause in GoLang .. */
coroc_chan_set_t coroc_chan_set_allocate(int n);
void coroc_chan_set_dealloc(coroc_chan_set_t set);

static inline 
void coroc_chan_set_init(coroc_chan_set_t set, int n) {
  set->sorted = false;
  set->volume = n;
  set->size = 0;
  set->locks = (lock_t*)(&set->cases[n]);
}

void coroc_chan_set_send(coroc_chan_set_t set, coroc_chan_t chan, void *buf);
void coroc_chan_set_recv(coroc_chan_set_t set, coroc_chan_t chan, void *buf);

extern int _coroc_chan_set_select(coroc_chan_set_t set, bool block,
                                coroc_chan_t *active);

#define coroc_chan_set_select(set, pchan) _coroc_chan_set_select(set, true, pchan)
#define coroc_chan_set_nbselect(set, pchan) \
  _coroc_chan_set_select(set, false, pchan)

static inline void __chan_memcpy(void *dst, const void *src, size_t size) {
  if (dst && src) memcpy(dst, src, size);
}

#endif  // _TSC_CORE_CHANNEL_H_
