// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>

#include "libtsc.h"

void callback(void) { printf("\ttime out!\n"); }

int main(int argc, char** argv) {
  uint64_t awaken = 0;
  int i = 0;
  tsc_timer_t timer = tsc_timer_allocate(1000000 * 2, callback);
  tsc_timer_after(timer, 1000000 * 2);  // 2 seconds later

  for (i = 0; i < 3; i++) {
    printf("waiting for 2 seconds!\n");
    tsc_chan_recv((tsc_chan_t)timer, &awaken);
    printf("awaken, time is %llu!\n", (long long unsigned)awaken);
  }

  printf("release the timer ..\n");
  tsc_timer_stop(timer);
  tsc_timer_dealloc(timer);

  tsc_coroutine_exit(0);
}
