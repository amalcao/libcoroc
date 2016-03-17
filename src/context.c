// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include "support.h"
#include "context.h"
#include "coroutine.h"

extern int __argc;
extern char** __argv;

typedef int (*main_entry_t)(int, char**);
typedef void (*boot_entry_t)();

static void bootstrap(uint32_t low, uint32_t high) {
  coroc_word_t tmp = high << 16;
  tmp <<= 16;
  tmp |= low;

  coroc_coroutine_t coroutine = (coroc_coroutine_t)tmp;
  if (coroutine->type == TSC_COROUTINE_MAIN)
    ((main_entry_t)(coroutine->entry))(__argc, __argv);
  else
    coroutine->entry(coroutine->arguments);
  coroc_coroutine_exit(0);
}

void TSC_CONTEXT_INIT(TSC_CONTEXT* ctx, void* stack, size_t stack_sz,
                      void* coroutine) {
  uint32_t low, high;
  uint64_t tmp = (coroc_word_t)(coroutine);
  sigset_t mask;

  low = tmp;
  tmp >>= 16;
  high = tmp >> 16;

  sigemptyset(&mask);

  memset(ctx, 0, sizeof(TSC_CONTEXT));
  TSC_CONTEXT_SAVE(ctx);

#ifdef ENABLE_SPLITSTACK
  (ctx->ctx).uc_stack.ss_sp = stack;
  (ctx->ctx).uc_stack.ss_size = stack_sz;
  (ctx->ctx).uc_sigmask = mask;
#else
  ctx->uc_stack.ss_sp = stack;
  ctx->uc_stack.ss_size = stack_sz;
  ctx->uc_sigmask = mask;
#endif

  TSC_CONTEXT_MAKE(ctx, (boot_entry_t)bootstrap, 2, low, high);
}
