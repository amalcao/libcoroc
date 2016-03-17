// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>

#include "libcoroc.h"

#define SIZE 4

int sub_task(coroc_chan_t chan) {
  int id = random();
  coroc_chan_send(chan, &id);

  coroc_coroutine_exit(0);
}

int sub_task2(coroc_coroutine_t parent) {
  // note: since the `&id' is sent just as a pointer,
  // so the `id' must be allocated on DATA segment (using static keyword),
  // not on the stack which released after coroc_coroutine_exit..
  static int id = 12345;

  coroc_udelay(10000);
  coroc_sendp(parent, &id, sizeof(int));

  coroc_coroutine_exit(0);
}

int main(int argc, char** argv) {
  coroc_coroutine_t me = coroc_coroutine_self();
  coroc_coroutine_t thrds[SIZE];
  coroc_chan_t chans[SIZE];
  coroc_chan_set_t set = coroc_chan_set_allocate(SIZE + 1);
  coroc_msg_t message;
  int i = 0, id;

  for (; i < SIZE; i++) {
    chans[i] = coroc_chan_allocate(sizeof(int), 0);
    thrds[i] = coroc_coroutine_allocate(sub_task, chans[i], "",
                                      TSC_COROUTINE_NORMAL, 
                                      TSC_DEFAULT_PRIO, NULL);
    coroc_chan_set_recv(set, chans[i], &id);
  }

  coroc_coroutine_allocate(sub_task2, me, "msg", 
                         TSC_COROUTINE_NORMAL, TSC_DEFAULT_PRIO, NULL);
  coroc_chan_set_recv(set, NULL, &message);

  for (i = 0; i <= SIZE; i++) {
    coroc_chan_t ch = NULL;
    coroc_chan_set_select(set, &ch);

    if (ch == me) {
      id = *(int*)(message.msg);
    }

    if (ch != NULL) {
      printf("[main task]: recv id is %d!\n", id);
    }
  }

  coroc_chan_set_dealloc(set);
  for (i = 0; i < SIZE; i++) coroc_chan_dealloc(chans[i]);

  coroc_coroutine_exit(0);
}
