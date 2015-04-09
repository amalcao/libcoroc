#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "vpu.h"
#include "netpoll.h"
#include "time.h"

TSC_SIGNAL_MASK_DECLARE

int tsc_net_nonblock(int fd) {
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

int tsc_net_read(int fd, void *buf, int n) {
  int m, total;
  for (total = 0; total < n; total += m) {
    while ((m = read(fd, (char*)buf + total, n - total)) < 0 &&
            errno == EAGAIN)
      tsc_net_wait(fd, TSC_NETPOLL_READ);
    if (m < 0) return m;
    if (m == 0) break;
  }
  return total;
}

int tsc_net_timed_read(int fd, void *buf, int n, int64_t timeout) {
  int m;
  while ((m = read(fd, buf, n)) < 0 && errno == EAGAIN)
    if (!tsc_net_timedwait(fd, TSC_NETPOLL_READ, timeout))
      return -1;
  return m;
}

int tsc_net_write(int fd, void *buf, int n) {
  int m, total;

  for (total = 0; total < n; total += m) {
    while ((m = write(fd, (char *)buf + total, n - total)) < 0 &&
           errno == EAGAIN)
      tsc_net_wait(fd, TSC_NETPOLL_WRITE);
    if (m < 0) return m;
    if (m == 0) break;
  }

  return total;
}

static void __tsc_poll_desc_init(tsc_poll_desc_t desc, int fd, int mode,
                                 tsc_inter_timer_t *deadline) {
  desc->done = false;
  desc->fd = fd;
  desc->mode = mode;
  desc->wait = tsc_coroutine_self();
  desc->deadline = deadline;
  lock_init(&desc->lock);

  tsc_refcnt_init(&desc->refcnt, TSC_DEALLOC);
}

int tsc_net_wait(int fd, int mode) {
  tsc_poll_desc_t desc = TSC_ALLOC(sizeof(struct tsc_poll_desc));
  __tsc_poll_desc_init(desc, fd, mode, NULL);

  TSC_SIGNAL_MASK();
  // calling the low level Netpoll APIs to add ..
  lock_acquire(&desc->lock);
  __tsc_netpoll_add(desc);
  // then suspend current thread ..
  vpu_suspend(&desc->lock, (unlock_handler_t)lock_release);

  TSC_SIGNAL_UNMASK();

  mode = desc->mode;
  tsc_refcnt_put(desc);

  return mode;
}

static void __tsc_netpoll_timeout(void *arg) {
  TSC_SIGNAL_MASK();

  tsc_inter_timer_t *timer = (tsc_inter_timer_t *)arg;
  tsc_poll_desc_t desc = timer->args;

  lock_acquire(&desc->lock);
  // hazard checking ..
  if (!TSC_CAS(&desc->done, false, true)) goto __exit_netpoll_timeout;

  desc->mode = 0;

  __tsc_netpoll_rem(desc);
  vpu_ready(desc->wait);

__exit_netpoll_timeout:
  lock_release(&desc->lock);
  tsc_refcnt_put(desc);  // dec the refcnt !!

  TSC_SIGNAL_UNMASK();
}

int tsc_net_timedwait(int fd, int mode, int64_t usec) {
  assert (usec > 0);

  tsc_inter_timer_t deadline;
  struct tsc_poll_desc *desc = TSC_ALLOC(sizeof(struct tsc_poll_desc));
  __tsc_poll_desc_init(desc, fd, mode, &deadline);

  deadline.when = tsc_getmicrotime() + usec;
  deadline.period = 0;
  deadline.func = __tsc_netpoll_timeout;
  deadline.args = tsc_refcnt_get(desc);  // inc the refcnt!!
  deadline.owner = NULL;

  TSC_SIGNAL_MASK();
  lock_acquire(&desc->lock);
  __tsc_netpoll_add(desc);
  // add the timer to do timeout check..
  tsc_add_intertimer(&deadline);
  // then suspend current task ..
  vpu_suspend(&desc->lock, (unlock_handler_t)lock_release);
  // delete the deadline timer here!!
  if (tsc_del_intertimer(&deadline) == 0) tsc_refcnt_put(desc);

  // drop the reference ..
  mode = desc->mode;
  tsc_refcnt_put(desc);

  TSC_SIGNAL_UNMASK();
  return mode;
}

int tsc_netpoll_wakeup(tsc_poll_desc_t desc) {
  lock_acquire(&desc->lock);

  // hazard checking, maybe not neccessary..
  // if (desc->done) goto __exit_netpoll_wakeup;
  if (!TSC_CAS(&desc->done, false, true)) goto __exit_netpoll_wakeup;

  __tsc_netpoll_rem(desc);
  // this function must be called by
  // system context, so don not need
  // to mask the signals ..
  vpu_ready(desc->wait);
  // put the ready task back to running queue
  // before removing the `desc' from the netpoll !!

__exit_netpoll_wakeup:
  lock_release(&desc->lock);
  return 0;
}

void tsc_netpoll_initialize(void) { __tsc_netpoll_init(128); }
