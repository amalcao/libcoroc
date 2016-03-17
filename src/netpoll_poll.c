// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

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
  coroc_poll_desc_t *table;
  pthread_mutex_t mutex;
} coroc_netpoll_manager;

int __coroc_netpoll_init(int max) {
  coroc_netpoll_manager.cap = max;
  coroc_netpoll_manager.size = 0;
  coroc_netpoll_manager.fds = malloc(max * sizeof(struct pollfd));
  coroc_netpoll_manager.table = malloc(max * sizeof(void *));

  pthread_mutex_init(&coroc_netpoll_manager.mutex, NULL);

  return 0;
}

int __coroc_netpoll_fini(void) {
  free(coroc_netpoll_manager.fds);
  free(coroc_netpoll_manager.table);
  return 0;
}

int __coroc_netpoll_add(coroc_poll_desc_t desc) {
  pthread_mutex_lock(&coroc_netpoll_manager.mutex);

  // realloc the buffer if the number of waiting requests
  // exceed the current capacity ..
  if (coroc_netpoll_manager.size == coroc_netpoll_manager.cap) {
    coroc_netpoll_manager.cap *= 2;
    coroc_netpoll_manager.fds =
        realloc(coroc_netpoll_manager.fds,
                coroc_netpoll_manager.cap * sizeof(struct pollfd));
    coroc_netpoll_manager.table = realloc(
        coroc_netpoll_manager.table, coroc_netpoll_manager.cap * sizeof(void *));
  }

  int size = coroc_netpoll_manager.size;

  coroc_netpoll_manager.table[size] = desc;
  desc->id = size;  // !!

  coroc_netpoll_manager.fds[size].fd = desc->fd;
  coroc_netpoll_manager.fds[size].events = 0;
  coroc_netpoll_manager.fds[size].revents = 0;

  if (desc->mode & TSC_NETPOLL_READ)
    coroc_netpoll_manager.fds[size].events |= POLLIN;
  if (desc->mode & TSC_NETPOLL_WRITE)
    coroc_netpoll_manager.fds[size].events |= POLLOUT;

  coroc_netpoll_manager.size++;

  pthread_mutex_unlock(&coroc_netpoll_manager.mutex);
  return 0;
}

int __coroc_netpoll_rem(coroc_poll_desc_t desc) {
  int id, size;
  struct pollfd *pfds = &coroc_netpoll_manager.fds[0];

  // pthread_mutex_lock (& coroc_netpoll_manager . mutex);

  id = desc->id;
  size = coroc_netpoll_manager.size;
  // delete current one, and copy the last one
  // to current's spot..
  memcpy(&pfds[id], &pfds[size - 1], sizeof(struct pollfd));
  coroc_netpoll_manager.table[id] = coroc_netpoll_manager.table[size - 1];
  coroc_netpoll_manager.table[id]->id = id;

  coroc_netpoll_manager.size--;

  // pthread_mutex_unlock (& coroc_netpoll_manager . mutex);
  return 0;
}

bool __coroc_netpoll_polling(int timeout) {
  int size, i, mode = 0;
  struct pollfd *pfds = coroc_netpoll_manager.fds;
  
  size = coroc_netpoll_manager.size;
  if (size == 0) return false;

  if (poll(pfds, size, timeout) <= 0) return false;

  pthread_mutex_lock(&coroc_netpoll_manager.mutex);

  for (i = 0; i < size; i++) {
    coroc_poll_desc_t desc = coroc_netpoll_manager.table[i];

    // FIXME: hazard checking here, make sure only one
    //        thread will wakeup this desc !!
    if (!TSC_CAS(&desc->done, false, true)) 
      continue;

    if (pfds[i].revents & POLLERR) {
      mode = TSC_NETPOLL_ERROR;
    } else {
        if ((desc->mode & TSC_NETPOLL_READ) &&
            (pfds[i].revents & (POLLIN | POLLRDHUP)))
            mode |= TSC_NETPOLL_READ;
        if ((desc->mode & TSC_NETPOLL_WRITE) &&
            (pfds[i].revents & POLLOUT))
            mode |= TSC_NETPOLL_WRITE;
    }

    desc->mode = mode;
    coroc_netpoll_wakeup(desc);
  }

  pthread_mutex_unlock(&coroc_netpoll_manager.mutex);

  return true;
}

int __coroc_netpoll_size(void) { return coroc_netpoll_manager.size; }
