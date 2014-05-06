#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include "vpu.h"
#include "clock.h"
#include "coroutine.h"

extern void tsc_vpu_initialize(int, tsc_coroutine_handler_t);
extern void tsc_clock_initialize(void);
extern void tsc_intertimer_initialize(void);
extern void tsc_vfs_initialize(int);
extern void tsc_netpoll_initialize(void);

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

int tsc_boot(int argc, char **argv, int np, int nvfs,
             tsc_coroutine_handler_t entry) {
  __argc = argc;
  __argv = argv;

  if (np <= 0) {
    __tsc_env2int("TSC_NP", &np);
    if (np <= 0) np = TSC_NP_ONLINE();
  }

  if (nvfs < 0 && !__tsc_env2int("TSC_VFS", &nvfs)) {
    nvfs = (np > 1) ? (np >> 1) : 1;
  }

  tsc_clock_initialize();
  tsc_intertimer_initialize();
  tsc_vfs_initialize(nvfs);
  tsc_netpoll_initialize();
  tsc_vpu_initialize(np, entry);
  // TODO : more modules later .. --

  clock_routine();  // never return ..

  return 0;
}
