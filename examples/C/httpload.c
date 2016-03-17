// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "libcoroc.h"

char *server;
char *url;

int fetchtask(void *);

int main(int argc, char **argv) {
  int i, n;

  if (argc != 4) {
    fprintf(stderr, "usage: httpload n server url\n");
    coroc_coroutine_exit(-1);
  }
  n = atoi(argv[1]);
  server = argv[2];
  url = argv[3];

  for (i = 0; i < n; i++) {
    coroc_coroutine_allocate(fetchtask, 0, "fetch", 
                           TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, 0);
    coroc_udelay(1000);
  }

  while (1) coroc_coroutine_yield();

  coroc_coroutine_exit(0);
}

int fetchtask(void *v) {
  int fd, n;
  char buf[512];

  fprintf(stderr, "starting...\n");
  for (;;) {
    if ((fd = coroc_net_dial(true, server, 80)) < 0) {
      fprintf(stderr, "dial %s: %s\n", server, strerror(errno));
      continue;
    }
    snprintf(buf, sizeof buf, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", url,
             server);
    coroc_net_write(fd, buf, strlen(buf));
    while ((n = coroc_net_read(fd, buf, sizeof buf)) > 0)
      ;
    close(fd);
    write(1, ".", 1);
  }

  coroc_coroutine_exit(0);
}
