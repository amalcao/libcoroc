// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>

#include "libcoroc.h"

coroc_coroutine_t init;

int subtask(coroc_chan_t chan) {
  int ret, id;
  while (1) {
    ret = coroc_chan_recv(chan, &id);
    // printf ("[subtask] %x\n", ret);
    if (ret == CHAN_CLOSED) break;
    printf("[subtask] recv id is %d.\n", id);
  }

  coroc_sendp(init, NULL, 0);
  coroc_refcnt_put((coroc_refcnt_t)chan);
  coroc_coroutine_exit(0);
}

int main(int argc, char** argv) {
  init = coroc_coroutine_self();

  uint64_t awaken = 0;
  int i = 0;
  coroc_chan_t chan = coroc_chan_allocate(sizeof(int), 0);

  coroc_timer_t timer = coroc_timer_allocate(1000000 * 1, NULL);
  coroc_timer_after(timer, 1000000 * 1);  // 1 seconds later

  coroc_coroutine_allocate(subtask, coroc_refcnt_get(chan), "sub",
                         TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, 0);

  for (i = 0; i < 5; i++) {
    printf("waiting for 1 seconds!\n");
    coroc_chan_recv((coroc_chan_t)timer, &awaken);
    coroc_chan_sende(chan, i + 1);
  }

  printf("release the timer ..\n");
  coroc_chan_close(chan);
  coroc_refcnt_put((coroc_refcnt_t)chan);

  coroc_timer_stop(timer);
  coroc_timer_dealloc(timer);

  coroc_recv(NULL, 0, true);

  coroc_coroutine_exit(0);
}
