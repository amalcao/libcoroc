#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "vpu.h"
#include "clock.h"
#include "coroutine.h"

extern void tsc_vpu_initialize(int, tsc_coroutine_handler_t);
extern void tsc_clock_initialize(void);
extern void tsc_intertimer_initialize(void);
extern void tsc_vfs_initialize(void);
extern void tsc_netpoll_initialize(void);

int __argc;
char **__argv;

int tsc_boot(int argc, char **argv, int np, tsc_coroutine_handler_t entry) {
  __argc = argc;
  __argv = argv;

  if (np <= 0) {
    const char *env = getenv("TSC_NP");
    char *endp = NULL;

    if (env != NULL) np = strtol(env, &endp, 0);
    if (np <= 0 || endp == env) np = TSC_NP_ONLINE();
  }

  tsc_clock_initialize();
  tsc_intertimer_initialize();
  tsc_vfs_initialize();
  tsc_netpoll_initialize();
  tsc_vpu_initialize(np, entry);
  // TODO : more modules later .. --

  clock_routine();  // never return ..

  return 0;
}
