// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

/* - - - -
 * The User APP using LibTSC ..
 * - - - - */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "libtsc.h"

void sub_task(void* arg) {
  uint64_t id = (uint64_t)arg;

  printf("[sub_task:] id is %i!\n", id);

  for (;;) {
#ifndef ENABLE_TIMESHARE
    tsc_coroutine_yield();
#endif
  }

  tsc_coroutine_exit(0);
}

int main(int argc, char** argv) {
  tsc_coroutine_t threads[100];

  int i;
  for (i = 0; i < 100; ++i) {
    threads[i] =
        tsc_coroutine_allocate(sub_task, i, "", TSC_COROUTINE_NORMAL, 0);
  }

  for (;;) {
#ifndef ENABLE_TIMESHARE
    tsc_coroutine_yield();
#endif
  }

  tsc_coroutine_exit(0);

  return 0;
}
