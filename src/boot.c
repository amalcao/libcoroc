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
#include "coroc_clock.h"

extern void coroc_vpu_initialize(int, coroc_coroutine_handler_t);
extern void coroc_clock_initialize(void);
extern void coroc_intertimer_initialize(void);
extern void coroc_async_pool_initialize(int);
extern void coroc_netpoll_initialize(void);
extern void coroc_profiler_initialize(int);

int __argc;
char **__argv;

static bool __coroc_env2int(const char *env, int *ret) {
  const char *value = getenv(env);
  char *endp = NULL;

  if (value == NULL) return false;

  errno = 0;
  *ret = strtol(value, &endp, 0);
  return (errno == 0);
}

int coroc_boot(int argc, char **argv, int np, int nasync,
             coroc_coroutine_handler_t entry) {
  int profile = 0;

  __argc = argc;
  __argv = argv;

  if (np <= 0) {
    __coroc_env2int("TSC_NP", &np);
    if (np <= 0) np = TSC_NP_ONLINE();
  }

  if (nasync <= 0 && !__coroc_env2int("TSC_ASYNC", &nasync)) {
    nasync = (np > 1) ? (np >> 1) : 1;
  }

  __coroc_env2int("TSC_PROFILE", &profile);

  coroc_clock_initialize();
  coroc_intertimer_initialize();
  coroc_netpoll_initialize();
  coroc_async_pool_initialize(nasync);
  coroc_vpu_initialize(np, entry);
  coroc_profiler_initialize(profile);
  // TODO : more modules later .. --

  clock_routine();  // never return ..

  return 0;
}
