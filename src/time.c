// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "coroc_time.h"
#if defined(ENABLE_NOTIFY)
#include "notify.h"
#endif

#define TSC_DEFAULT_INTERTIMERS_CAP 32

TSC_SIGNAL_MASK_DECLARE

static void coroc_send_timer(void *arg) {
  coroc_timer_t timer = ((coroc_inter_timer_t *)arg)->owner;

  if (timer->timer.args) {
    typedef void (*func_t)(void);
    func_t f = (func_t)(timer->timer.args);
    f();  // FIXME : calling f() from a new thread !!
  }

  coroc_chan_send((coroc_chan_t)timer, &timer->timer.when);
}

static void inline __coroc_timer_init(coroc_timer_t t, uint32_t period,
                                    void (*func)(void)) {
  // init the buffered channel ..
  coroc_buffered_chan_init(&t->_chan, sizeof(uint64_t), 1, false);

  // re-init the refcnt's release handler ..
  coroc_refcnt_init(&(t->_chan._chan.refcnt), 
                  (release_handler_t)(coroc_timer_dealloc));

  // init the internal timer ..
  t->timer.when = 0;
  t->timer.period = period;
  t->timer.func = coroc_send_timer;
  t->timer.args = (void *)func;
  t->timer.owner = t;
}

coroc_timer_t coroc_timer_allocate(uint32_t period, void (*func)(void)) {
  coroc_timer_t t = TSC_ALLOC(sizeof(struct coroc_timer));
  assert(t != NULL);
  __coroc_timer_init(t, period, func);
  return t;
}

void coroc_timer_dealloc(coroc_timer_t t) {
  coroc_timer_stop(t);
  TSC_DEALLOC(t);
}

coroc_chan_t coroc_timer_after(coroc_timer_t t, uint64_t after) {
  uint64_t curr = coroc_getmicrotime();
  return coroc_timer_at(t, curr + after);
}

coroc_chan_t coroc_timer_at(coroc_timer_t t, uint64_t when) {
  t->timer.when = when;
  coroc_timer_start(t);
  return (coroc_chan_t)t;
}

int coroc_timer_start(coroc_timer_t t) {
  if (t->timer.when <= coroc_getmicrotime()) {
    coroc_send_timer(&t->timer);
    return -1;
  }

  return coroc_add_intertimer(&t->timer);
}

int coroc_timer_stop(coroc_timer_t t) { return coroc_del_intertimer(&t->timer); }

int coroc_timer_reset(coroc_timer_t t, uint64_t when) {
  coroc_timer_stop(t);
  t->timer.when = when;
  return coroc_timer_start(t);
}

// ---------------------------------------------------
//

static struct {
  coroc_lock lock;
#if defined(ENABLE_NOTIFY)
  coroc_notify_t note;
#endif
  coroc_inter_timer_t **timers;
  
  bool suspend;

  int32_t cap;
  int32_t size;
  
  coroc_coroutine_t daemon;
} coroc_intertimer_manager;

void coroc_intertimer_initialize(void) {
  coroc_intertimer_manager.size = 0;
  coroc_intertimer_manager.cap = TSC_DEFAULT_INTERTIMERS_CAP;
  lock_init(&coroc_intertimer_manager.lock);
#if defined(ENABLE_NOTIFY)
  coroc_notify_clear(&coroc_intertimer_manager.note);
#endif
  coroc_intertimer_manager.timers =
      TSC_ALLOC(TSC_DEFAULT_INTERTIMERS_CAP * sizeof(void *));
  memset(coroc_intertimer_manager.timers, 0,
         TSC_DEFAULT_INTERTIMERS_CAP * sizeof(void *));
}

// exchange the two elements in the heap
static inline void __exchange_heap(coroc_inter_timer_t **timers, uint32_t e0,
                                   uint32_t e1) {
  coroc_inter_timer_t *tmp = timers[e0];
  timers[e0] = timers[e1];
  timers[e1] = tmp;
  timers[e0]->index = e0;
  timers[e1]->index = e1;
}

// adjust the heap using a top-down strategy,
// `cur' is index of the given top,
// and `size' is whole size of the heap array..
static void __down_adjust_heap(coroc_inter_timer_t **timers, uint32_t cur,
                               uint32_t size) {
  uint32_t left, right, tmp;
  left = (cur << 1) + 1;
  right = left + 1;

  if (left >= size) return;

  if (right >= size)
    tmp = left;
  else
    tmp = (timers[left]->when < timers[right]->when) ? left : right;

  if (timers[tmp]->when < timers[cur]->when) {
    __exchange_heap(timers, cur, tmp);
    // adjust the child ..
    __down_adjust_heap(timers, tmp, size);
  }
}

// adjust the heap using down-up strategy,
// where the `cur' is the index to handle,
// no other argument is needed since the concurrent call
// will stop when reach the top index, 0.
static void __up_adjust_heap(coroc_inter_timer_t **timers, uint32_t cur) {
  if (cur == 0) return;

  uint32_t parent = (cur - 1) >> 1;

  if (timers[cur]->when < timers[parent]->when) {
    __exchange_heap(timers, cur, parent);
    // adjust parent ..
    __up_adjust_heap(timers, parent);
  }
}

// a special corouine will running this,
// geting a timer which is ready and triger its callback..
static int coroc_intertimer_routine(void *unused) {
  // once this routine is started,
  // it will never stop until the whole system exits ..
  for (;;) {
    TSC_SIGNAL_MASK();

    lock_acquire(&coroc_intertimer_manager.lock);

    coroc_inter_timer_t **__timers = coroc_intertimer_manager.timers;
    int32_t __size = coroc_intertimer_manager.size;
    uint64_t now = coroc_getmicrotime();

    while (__size > 0) {
      coroc_inter_timer_t *t = __timers[0];

      if (t->when <= now) {
        // time out !!
        (t->func)((void *)t);
        if (t->period == 0) {
          // del the timer ..
          __timers[0]->index = -1;
          if (--__size > 0) {
            __timers[0] = __timers[__size];
            __timers[0]->index = 0;
          }

          coroc_intertimer_manager.size = __size;
        } else {
          // update the `when` field and adjust again ..
          t->when = now + t->period;
        }

        if (__size > 0) __down_adjust_heap(__timers, 0, __size);
      } else {
        break;
      }
    }

    // no alive timers here, just suspend myself ..
    if (__size == 0) {
      coroc_intertimer_manager.suspend = true;
      vpu_suspend((void *)(&coroc_intertimer_manager.lock),
                  (unlock_handler_t)(lock_release));
    } else {
      int64_t left = 1000 * (__timers[0]->when - now);
      lock_release(&coroc_intertimer_manager.lock);
#if defined(ENABLE_NOTIFY)
      coroc_notify_tsleep_u(&coroc_intertimer_manager.note, left);
      coroc_notify_clear(&coroc_intertimer_manager.note);
#else
      coroc_coroutine_yield();  // let others run ..
#endif
    }

    TSC_SIGNAL_UNMASK();
  }

  return 0;
}

// start the "timer" corouine when first add a timer.
static void coroc_intertimer_start(void) {
  assert(coroc_intertimer_manager.daemon == NULL);

  coroc_coroutine_t daemon = coroc_coroutine_allocate(
      coroc_intertimer_routine, NULL, "timer", 
      TSC_COROUTINE_NORMAL, TSC_PRIO_HIGH, NULL);
  coroc_intertimer_manager.daemon = daemon;
}

int coroc_add_intertimer(coroc_inter_timer_t *timer) {
  TSC_SIGNAL_MASK();

  int ret = 0;
  lock_acquire(&coroc_intertimer_manager.lock);

  // realloc if need ..
  if (coroc_intertimer_manager.cap == coroc_intertimer_manager.size) {
    coroc_intertimer_manager.cap *= 2;
    void *p = coroc_intertimer_manager.timers;
    coroc_intertimer_manager.timers =
        TSC_REALLOC(p, coroc_intertimer_manager.cap * sizeof(void *));
  }

  coroc_inter_timer_t **__timers = coroc_intertimer_manager.timers;
  int32_t __size = coroc_intertimer_manager.size;

  __timers[__size] = timer;
  timer->index = __size;  // fast path for deletion
  __up_adjust_heap(__timers, __size);

  coroc_intertimer_manager.size++;

  if (coroc_intertimer_manager.daemon == NULL) {
    coroc_intertimer_start();
  } else if (__size == 0) {
    // awaken the timer daemon thread ..
    if (coroc_intertimer_manager.suspend) {
      coroc_intertimer_manager.suspend = false;
      lock_release(&coroc_intertimer_manager.lock);
      // NOTE: must release the lock before calling `vpu_ready()' !!
      vpu_ready(coroc_intertimer_manager.daemon, true);
      goto __exit;
    }
  }
#if defined(ENABLE_NOTIFY)
  else {
    coroc_notify_wakeup(&coroc_intertimer_manager.note);
  }
#endif
  lock_release(&coroc_intertimer_manager.lock);

__exit:
  TSC_SIGNAL_UNMASK();
  return ret;
}

int coroc_del_intertimer(coroc_inter_timer_t *timer) {
  int ret = -1;

  lock_acquire(&coroc_intertimer_manager.lock);

  coroc_inter_timer_t **__timers = coroc_intertimer_manager.timers;
  int32_t __size = coroc_intertimer_manager.size;
  int32_t i = timer->index;

  if (i < 0 || i >= __size || timer != __timers[i]) goto __exit_del_intertimer;

  __timers[i] = __timers[--__size];
  __timers[i]->index = i;
  __down_adjust_heap(__timers, i, __size);
  coroc_intertimer_manager.size = __size;
  ret = 0;

__exit_del_intertimer:
  lock_release(&coroc_intertimer_manager.lock);

  return ret;
}

void coroc_udelay(uint64_t us) {
  // initialize a coroc_timer on stack !!
  struct coroc_timer timer;
  __coroc_timer_init(&timer, 0, 0);
  coroc_chan_recv(coroc_timer_after(&timer, us), NULL);
}
