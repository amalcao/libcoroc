#ifndef _TSC_CORE_ASYNC_POOL_H_
#define _TSC_CORE_ASYNC_POOL_H_

#include <stdlib.h>
#include <stdbool.h>

#include "support.h"
#include "coroutine.h"

typedef void (*tsc_async_callback_t)(void *);

// the asynchronize request type ..
typedef struct {
  queue_item_t link;
  tsc_coroutine_t wait;
  tsc_async_callback_t func;
  void *argument;
} tsc_async_request_t;

// the API for the coroutines ..
void tsc_async_request_submit(tsc_async_callback_t func, void *argument);

// the API for the vpus or framework ..
tsc_coroutine_t tsc_async_pool_fetch(void);
void tsc_async_pool_initialize(int);
bool tsc_async_pool_working(void);

#endif  // _TSC_CORE_ASYNC_POOL_H_
