// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>

#include "coroutine.h"

extern int user_main(void *);
extern int tsc_boot(int, char **, int, int, tsc_coroutine_handler_t);

#ifdef __APPLE__
#define __tsc_main main  // FIXME: weak alias not support by clang!!
#else
extern int main(int, char **) __attribute__((weak, alias("__tsc_main")));
#endif

int __tsc_main(int argc, char **argv) {
  return tsc_boot(argc, argv, 0, -1, (tsc_coroutine_handler_t)user_main);
}
