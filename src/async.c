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
} tsc_async_pool_manager;

// this is the `core' part !
TSC_SIGNAL_MASK_DECLARE

// the routine loop of each `async thread'
static void *tsc_async_thread_routine(void *unused) {
  pthread_mutex_lock(&tsc_async_pool_manager.mutex);
  for (;;) {
    while (tsc_async_pool_manager.wait_que.status == 0) {
      // prepare to waitting for next job or timeout ..
      pthread_cond_wait(&tsc_async_pool_manager.cond,
                        &tsc_async_pool_manager.mutex);
    }
    // get a request from the waiting queue
    tsc_async_request_t *req = queue_rem(&tsc_async_pool_manager.wait_que);
    assert(req != 0);

    pthread_mutex_unlock(&tsc_async_pool_manager.mutex);

    // do the work of this request,
    // NOTE that the call will block current thread ..
    req->func(req->argument);

    // after the callback done, add the wait coroutine
    // to the ready queue..
    vpu_ready(req->wait, false);

    pthread_mutex_lock(&tsc_async_pool_manager.mutex);
  }
  // never return here maybe ..
  pthread_exit(0);
}

// initialize the asynchronize threads' pool with
// `n' threads, n must be a positive number !!
void tsc_async_pool_initialize(int n) {
  // NOTE that the async thread number must not be less than 1!
  // assert(n > 0);

  int i;

  // init the async thread pool manager ..
  pthread_mutex_init(&tsc_async_pool_manager.mutex, NULL);
  
  if (n == 0) {  // ignore the async threads pool
    tsc_async_pool_manager.threads = NULL;
    return ;
  }

  pthread_cond_init(&tsc_async_pool_manager.cond, NULL);
  // the wait queue will be protected by the manager's mutex!
  queue_init(&tsc_async_pool_manager.wait_que);
  // alloc the thread pool ..
  tsc_async_pool_manager.threads = TSC_ALLOC(n * sizeof(TSC_OS_THREAD_T));

  // start n asynchronize working threads ..
  for (i = 0; i < n; ++i) {
    TSC_OS_THREAD_CREATE(&tsc_async_pool_manager.threads[i], NULL,
                         tsc_async_thread_routine, (void *)(tsc_word_t)i);
  }

  return;
}

// init and submit the async request,
// NOTE the req must be allocated on the calling stack
// of the current coroutine !!
void tsc_async_request_submit(tsc_async_callback_t func, void *argument) {
  tsc_async_request_t req;

  TSC_SIGNAL_MASK();
  
  if (tsc_async_pool_manager.threads == NULL) {
    func(argument);
  } else {
    // init ..
    req.wait = tsc_coroutine_self();
    req.func = func;
    req.argument = argument;
    queue_item_init(&req.link, &req);

    // submit this request to the async pool ..
    pthread_mutex_lock(&tsc_async_pool_manager.mutex);

    // add req to the wait queue
    queue_add(&tsc_async_pool_manager.wait_que, &req.link);

    pthread_cond_signal(&tsc_async_pool_manager.cond);

    // suspend current coroutine and release the mutex ..
    req.wait->async_wait = 1;
    vpu_suspend(&tsc_async_pool_manager.mutex,
                (unlock_handler_t)pthread_mutex_unlock);
  }

  // return from the block func ..
  TSC_SIGNAL_UNMASK();
  return;
}
