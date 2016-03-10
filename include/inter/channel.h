// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_CORE_CHANNEL_H_
#define _TSC_CORE_CHANNEL_H_

#include <stdint.h>
#include <string.h>
#include "support.h"
#include "refcnt.h"
#include "tsc_queue.h"
#include "lock_chain.h"

enum { CHAN_SUCCESS = 0, CHAN_AWAKEN = 1, CHAN_BUSY = 2, CHAN_CLOSED = 4, };

struct tsc_chan;
typedef bool (*tsc_chan_handler)(struct tsc_chan *, void *);

// the general channel type ..
typedef struct tsc_chan {
  struct tsc_refcnt refcnt;
  bool close;
  bool select;
  tsc_lock lock;
  int32_t isref:1;
  int32_t elemsize:31;
  queue_t recv_que;
  queue_t send_que;
  tsc_chan_handler copy_to_buff;
  tsc_chan_handler copy_from_buff;
} *tsc_chan_t;

// the buffered channel ..
typedef struct tsc_buffered_chan {
  struct tsc_chan _chan;
  int32_t bufsize;
  int32_t nbuf;
  int32_t recvx;
  int32_t sendx;
  uint8_t *buf;
} *tsc_buffered_chan_t;

extern void _tsc_chan_dealloc(tsc_chan_t);

// init the general channel ..
static inline void tsc_chan_init(tsc_chan_t ch, int32_t elemsize, bool isref,
                                 tsc_chan_handler to, tsc_chan_handler from) {
  tsc_refcnt_init(&ch->refcnt, (release_handler_t)_tsc_chan_dealloc);

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

extern bool __tsc_copy_to_buff(tsc_chan_t, void *);
extern bool __tsc_copy_from_buff(tsc_chan_t, void *);

// init the buffered channel ..
static inline void tsc_buffered_chan_init(tsc_buffered_chan_t ch,
                                          int32_t elemsize, 
                                          int32_t bufsize, bool isref) {
  tsc_chan_init((tsc_chan_t)ch, elemsize, isref, __tsc_copy_to_buff,
                __tsc_copy_from_buff);

  ch->bufsize = bufsize;
  ch->buf = (uint8_t *)(ch + 1);
  ch->nbuf = 0;
  ch->recvx = ch->sendx = 0;
}

tsc_chan_t _tsc_chan_allocate(int32_t elemsize, int32_t bufsize, bool isref);
void _tsc_chan_dealloc(tsc_chan_t chan);

#define tsc_chan_allocate(es, bs) _tsc_chan_allocate(es, bs, false)
#define tsc_chan_dealloc(chan) _tsc_chan_dealloc(chan)

extern int _tsc_chan_send(tsc_chan_t chan, void *buf, bool block);
extern int _tsc_chan_recv(tsc_chan_t chan, void *buf, bool block);

#define tsc_chan_send(chan, buf) _tsc_chan_send(chan, buf, true)
#define tsc_chan_recv(chan, buf) _tsc_chan_recv(chan, buf, true)
#define tsc_chan_nbsend(chan, buf) _tsc_chan_send(chan, buf, false)
#define tsc_chan_nbrecv(chan, buf) _tsc_chan_recv(chan, buf, false)

#if 0
extern int _tsc_chan_sendp(tsc_chan_t chan, void *ptr, bool block);
extern int _tsc_chan_recvp(tsc_chan_t chan, void **pptr, bool block);
#else
#define _tsc_chan_sende(chan, exp, block)          \
  ({                                               \
    typeof(exp) __temp = exp;                      \
    assert(sizeof(__temp) == chan->elemsize);      \
    int rc = _tsc_chan_send(chan, &__temp, block); \
    rc;                                            \
  })

#define tsc_chan_sende(chan, exp) _tsc_chan_sende(chan, exp, true)
#define tsc_chan_nbsende(chan, exp) _tsc_chan_sende(chan, exp, false)

#define _tsc_chan_sendp _tsc_chan_sende
#endif

#define tsc_chan_sendp(chan, ptr) _tsc_chan_sendp(chan, ptr, true)
#define tsc_chan_nbsendp(chan, ptr) _tsc_chan_sendp(chan, ptr, false)

extern int tsc_chan_close(tsc_chan_t chan);

enum { CHAN_SEND = 0, CHAN_RECV, };

typedef struct {
  int type;
  tsc_chan_t chan;
  void *buf;
} tsc_scase_t;

typedef struct tsc_chan_set {
  // this is a lock chain actrually ..
  struct {
    bool sorted;
    int volume;
    int size;
    lock_t *locks;
  };
  tsc_scase_t cases[0];
} *tsc_chan_set_t;

#define CHAN_SET_SIZE(n) \
  (sizeof(struct tsc_chan_set) + (n) * (sizeof(tsc_scase_t) + sizeof(lock_t)))

/* multi-channel send / recv , like select clause in GoLang .. */
tsc_chan_set_t tsc_chan_set_allocate(int n);
void tsc_chan_set_dealloc(tsc_chan_set_t set);

static inline 
void tsc_chan_set_init(tsc_chan_set_t set, int n) {
  set->sorted = false;
  set->volume = n;
  set->size = 0;
  set->locks = (lock_t*)(&set->cases[n]);
}

void tsc_chan_set_send(tsc_chan_set_t set, tsc_chan_t chan, void *buf);
void tsc_chan_set_recv(tsc_chan_set_t set, tsc_chan_t chan, void *buf);

extern int _tsc_chan_set_select(tsc_chan_set_t set, bool block,
                                tsc_chan_t *active);

#define tsc_chan_set_select(set, pchan) _tsc_chan_set_select(set, true, pchan)
#define tsc_chan_set_nbselect(set, pchan) \
  _tsc_chan_set_select(set, false, pchan)

static inline void __chan_memcpy(void *dst, const void *src, size_t size) {
  if (dst && src) memcpy(dst, src, size);
}

#endif  // _TSC_CORE_CHANNEL_H_
