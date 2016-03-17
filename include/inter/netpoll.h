// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_NETPOLL_H_
#define _TSC_NETPOLL_H_

#include <stdint.h>
#include <stdbool.h>

#include "coroc_lock.h"
#include "coroc_time.h"
#include "support.h"
#include "coroutine.h"
#ifdef ENABLE_REFCNT
#include "refcnt.h"
#endif

enum { TSC_NETPOLL_READ = 1, TSC_NETPOLL_WRITE = 2, TSC_NETPOLL_ERROR = 4 };

typedef struct coroc_poll_desc {
  struct coroc_refcnt refcnt;
  bool done;
  int fd;
  int mode;
  int id;  // for fast delete ...
  coroc_coroutine_t wait;
  coroc_inter_timer_t *deadline;
  coroc_lock lock;
} *coroc_poll_desc_t;

// internal netpoll API..
int __coroc_netpoll_init(int max);
int __coroc_netpoll_add(coroc_poll_desc_t desc);
int __coroc_netpoll_rem(coroc_poll_desc_t desc);
int __coroc_netpoll_fini(void);
int __coroc_netpoll_size(void);
bool __coroc_netpoll_polling(int timeout);

void coroc_netpoll_initialize(void);
int coroc_netpoll_wakeup(coroc_poll_desc_t desc);

// the public netpoll API ..
int coroc_net_nonblock(int fd);
int coroc_net_read(int fd, void *buf, int size);
int coroc_net_timed_read(int fd, void *buf, int size, int64_t usec);
int coroc_net_write(int fd, void *buf, int size);
int coroc_net_wait(int fd, int mode);
int coroc_net_timedwait(int fd, int mode, int64_t usec);

int coroc_net_announce(bool istcp, const char *server, int port);
int coroc_net_timed_accept(int fd, char *server, int *port, int64_t usec);
int coroc_net_accept(int fd, char *server, int *port);
int coroc_net_lookup(const char *name, uint32_t *ip);
int coroc_net_timed_dial(bool istcp, const char *server, int port, int64_t usec);
int coroc_net_dial(bool istcp, const char *server, int port);

#endif  // _TSC_NETPOLL_H_
