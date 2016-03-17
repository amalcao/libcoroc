// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <assert.h>
#include <pthread.h>

#include "async.h"
#include "vpu.h"
#include "netpoll.h"

// the asynchronized thread pool manager ..
struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  queue_t wait_que;
  TSC_OS_THREAD_T *threads;
} coroc_async_pool_manager;

// this is the `core' part !
TSC_SIGNAL_MASK_DECLARE

// the routine loop of each `async thread'
static void *coroc_async_thread_routine(void *unused) {
  pthread_mutex_lock(&coroc_async_pool_manager.mutex);
  for (;;) {
    while (coroc_async_pool_manager.wait_que.status == 0) {
      // prepare to waitting for next job or timeout ..
      pthread_cond_wait(&coroc_async_pool_manager.cond,
                        &coroc_async_pool_manager.mutex);
    }
    // get a request from the waiting queue
    coroc_async_request_t *req = queue_rem(&coroc_async_pool_manager.wait_que);
    assert(req != 0);

    pthread_mutex_unlock(&coroc_async_pool_manager.mutex);

    // do the work of this request,
    // NOTE that the call will block current thread ..
    req->func(req->argument);

    // after the callback done, add the wait coroutine
    // to the ready queue..
    vpu_ready(req->wait, false);

    pthread_mutex_lock(&coroc_async_pool_manager.mutex);
  }
  // never return here maybe ..
  pthread_exit(0);
}

// initialize the asynchronize threads' pool with
// `n' threads, n must be a positive number !!
void coroc_async_pool_initialize(int n) {
  // NOTE that the async thread number must not be less than 1!
  // assert(n > 0);

  int i;

  // init the async thread pool manager ..
  pthread_mutex_init(&coroc_async_pool_manager.mutex, NULL);
  
  if (n == 0) {  // ignore the async threads pool
    coroc_async_pool_manager.threads = NULL;
    return ;
  }

  pthread_cond_init(&coroc_async_pool_manager.cond, NULL);
  // the wait queue will be protected by the manager's mutex!
  queue_init(&coroc_async_pool_manager.wait_que);
  // alloc the thread pool ..
  coroc_async_pool_manager.threads = TSC_ALLOC(n * sizeof(TSC_OS_THREAD_T));

  // start n asynchronize working threads ..
  for (i = 0; i < n; ++i) {
    TSC_OS_THREAD_CREATE(&coroc_async_pool_manager.threads[i], NULL,
                         coroc_async_thread_routine, (void *)(coroc_word_t)i);
  }

  return;
}

// init and submit the async request,
// NOTE the req must be allocated on the calling stack
// of the current coroutine !!
void coroc_async_request_submit(coroc_async_callback_t func, void *argument) {
  coroc_async_request_t req;

  TSC_SIGNAL_MASK();
  
  if (coroc_async_pool_manager.threads == NULL) {
    func(argument);
  } else {
    // init ..
    req.wait = coroc_coroutine_self();
    req.func = func;
    req.argument = argument;
    queue_item_init(&req.link, &req);

    // submit this request to the async pool ..
    pthread_mutex_lock(&coroc_async_pool_manager.mutex);

    // add req to the wait queue
    queue_add(&coroc_async_pool_manager.wait_que, &req.link);

    pthread_cond_signal(&coroc_async_pool_manager.cond);

    // suspend current coroutine and release the mutex ..
    req.wait->async_wait = 1;
    vpu_suspend(&coroc_async_pool_manager.mutex,
                (unlock_handler_t)pthread_mutex_unlock);
  }

  // return from the block func ..
  TSC_SIGNAL_UNMASK();
  return;
}
