// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

/* Copyright (c) 2005 Russ Cox, MIT; see COPYRIGHT */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libcoroc.h"

int quiet = 0;
int goal = 100;
int buffer = 0;

int primetask(void *arg) {
  coroc_chan_t c, nc;
  unsigned long p, i;
  c = arg;

  coroc_chan_recv(c, &p);

  if (p > goal) exit(0);

  if (!quiet) printf("%d\n", p);

  nc = coroc_chan_allocate(sizeof(unsigned long), buffer);
  coroc_coroutine_allocate(primetask, nc, "", 
                         TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, NULL);
  for (;;) {
    coroc_chan_recv(c, &i);
    if (i % p) coroc_chan_send(nc, &i);
  }
  return 0;
}

void main(int argc, char **argv) {
  unsigned long i;
  coroc_chan_t c;

  printf("goal=%d\n", goal);

  c = coroc_chan_allocate(sizeof(unsigned long), buffer);
  coroc_coroutine_allocate(primetask, c, "", 
                         TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, NULL);
  for (i = 2;; i++) coroc_chan_send(c, &i);
}
