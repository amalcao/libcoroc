#ifndef _TSC_NETPOLL_H_
#define _TSC_NETPOLL_H_

#include <stdbool.h>

#include "support.h"
#include "lock.h"
#include "coroutine.h"
#include "time.h"

enum { TSC_NETPOLL_READ = 1, TSC_NETPOLL_WRITE = 2, TSC_NETPOLL_ERROR = 4 };

typedef struct tsc_poll_desc {
  bool done;
  int fd;
  int mode;
  int id;  // for fast delete ...
  tsc_coroutine_t wait;
  tsc_inter_timer_t *deadline;
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
int tsc_netpoll_wakeup(tsc_poll_desc_t desc);

// the public netpoll API ..
int tsc_net_nonblock(int fd);
int tsc_net_read(int fd, void *buf, int size);
int tsc_net_write(int fd, void *buf, int size);
int tsc_net_wait(int fd, int mode);

int tsc_net_announce(bool istcp, const char *server, int port);
int tsc_net_accept(int fd, char *server, int *port);
int tsc_net_lookup(const char *name, uint32_t *ip);
int tsc_net_dial(bool istcp, char *server, int port);

#endif  // _TSC_NETPOLL_H_
