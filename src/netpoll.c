#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "vpu.h"
#include "netpoll.h"

TSC_SIGNAL_MASK_DECLARE

int tsc_net_nonblock (int fd)
{
  return fcntl(fd, F_SETFL, 
               fcntl(fd, F_GETFL)|O_NONBLOCK);
}

int tsc_net_read (int fd, void *buf, int n)
{
  int m;
  while ((m = read(fd, buf, n)) < 0 && errno == EAGAIN)
    tsc_net_wait (fd, TSC_NETPOLL_READ);
  return m;
}

int tsc_net_write (int fd, void *buf, int n)
{
  int m, total;

  for (total = 0; total < n; total+=m) {
      while ((m = write(fd, (char*)buf+total, n-total)) < 0 &&
             errno == EAGAIN) 
        tsc_net_wait (fd, TSC_NETPOLL_WRITE);
      if (m < 0)
        return m;
      if (m == 0)
        break;
  }

  return total;
}

static void __tsc_poll_desc_init (tsc_poll_desc_t desc, int fd, int mode)
{
  desc -> done = false;
  desc -> ok = false;
  desc -> fd = fd;
  desc -> mode = mode;
  desc -> wait = tsc_coroutine_self();
  lock_init (& desc -> lock);
}

int tsc_net_wait (int fd, int mode)
{
  struct tsc_poll_desc desc;
  __tsc_poll_desc_init (& desc, fd, mode);

  TSC_SIGNAL_MASK();
  // calling the low level Netpoll APIs to add ..
  lock_acquire (& desc . lock);
  __tsc_netpoll_add (& desc);
  // then suspend current thread ..
  vpu_suspend (NULL, & desc . lock, (unlock_handler_t)lock_release);

  TSC_SIGNAL_UNMASK();
  return 0;
}

int tsc_netpoll_wakeup (tsc_poll_desc_t desc, bool ok)
{
  lock_acquire (& desc -> lock);

  // hazard checking, maybe not neccessary..
  if (desc -> done)
    goto __exit_netpoll_wakeup;

  desc -> ok = ok;
  desc -> done = true;

  __tsc_netpoll_rem (desc);
  // this function must be called by 
  // system context, so don not need 
  // to mask the signals ..
  vpu_ready (desc -> wait);

__exit_netpoll_wakeup:
  lock_release (& desc -> lock);
  return 0;
}

void tsc_netpoll_initialize (void)
{
  __tsc_netpoll_init (128);
}


