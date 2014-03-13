#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "netpoll.h"


static int __tsc_epfd = -1;
static int __tsc_num_op = 0;

int __tsc_netpoll_init (int max)
{
  __tsc_epfd = epoll_create(max);
  return __tsc_epfd;
}

int __tsc_netpoll_fini (void)
{
  // TODO
  return 0;
}

static inline int __tsc_netpoll_ctl (tsc_poll_desc_t desc, int op)
{
  struct epoll_event ev;
  ev.events = EPOLLET;

  if (desc -> mode & TSC_NETPOLL_READ)
    ev.events |= (EPOLLIN|EPOLLRDHUP);
  if (desc -> mode & TSC_NETPOLL_WRITE)
    ev.events |= EPOLLOUT;

  ev.data.ptr = desc;

  return epoll_ctl (__tsc_epfd, op, desc -> fd, & ev);
}

int __tsc_netpoll_add (tsc_poll_desc_t desc)
{
  TSC_ATOMIC_INC( __tsc_num_op);
  return __tsc_netpoll_ctl (desc, EPOLL_CTL_ADD);
}

int __tsc_netpoll_rem (tsc_poll_desc_t desc)
{
  TSC_ATOMIC_INC(__tsc_num_op);
  return __tsc_netpoll_ctl (desc, EPOLL_CTL_DEL);
}

bool __tsc_netpoll_polling (bool block)
{
  struct epoll_event events[128];
  int timeout = -1, ready, i;
  if (!block) timeout = 0;

  ready = epoll_wait (__tsc_epfd, events, 128, timeout);

  if (ready <= 0)
    return false;

  for (i = 0; i < ready; i++) {
      tsc_poll_desc_t desc = events[i].data.ptr;
      if ((desc -> mode & TSC_NETPOLL_READ) &&
          (events[i].events & (EPOLLIN | EPOLLRDHUP)))
        tsc_netpoll_wakeup (desc, true);

      else if ((desc -> mode & TSC_NETPOLL_WRITE) &&
               (events[i].events & EPOLLOUT))
        tsc_netpoll_wakeup (desc, true);
      else if (events[i].events & EPOLLERR)
        tsc_netpoll_wakeup (desc, false);

      /* FIXME: some data competition may happen here!! 
       * the deleted `desc' will trigger the epoll after dead! */
#if 1 
      else
        assert (0); // !!
#endif
  }

  return true;
}

int __tsc_netpoll_size (void)
{
    return __tsc_num_op;
}

