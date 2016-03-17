// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>

#include "netpoll.h"
#include "refcnt.h"

static int __coroc_kqueue = -1;
static int __coroc_num_op = 0;

int __coroc_netpoll_init(int max) {
  __coroc_kqueue = kqueue() ;
  return __coroc_kqueue;
}

int __coroc_netpoll_fini(void) {
  // TODO
  return 0;
}

static inline int __coroc_netpoll_ctl(coroc_poll_desc_t desc, uint16_t flags) {
  struct kevent ev;

  ev.ident = desc->fd;

  if (desc->mode & TSC_NETPOLL_READ) ev.filter |= EVFILT_READ;
  if (desc->mode & TSC_NETPOLL_WRITE) ev.filter |= EVFILT_WRITE;
  
  ev.flags = flags;
  ev.fflags = 0;
  ev.data = 0;
  ev.udata = desc;

  return kevent(__coroc_kqueue, &ev, 1, NULL, 0, NULL);
}

int __coroc_netpoll_add(coroc_poll_desc_t desc) {
  coroc_refcnt_get(desc);  // inc the refcnt !!
  TSC_ATOMIC_INC(__coroc_num_op);
  return __coroc_netpoll_ctl(desc, EV_ADD | EV_ENABLE);
}

int __coroc_netpoll_rem(coroc_poll_desc_t desc) {
  // dec the refcnt!!
  // NOTE the `desc' will not be freed here
  // since the refcnt is at least 1 !!
  coroc_refcnt_put(desc);
  TSC_ATOMIC_DEC(__coroc_num_op);
  return __coroc_netpoll_ctl(desc, EV_DELETE);
}

bool __coroc_netpoll_polling(int timeout) {
  struct kevent events[128];
  struct timespec tmout, *ptmout = NULL;
  int ready, i, mode = 0;

  if (timeout >= 0) {
    tmout.tv_sec = timeout / 1000;
    tmout.tv_nsec = (timeout % 1000) * 1000000;
    ptmout = &tmout;
  }

  ready = kevent(__coroc_kqueue, events, 128, NULL, 0, ptmout);

  if (ready <= 0) return false;

  for (i = 0; i < ready; i++) {
    coroc_poll_desc_t desc = (coroc_poll_desc_t)events[i].udata;

    // FIXME: hazard checking here, make sure only one
    //        thread will wakeup this desc !!
    if (!TSC_CAS(&desc->done, false, true)) 
      continue;

    if (events[i].flags & EV_ERROR) {
      mode = TSC_NETPOLL_ERROR;
    } else {
      if ((desc->mode & TSC_NETPOLL_READ) &&
          (events[i].filter & EVFILT_READ))
        mode |= TSC_NETPOLL_READ;

      if ((desc->mode & TSC_NETPOLL_WRITE) && (events[i].filter & EVFILT_WRITE))
        mode |= TSC_NETPOLL_WRITE;
    }
    // the mode as the return value ..
    desc->mode = mode;
    coroc_netpoll_wakeup(desc);
  }

  return true;
}

int __coroc_netpoll_size(void) { return __coroc_num_op; }
