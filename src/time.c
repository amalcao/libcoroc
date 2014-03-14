#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "time.h"

#define TSC_DEFAULT_INTERTIMERS_CAP 32

TSC_SIGNAL_MASK_DECLARE

static void tsc_send_timer (void * arg)
{
    tsc_timer_t timer = (tsc_timer_t)arg;

    if (timer->timer.args) {
        typedef void (*func_t)(void);
        func_t f = (func_t)(timer->timer.args);
        f (); // FIXME : calling f() from a new thread !!
    }

    tsc_chan_send(timer->chan, & timer->timer.when);
}

static uint64_t tsc_getcurtime (void)
{
   struct timeval tv;
   struct timezone tz;

   gettimeofday (& tv, & tz);
   
   return tv.tv_sec * 1000000 + tv.tv_usec;
}

tsc_timer_t tsc_timer_allocate (uint32_t period, void (*func)(void))
{
    tsc_timer_t t = TSC_ALLOC(sizeof(struct tsc_timer));
    assert (t != NULL);

    t->timer.when = 0;
    t->timer.period = period;
    t->timer.func = tsc_send_timer;
    t->timer.args = (void*)func;

    t->chan  = tsc_chan_allocate (sizeof(uint64_t), 1);

    return t;
}

void tsc_timer_dealloc (tsc_timer_t t)
{
    tsc_timer_stop (t);
    tsc_chan_dealloc (t->chan);
    
    TSC_DEALLOC(t);
}

tsc_chan_t tsc_timer_after (tsc_timer_t t, uint64_t after)
{
    uint64_t curr = tsc_getcurtime();
    return tsc_timer_at (t, curr+after);
}

tsc_chan_t tsc_timer_at (tsc_timer_t t, uint64_t when)
{
    t->timer.when = when;
    tsc_timer_start (t);
    return t->chan;
}

int tsc_timer_start (tsc_timer_t t)
{
    if (t->timer.when <= tsc_getcurtime()) {
        tsc_send_timer (t); 
        return -1;
    }

    return tsc_add_intertimer (& t->timer);
}

int tsc_timer_stop (tsc_timer_t t)
{
    return tsc_del_intertimer (& t->timer);
}

int tsc_timer_reset (tsc_timer_t t, uint64_t when)
{
    tsc_timer_stop (t);
    t->timer.when = when;
    return tsc_timer_start (t);
}


// ---------------------------------------------------
//

static struct {
    lock lock;
    tsc_inter_timer_t **timers;
    uint32_t cap;
    uint32_t size;
    tsc_coroutine_t daemon;
} tsc_intertimer_manager;


void tsc_intertimer_initialize (void)
{
    tsc_intertimer_manager . size = 0;
    tsc_intertimer_manager . cap = TSC_DEFAULT_INTERTIMERS_CAP;
    lock_init (& tsc_intertimer_manager . lock); 
    tsc_intertimer_manager . timers = 
        TSC_ALLOC (TSC_DEFAULT_INTERTIMERS_CAP * sizeof(void*));
    memset (tsc_intertimer_manager . timers, 0, 
        TSC_DEFAULT_INTERTIMERS_CAP * sizeof(void*));
}

// adjust the heap using a top-down strategy,
// `cur' is index of the given top, 
// and `size' is whole size of the heap array..
static void __down_adjust_heap (tsc_inter_timer_t **timers, uint32_t cur, uint32_t size)
{
    uint32_t left, right, tmp;
    left = (cur << 1) + 1;
    right = left + 1;

    if (left >= size)
        return ;

    if (right >= size) 
        tmp = left;
    else
        tmp = (timers[left]->when < timers[right]->when) ? left : right;

    if (timers[tmp]->when < timers[cur]->when) {
        tsc_inter_timer_t *t = timers[cur];
        timers[cur] = timers[tmp];
        timers[tmp] = t;

        // adjust the child ..
        __down_adjust_heap (timers, tmp, size);
    }
}

// adjust the heap using down-up strategy,
// where the `cur' is the index to handle,
// no other argument is needed since the concurrent call
// will stop when reach the top index, 0.
static void __up_adjust_heap (tsc_inter_timer_t **timers, uint32_t cur)
{
    if (cur == 0)
        return;

    uint32_t parent = (cur - 1) >> 1;

    if (timers[cur]->when < timers[parent]->when) {
        tsc_inter_timer_t *t = timers[cur];
        timers[cur] = timers[parent];
        timers[parent] = t;

        // adjust parent ..
        __up_adjust_heap (timers, parent);
    }
}

// a special corouine will running this,
// geting a timer which is ready and triger its callback..
static int tsc_intertimer_routine (void *unused)
{
    // once this routine is started,
    // it will never stop until the whole system exits ..
    for (;;) {
        TSC_SIGNAL_MASK();

        lock_acquire (& tsc_intertimer_manager . lock);
        
        tsc_inter_timer_t **__timers = tsc_intertimer_manager . timers;
        uint32_t __size = tsc_intertimer_manager . size;

        while (__size > 0) {
            uint64_t now = tsc_getcurtime();
            tsc_inter_timer_t *t = __timers[0];
           
            if (t->when <= now) {
                // time out !!
                (t->func) ((void*)t);
                if (t->period == 0) {
                    // del the timer ..
                    if (--__size > 0)
                        __timers[0] = __timers[__size];
                    
                    tsc_intertimer_manager . size = __size;
                } else {
                    // update the `when` field and adjust again ..
                    t->when = now + t->period;    
                }

                if (__size > 0)
                    __down_adjust_heap (__timers, 0, __size);
            } else {
                break;
            }
        }

        // no alive timers here, just suspend myself ..
        if (__size == 0) {
            vpu_suspend (NULL, (void*)(& tsc_intertimer_manager . lock), 
                        (unlock_hander_t)(lock_release));
        } else {
            lock_release (& tsc_intertimer_manager . lock);
            tsc_coroutine_yield (); // let others run ..
        }

        TSC_SIGNAL_UNMASK();
    }

    return 0;
}

// start the "timer" corouine when first add a timer.
static void tsc_intertimer_start (void)
{
    assert (tsc_intertimer_manager . daemon == NULL);

    tsc_coroutine_t daemon = tsc_coroutine_allocate (
                        tsc_intertimer_routine, 
                        NULL, "timer", 
                        TSC_COROUTINE_NORMAL, NULL);
    tsc_intertimer_manager . daemon = daemon;
}

int tsc_add_intertimer (tsc_inter_timer_t *timer)
{
    TSC_SIGNAL_MASK();

    int ret = 0;
    lock_acquire (& tsc_intertimer_manager . lock);
    
    // realloc if need ..
    if (tsc_intertimer_manager . cap == tsc_intertimer_manager . size) {
        tsc_intertimer_manager . cap *= 2;
        void *p = tsc_intertimer_manager . timers;
        tsc_intertimer_manager . timers = 
            TSC_REALLOC(p, tsc_intertimer_manager . cap * sizeof (void*)); 
    }
    
    tsc_inter_timer_t **__timers = tsc_intertimer_manager . timers;
    uint32_t __size = tsc_intertimer_manager . size;

    __timers[__size] = timer;
    __up_adjust_heap (__timers, __size);

    tsc_intertimer_manager . size ++;
    
    if (tsc_intertimer_manager . daemon == NULL) {
        tsc_intertimer_start ();
    } else if (__size == 0) {
    // awaken the timer daemon thread ..
        vpu_ready (tsc_intertimer_manager . daemon);
    }

    lock_release (& tsc_intertimer_manager . lock);
    
    TSC_SIGNAL_UNMASK();
    return ret;
}

int tsc_del_intertimer (tsc_inter_timer_t *timer)
{
    int ret = 1;

    lock_acquire (& tsc_intertimer_manager . lock);

    tsc_inter_timer_t **__timers = tsc_intertimer_manager . timers;
    uint32_t __size = tsc_intertimer_manager . size;
    uint32_t i;

    for (i = 0; i < __size; i++) {
        if (__timers[i] == timer) {
            __timers[i] = __timers[--__size];
            __down_adjust_heap (__timers, i, __size);
            tsc_intertimer_manager . size = __size;
            ret = 0;
            break;
        }
    }

    lock_release (& tsc_intertimer_manager . lock);

    return ret;
}

void tsc_udelay (uint64_t us)
{
    // TODO : provide static declare way to 
    // initialize a tsc_timer on stack !!
    tsc_timer_t timer = tsc_timer_allocate(0, 0);
    tsc_chan_recv(tsc_timer_after(timer, us), NULL);
    tsc_timer_dealloc(timer);
}
