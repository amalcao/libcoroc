#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tsc_time.h"
#if defined(ENABLE_NOTIFY)
#include "notify.h"
#endif

#define TSC_DEFAULT_INTERTIMERS_CAP 32

TSC_SIGNAL_MASK_DECLARE

static void tsc_send_timer(void *arg) {
  tsc_timer_t timer = ((tsc_inter_timer_t *)arg)->owner;

  if (timer->timer.args) {
    typedef void (*func_t)(void);
    func_t f = (func_t)(timer->timer.args);
    f();  // FIXME : calling f() from a new thread !!
  }

  tsc_chan_send((tsc_chan_t)timer, &timer->timer.when);
}

static void inline __tsc_timer_init(tsc_timer_t t, uint32_t period,
                                    void (*func)(void)) {
  // init the buffered channel ..
  tsc_buffered_chan_init(&t->_chan, sizeof(uint64_t), 1, false);

  // init the internal timer ..
  t->timer.when = 0;
  t->timer.period = period;
  t->timer.func = tsc_send_timer;
  t->timer.args = (void *)func;
  t->timer.owner = t;
}

tsc_timer_t tsc_timer_allocate(uint32_t period, void (*func)(void)) {
  tsc_timer_t t = TSC_ALLOC(sizeof(struct tsc_timer));
  assert(t != NULL);
  __tsc_timer_init(t, period, func);
  return t;
}

void tsc_timer_dealloc(tsc_timer_t t) {
  tsc_timer_stop(t);
  TSC_DEALLOC(t);
}

tsc_chan_t tsc_timer_after(tsc_timer_t t, uint64_t after) {
  uint64_t curr = tsc_getmicrotime();
  return tsc_timer_at(t, curr + after);
}

tsc_chan_t tsc_timer_at(tsc_timer_t t, uint64_t when) {
  t->timer.when = when;
  tsc_timer_start(t);
  return (tsc_chan_t)t;
}

int tsc_timer_start(tsc_timer_t t) {
  if (t->timer.when <= tsc_getmicrotime()) {
    tsc_send_timer(&t->timer);
    return -1;
  }

  return tsc_add_intertimer(&t->timer);
}

int tsc_timer_stop(tsc_timer_t t) { return tsc_del_intertimer(&t->timer); }

int tsc_timer_reset(tsc_timer_t t, uint64_t when) {
  tsc_timer_stop(t);
  t->timer.when = when;
  return tsc_timer_start(t);
}

// ---------------------------------------------------
//

static struct {
  tsc_lock lock;
#if defined(ENABLE_NOTIFY)
  tsc_notify_t note;
#endif
  tsc_inter_timer_t **timers;
  
  bool suspend;

  int32_t cap;
  int32_t size;
  
  tsc_coroutine_t daemon;
} tsc_intertimer_manager;

void tsc_intertimer_initialize(void) {
  tsc_intertimer_manager.size = 0;
  tsc_intertimer_manager.cap = TSC_DEFAULT_INTERTIMERS_CAP;
  lock_init(&tsc_intertimer_manager.lock);
#if defined(ENABLE_NOTIFY)
  tsc_notify_clear(&tsc_intertimer_manager.note);
#endif
  tsc_intertimer_manager.timers =
      TSC_ALLOC(TSC_DEFAULT_INTERTIMERS_CAP * sizeof(void *));
  memset(tsc_intertimer_manager.timers, 0,
         TSC_DEFAULT_INTERTIMERS_CAP * sizeof(void *));
}

// exchange the two elements in the heap
static inline void __exchange_heap(tsc_inter_timer_t **timers, uint32_t e0,
                                   uint32_t e1) {
  tsc_inter_timer_t *tmp = timers[e0];
  timers[e0] = timers[e1];
  timers[e1] = tmp;
  timers[e0]->index = e0;
  timers[e1]->index = e1;
}

// adjust the heap using a top-down strategy,
// `cur' is index of the given top,
// and `size' is whole size of the heap array..
static void __down_adjust_heap(tsc_inter_timer_t **timers, uint32_t cur,
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
static void __up_adjust_heap(tsc_inter_timer_t **timers, uint32_t cur) {
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
static int tsc_intertimer_routine(void *unused) {
  // once this routine is started,
  // it will never stop until the whole system exits ..
  for (;;) {
    TSC_SIGNAL_MASK();

    lock_acquire(&tsc_intertimer_manager.lock);

    tsc_inter_timer_t **__timers = tsc_intertimer_manager.timers;
    int32_t __size = tsc_intertimer_manager.size;
    uint64_t now = tsc_getmicrotime();

    while (__size > 0) {
      tsc_inter_timer_t *t = __timers[0];

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

          tsc_intertimer_manager.size = __size;
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
      tsc_intertimer_manager.suspend = true;
      vpu_suspend((void *)(&tsc_intertimer_manager.lock),
                  (unlock_handler_t)(lock_release));
    } else {
      int64_t left = 1000 * (__timers[0]->when - now);
      lock_release(&tsc_intertimer_manager.lock);
#if defined(ENABLE_NOTIFY)
      tsc_notify_tsleep_u(&tsc_intertimer_manager.note, left);
      tsc_notify_clear(&tsc_intertimer_manager.note);
#else
      tsc_coroutine_yield();  // let others run ..
#endif
    }

    TSC_SIGNAL_UNMASK();
  }

  return 0;
}

// start the "timer" corouine when first add a timer.
static void tsc_intertimer_start(void) {
  assert(tsc_intertimer_manager.daemon == NULL);

  tsc_coroutine_t daemon = tsc_coroutine_allocate(
      tsc_intertimer_routine, NULL, "timer", TSC_COROUTINE_NORMAL, NULL);
  tsc_intertimer_manager.daemon = daemon;
}

int tsc_add_intertimer(tsc_inter_timer_t *timer) {
  TSC_SIGNAL_MASK();

  int ret = 0;
  lock_acquire(&tsc_intertimer_manager.lock);

  // realloc if need ..
  if (tsc_intertimer_manager.cap == tsc_intertimer_manager.size) {
    tsc_intertimer_manager.cap *= 2;
    void *p = tsc_intertimer_manager.timers;
    tsc_intertimer_manager.timers =
        TSC_REALLOC(p, tsc_intertimer_manager.cap * sizeof(void *));
  }

  tsc_inter_timer_t **__timers = tsc_intertimer_manager.timers;
  int32_t __size = tsc_intertimer_manager.size;

  __timers[__size] = timer;
  timer->index = __size;  // fast path for deletion
  __up_adjust_heap(__timers, __size);

  tsc_intertimer_manager.size++;

  if (tsc_intertimer_manager.daemon == NULL) {
    tsc_intertimer_start();
  } else if (__size == 0) {
    // awaken the timer daemon thread ..
    if (tsc_intertimer_manager.suspend) {
      tsc_intertimer_manager.suspend = false;
      vpu_ready(tsc_intertimer_manager.daemon);
    }
  }
#if defined(ENABLE_NOTIFY)
  else {
    tsc_notify_wakeup(&tsc_intertimer_manager.note);
  }
#endif
  lock_release(&tsc_intertimer_manager.lock);

  TSC_SIGNAL_UNMASK();
  return ret;
}

int tsc_del_intertimer(tsc_inter_timer_t *timer) {
  int ret = -1;

  lock_acquire(&tsc_intertimer_manager.lock);

  tsc_inter_timer_t **__timers = tsc_intertimer_manager.timers;
  int32_t __size = tsc_intertimer_manager.size;
  int32_t i = timer->index;

  if (i < 0 || i >= __size || timer != __timers[i]) goto __exit_del_intertimer;

  __timers[i] = __timers[--__size];
  __timers[i]->index = i;
  __down_adjust_heap(__timers, i, __size);
  tsc_intertimer_manager.size = __size;
  ret = 0;

__exit_del_intertimer:
  lock_release(&tsc_intertimer_manager.lock);

  return ret;
}

void tsc_udelay(uint64_t us) {
  // initialize a tsc_timer on stack !!
  struct tsc_timer timer;
  __tsc_timer_init(&timer, 0, 0);
  tsc_chan_recv(tsc_timer_after(&timer, us), NULL);
}
