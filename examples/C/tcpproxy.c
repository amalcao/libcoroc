// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

#include "libcoroc.h"

char *server;
int port;
int proxy_task(void *);
int rwtask(void *);

int *mkfd2(int fd1, int fd2) {
  int *a;

  a = malloc(2 * sizeof a[0]);
  if (a == 0) {
    fprintf(stderr, "out of memory\n");
    abort();
  }

  a[0] = fd1;
  a[1] = fd2;

  return a;
}

int main(int argc, char **argv) {
  int cfd, fd;
  int rport;
  char remote[16];

  if (argc != 4) {
    fprintf(stderr, "usage: tcpproxy localport server remoteport\n");
    coroc_coroutine_exit(-1);
  }

  server = argv[2];
  port = atoi(argv[3]);

  if ((fd = coroc_net_announce(true, 0, atoi(argv[1]))) < 0) {
    fprintf(stderr, "cannot announce on tcp port %d: %s\n", atoi(argv[1]),
            strerror(errno));
    coroc_coroutine_exit(-1);
  }

  coroc_net_nonblock(fd);
  while ((cfd = coroc_net_accept(fd, remote, &rport)) >= 0) {
    fprintf(stderr, "connection from %s:%d\n", remote, rport);
    coroc_coroutine_allocate(proxy_task, (void*)cfd, "proxy",
                           TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, NULL);
  }

  coroc_coroutine_exit(0);
}

int proxy_task(void *v) {
  int fd, remotefd;

  fd = (int)v;
  if ((remotefd = coroc_net_dial(true, server, port)) < 0) {
    close(fd);
    coroc_coroutine_exit(-1);
  }

  fprintf(stderr, "connected to %s:%d\n", server, port);

  coroc_coroutine_allocate(rwtask, mkfd2(fd, remotefd), "rwtask",
                         TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, NULL);
  coroc_coroutine_allocate(rwtask, mkfd2(remotefd, fd), "rwtask",
                         TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, NULL);

  coroc_coroutine_exit(0);
}

int rwtask(void *v) {
  int *a, rfd, wfd, n;
  char buf[2048];

  a = v;
  rfd = a[0];
  wfd = a[1];
  free(a);

  while ((n = coroc_net_read(rfd, buf, sizeof buf)) > 0)
    coroc_net_write(wfd, buf, n);

  shutdown(wfd, SHUT_WR);
  close(rfd);

  coroc_coroutine_exit(0);
}
