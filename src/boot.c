// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include "vpu.h"
#include "coroutine.h"
#include "tsc_clock.h"

extern void tsc_vpu_initialize(int, tsc_coroutine_handler_t);
extern void tsc_clock_initialize(void);
extern void tsc_intertimer_initialize(void);
extern void tsc_async_pool_initialize(int);
extern void tsc_netpoll_initialize(void);
extern void tsc_profiler_initialize(int);

int __argc;
char **__argv;

static bool __tsc_env2int(const char *env, int *ret) {
  const char *value = getenv(env);
  char *endp = NULL;

  if (value == NULL) return false;

  errno = 0;
  *ret = strtol(value, &endp, 0);
  return (errno == 0);
}

int tsc_boot(int argc, char **argv, int np, int nasync,
             tsc_coroutine_handler_t entry) {
  int profile = 0;

  __argc = argc;
  __argv = argv;

  if (np <= 0) {
    __tsc_env2int("TSC_NP", &np);
    if (np <= 0) np = TSC_NP_ONLINE();
  }

  if (nasync <= 0 && !__tsc_env2int("TSC_ASYNC", &nasync)) {
    nasync = (np > 1) ? (np >> 1) : 1;
  }

  __tsc_env2int("TSC_PROFILE", &profile);

  tsc_clock_initialize();
  tsc_intertimer_initialize();
  tsc_netpoll_initialize();
  tsc_async_pool_initialize(nasync);
  tsc_vpu_initialize(np, entry);
  tsc_profiler_initialize(profile);
  // TODO : more modules later .. --

  clock_routine();  // never return ..

  return 0;
}
