// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdlib.h>

#include "coroutine.h"

extern int user_main(void *);
extern int coroc_boot(int, char **, int, int, coroc_coroutine_handler_t);

#ifdef __APPLE__
#define __coroc_main main  // FIXME: weak alias not support by clang!!
#else
extern int main(int, char **) __attribute__((weak, alias("__coroc_main")));
#endif

int __coroc_main(int argc, char **argv) {
  return coroc_boot(argc, argv, 0, -1, (coroc_coroutine_handler_t)user_main);
}
