#ifndef __LIBCOROC_H__
#define __LIBCOROC_H__

/* using the libtsc implementation as the runtime env */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <libtsc.h>

/* standard definations for CoroC internals */

/// basic builtin types
typedef tsc_chan_t      __chan_t;
typedef tsc_coroutine_t __task_t;
typedef tsc_chan_set_t  __select_set_t;
typedef unsigned long long  __time_t;

/// for reference-counting ops
typedef tsc_refcnt_t    __refcnt_t;
#define __refcnt_get(R) (typeof(R))tsc_refcnt_get((tsc_refcnt_t)(R))
#define __refcnt_put(R) tsc_refcnt_put((tsc_refcnt_t)(R))

/// for coroutine ops
typedef tsc_coroutine_handler_t __CoroC_spawn_handler_t;
#define __CoroC_Spawn(F, A) \
            tsc_coroutine_allocate((F), (A), "", TSC_COROUTINE_NORMAL, NULL)
#define __CoroC_Yield   tsc_coroutine_yield
#define __CoroC_Self    tsc_coroutine_self
#define __CoroC_Quit    tsc_coroutine_exit
#define __CoroC_Exit    tsc_coroutine_exit

/// for channel ops
#define __CoroC_Chan tsc_chan_allocate
#define __CoroC_Chan_Close(C)  tsc_chan_close(*(C))
#define __CoroC_Null       NULL

#define __CoroC_Chan_SendExpr(C, V) ({ \
    typeof(V) __temp = (V); \
    bool ret = tsc_chan_send(C, (void*)(&__temp)) != CHAN_CLOSED; \
    ret;})
#define __CoroC_Chan_Send(C, P) ({ \
    bool ret = tsc_chan_send(C, (void*)(P)) != CHAN_CLOSED; \
    ret;})
#define __CoroC_Chan_Recv(C, P) ({ \
    bool ret = tsc_chan_recv(C, (void*)(P)) != CHAN_CLOSED; \
    ret;})

#define __CoroC_Chan_SendExpr_NB(C, V) ({ \
    typeof(V) __temp = (V); \
    bool ret = tsc_chan_nbsend(C, (void*)(&__temp)) == CHAN_SUCCESS; \
    ret;})
#define __CoroC_Chan_Send_NB(C, P) ({ \
    bool ret = tsc_chan_nbsend(C, (void*)(P)) == CHAN_SUCCESS; \
    ret;})
#define __CoroC_Chan_Recv_NB(C, P) ({ \
    bool ret = tsc_chan_nbrecv(C, (void*)(P)) == CHAN_SUCCESS; \
    ret;})

///  channel select ops ..
#define __CoroC_Select_Alloc    tsc_chan_set_allocate
#define __CoroC_Select_Dealloc  tsc_chan_set_dealloc
#define __CoroC_Select(S, B) ({ \
    tsc_chan_t active = NULL;   \
    _tsc_chan_set_select(S, B, &active); \
    active; })
#define __CoroC_Select_SendExpr(S, C, E) \
    do { typeof(E) __temp = E; \
         tsc_chan_set_send(S, C, &__temp); } while (0)
#define __CoroC_Select_Send     tsc_chan_set_send
#define __CoroC_Select_Recv     tsc_chan_set_recv

/// for Task based send / recv
#define __CoroC_Task_Send(task, buf, sz) tsc_send(*(task), buf, sz)
#define __CoroC_Task_Recv(buf, sz) tsc_recv(buf, sz, 1)
#define __CoroC_Task_Recv_NB(buf, sz) tsc_recv(buf, sz, 0)

/// for Timer / Ticker ..
#define __CoroC_Timer_At(at) ({ \
    tsc_timer_t __timer = tsc_timer_allocate(0, NULL); \
    tsc_timer_at(__timer, at); \
    (tsc_chan_t)(__timer); })

#define __CoroC_Timer_After(after) ({ \
    tsc_timer_t __timer = tsc_timer_allocate(0, NULL); \
    tsc_timer_after(__timer, after); \
    (tsc_chan_t)(__timer); })

#define __CoroC_Ticker(period) ({ \
    tsc_timer_t __ticker = tsc_timer_allocate(period, NULL); \
    tsc_timer_after(__ticker, period);  \
    (tsc_chan_t)(__ticker); })

#define __CoroC_Stop(T)  {  \
    tsc_timer_stop((tsc_timer_t)(*(T)));   \
    tsc_timer_dealloc((tsc_timer_t)(*(T)));\
    (T).detach(); }

/// for time management
#define __CoroC_Now     tsc_getcurtime
#define __CoroC_Sleep   tsc_udelay

#endif // __LIBCOROC_H__
