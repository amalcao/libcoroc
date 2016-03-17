// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_CORE_ASYNC_POOL_H_
#define _TSC_CORE_ASYNC_POOL_H_

#include <stdlib.h>
#include <stdbool.h>

#include "support.h"
#include "coroutine.h"

typedef void (*coroc_async_callback_t)(void *);

// the asynchronize request type ..
typedef struct {
  queue_item_t link;
  coroc_coroutine_t wait;
  coroc_async_callback_t func;
  void *argument;
} coroc_async_request_t;

// the API for the coroutines ..
void coroc_async_request_submit(coroc_async_callback_t func, void *argument);

// the API for the vpus or framework ..
coroc_coroutine_t coroc_async_pool_fetch(void);
void coroc_async_pool_initialize(int);
bool coroc_async_pool_working(void);

#endif  // _TSC_CORE_ASYNC_POOL_H_
