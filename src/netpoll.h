#ifndef _TSC_NETPOLL_H_
#define _TSC_NETPOLL_H_

#include <stdbool.h>

#include "support.h"
#include "lock.h"
#include "coroutine.h"

enum { TSC_NETPOLL_READ = 1, TSC_NETPOLL_WRITE, };

typedef struct tsc_poll_desc {
  bool done;
  bool ok;
  int fd;
  int mode;
  int id;  // for fast delete ...
  tsc_coroutine_t wait;
  lock lock;
} *tsc_poll_desc_t;

// internal netpoll API..
int __tsc_netpoll_init(int max);
int __tsc_netpoll_add(tsc_poll_desc_t desc);
int __tsc_netpoll_rem(tsc_poll_desc_t desc);
int __tsc_netpoll_fini(void);
int __tsc_netpoll_size(void);
bool __tsc_netpoll_polling(bool block);

void tsc_netpoll_initialize(void);
int tsc_netpoll_wakeup(tsc_poll_desc_t desc, bool ok);

// the public netpoll API ..
int tsc_net_nonblock(int fd);
int tsc_net_read(int fd, void *buf, int size);
int tsc_net_write(int fd, void *buf, int size);
int tsc_net_wait(int fd, int mode);

#endif  // _TSC_NETPOLL_H_
