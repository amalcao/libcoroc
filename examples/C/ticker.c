// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>

#include "libcoroc.h"

void callback(void) { printf("\ttime out!\n"); }

int main(int argc, char** argv) {
  uint64_t awaken = 0;
  int i = 0;
  coroc_timer_t timer = coroc_timer_allocate(1000000 * 2, callback);
  coroc_timer_after(timer, 1000000 * 2);  // 2 seconds later

  for (i = 0; i < 3; i++) {
    printf("waiting for 2 seconds!\n");
    coroc_chan_recv((coroc_chan_t)timer, &awaken);
    printf("awaken, time is %llu!\n", (long long unsigned)awaken);
  }

  printf("release the timer ..\n");
  coroc_timer_stop(timer);
  coroc_timer_dealloc(timer);

  coroc_coroutine_exit(0);
}
