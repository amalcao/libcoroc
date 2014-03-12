#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "netpoll.h"

struct {
    int cap;
    int size;
    struct pollfd *fds;
    tsc_poll_desc_t *table;
    pthread_mutex_t mutex;
} tsc_netpoll_manager;

int __tsc_netpoll_init (int max)
{
  tsc_netpoll_manager . cap = max;
  tsc_netpoll_manager . size = 0;
  tsc_netpoll_manager . fds = malloc(max * sizeof(struct pollfd));
  tsc_netpoll_manager . table = malloc(max * sizeof(void*));

  pthread_mutex_init(& tsc_netpoll_manager . mutex, NULL);

  return 0;
}

int __tsc_netpoll_fini (void)
{
  free (tsc_netpoll_manager . fds);
  free (tsc_netpoll_manager . table);
  return 0;
}

int __tsc_netpoll_add (tsc_poll_desc_t desc)
{
  pthread_mutex_lock (& tsc_netpoll_manager . mutex);

  // realloc the buffer if the number of waiting requests
  // exceed the current capacity ..
  if (tsc_netpoll_manager . size == tsc_netpoll_manager . cap) {
      tsc_netpoll_manager . cap *= 2;
      tsc_netpoll_manager . fds = realloc (
                                           tsc_netpoll_manager . fds,
                                           tsc_netpoll_manager .cap * sizeof(struct pollfd));
      tsc_netpoll_manager . table = realloc (
                                             tsc_netpoll_manager . table,
                                             tsc_netpoll_manager . cap * sizeof(void*) );
  }

  int size = tsc_netpoll_manager . size;

  tsc_netpoll_manager . table[size] = desc;
  desc -> id = size; // !!

  tsc_netpoll_manager . fds[size] . fd = desc -> fd;
  tsc_netpoll_manager . fds[size] . events = 0;
  tsc_netpoll_manager . fds[size] . revents = 0;

  if (desc -> mode & TSC_NETPOLL_READ)
    tsc_netpoll_manager . fds[size] . events |= POLLIN;
  if (desc -> mode & TSC_NETPOLL_WRITE)
    tsc_netpoll_manager . fds[size] . events |= POLLOUT;

  tsc_netpoll_manager . size ++;

  pthread_mutex_unlock (& tsc_netpoll_manager . mutex);
}

int __tsc_netpoll_rem (tsc_poll_desc_t desc)
{
  int id, size;
  struct pollfd *pfds = & tsc_netpoll_manager . fds[0];

  // pthread_mutex_lock (& tsc_netpoll_manager . mutex);

  id = desc -> id;
  size = tsc_netpoll_manager . size;
  // delete current one, and copy the last one 
  // to current's spot..
  memcpy (& pfds[id], & pfds[size - 1], sizeof(struct pollfd));
  tsc_netpoll_manager . table[id] = tsc_netpoll_manager . table [size-1];
  tsc_netpoll_manager . table[id] -> id = id;

  tsc_netpoll_manager . size --;

  // pthread_mutex_unlock (& tsc_netpoll_manager . mutex);
}

bool __tsc_netpoll_polling (bool block)
{
  bool ret = false;
  int size, ms, i;
  struct pollfd *pfds = tsc_netpoll_manager . fds;

  pthread_mutex_lock (& tsc_netpoll_manager . mutex);

  size = tsc_netpoll_manager . size;

  if (size == 0) 
    goto __exit_polling;
    
  ms = block ? -1 : 0;

  if (poll (pfds, size, ms) <= 0)
    goto __exit_polling;

  for (i = 0; i < size; i++) {
    if (pfds[i] . revents) {
        tsc_poll_desc_t desc = tsc_netpoll_manager . table[i];
        tsc_netpoll_wakeup (desc, true);
    }
  }
  
__exit_polling:
  pthread_mutex_unlock (& tsc_netpoll_manager . mutex);
  return ret;
}

int __tsc_netpoll_size (void)
{
    return tsc_netpoll_manager . size;
}
