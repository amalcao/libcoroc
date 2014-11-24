#ifndef __LIBCOROC_H__
#define __LIBCOROC_H__

#if defined(__COROC__)

/* using the libtsc implementation as the runtime env */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <libtsc.h>

#else // defined(__COROC__)

/* standard definations for CoroC internals */

/* NOTE: These macros must be designed carefully to avoid 
 *       include other macros which defined in other header files! */

/// basic builtin types

#if !defined(NULL)
# define NULL ((void*)0)
#endif

#define __chan_t        tsc_chan_t
#define __task_t        tsc_coroutine_t
#define __select_set_t  tsc_chan_set_t
#define __coroc_time_t  uint64_t
#define __group_t       tsc_group_t

/// for reference-counting ops
#define __refcnt_t      tsc_refcnt_t
#define __refcnt_get(R) (typeof(R))tsc_refcnt_get((tsc_refcnt_t)(R))
#define __refcnt_put(R) tsc_refcnt_put((tsc_refcnt_t)(R))

/* more interfaces for new auto scope branch */
#define __refcnt_assign(D, S) ({ \
            tsc_refcnt_put((tsc_refcnt_t)(D)); \
            D = (typeof(D))tsc_refcnt_get((tsc_refcnt_t)(S)); \
            D; })

#define __refcnt_assign_expr(D, E) ({ \
            tsc_refcnt_put((tsc_refcnt_t)(D)); \
            D = (typeof(D))(E); D; })

#define __refcnt_put_array(A, N) ({ \
            int i = 0;  \
            for (i = 0; i < (N); ++i) \
              tsc_refcnt_put((tsc_refcnt_t)((A)[i])); })

/// for coroutine ops
#define __CoroC_spawn_handler_t     tsc_coroutine_handler_t

#define __CoroC_Spawn(F, A) \
            tsc_coroutine_allocate((F), (A), "", TSC_COROUTINE_NORMAL, NULL)
#define __CoroC_Spawn_Opt(F, A, CB) \
            tsc_coroutine_allocate((F), (A), "", TSC_COROUTINE_NORMAL, (CB))

#define __CoroC_Yield   tsc_coroutine_yield
#define __CoroC_Self    tsc_coroutine_self
#define __CoroC_Quit    tsc_coroutine_exit
#define __CoroC_Exit    tsc_coroutine_exit

/// for group ops
#define __CoroC_Group       tsc_group_alloc
#define __CoroC_Add_Task    tsc_group_add_task
#define __CoroC_Notify      tsc_group_notify
#define __CoroC_Sync        tsc_group_sync

/// for channel ops
#define __CoroC_Chan tsc_chan_allocate
#define __CoroC_Chan_Close(C)  tsc_chan_close((C))
#define __CoroC_Null       NULL

#define __CoroC_Chan_SendExpr(C, V) ({ \
    typeof(V) __temp = (V); \
    _Bool ret = _tsc_chan_send(C, (void*)(&__temp), 1) != CHAN_CLOSED; \
    ret;})
#define __CoroC_Chan_Send(C, P) ({ \
    _Bool ret = _tsc_chan_send(C, (void*)(P), 1) != CHAN_CLOSED; \
    ret;})
#define __CoroC_Chan_Recv(C, P) ({ \
    _Bool ret = _tsc_chan_recv(C, (void*)(P), 1) != CHAN_CLOSED; \
    ret;})

#define __CoroC_Chan_SendExpr_NB(C, V) ({ \
    typeof(V) __temp = (V); \
    _Bool ret = _tsc_chan_send(C, (void*)(&__temp), 0) == CHAN_SUCCESS; \
    ret;})
#define __CoroC_Chan_Send_NB(C, P) ({ \
    _Bool ret = _tsc_chan_send(C, (void*)(P), 0) == CHAN_SUCCESS; \
    ret;})
#define __CoroC_Chan_Recv_NB(C, P) ({ \
    _Bool ret = _tsc_chan_recv(C, (void*)(P), 0) == CHAN_SUCCESS; \
    ret;})

#define __CoroC_Chan_SendRef(C, R) ({   \
    _Bool ret = 1;                      \
    tsc_refcnt_get((tsc_refcnt_t)(*(R)));  \
    if (_tsc_chan_send(C, (void*)(R), 1) == CHAN_CLOSED) {  \
      ret = 0; tsc_refcnt_put((tsc_refcnt_t)(*(R))); }         \
    ret;})
#define __CoroC_Chan_RecvRef(C, R) ({       \
    tsc_refcnt_put((tsc_refcnt_t)(*(R)));   \
    _Bool ret = _tsc_chan_recv(C, (void*)(R), 0) != CHAN_CLOSED; \
    ret;})

#if 0
#define __CoroC_Chan_SendRef_NB(C, R) ({    \
    _Bool ret = 1;                          \
    tsc_refcnt_get((tsc_refcnt_t)(R));      \
    if (_tsc_chan_send(C, (void*)(R), 1) != CHAN_SUCCESS) { \
      ret = 0; tsc_refcnt_put((tsc_refcnt_t)(R)); }         \
    ret;})
#define __CoroC_Chan_RecvRef_NB(C, R) ({    \
    tsc_refcnt_put((tsc_refcnt_t)(*(R)));   \
    _Bool ret = _tsc_chan_recv(C, (void*)(R), 0) == CHAN_SUCCESS; \
    ret;})
#endif

#define __CoroC_Chan_SendRef_NB(C, R) ({ \
    tsc_refcnt_get((tsc_refcnt_t)(*(R)));\
    _Bool ret = _tsc_chan_send(C, (void*)(R), 1) == CHAN_SUCCESS; \
    ret;)}
#define __CoroC_Chan_RecvRef_NB(C, R) ({ \
    tsc_refcnt_put((tsc_refcnt_t)(*(R)));\
    _Bool ret = _tsc_chan_recv(C, (void*)(R), 1) == CHAN_SUCCESS; \
    ret;)}

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

#define __CoroC_Select_SendRef(S, C, R) do {        \
            tsc_refcnt_get((tsc_refcnt_t)(*(R)));   \
            tsc_chan_set_send(S, C, (void*)(R)); } while (0)
#define __CoroC_Select_RecvRef(S, C, R) do {        \
            tsc_refcnt_put((tsc_refcnt_put)(*(R))); \
            tsc_chan_set_recv(S, C, (void*)(R)); } while (0)

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

#define __CoroC_Stop(T)  tsc_timer_stop((tsc_timer_t)(T))

/// for time management
#define __CoroC_Now     tsc_getcurtime
#define __CoroC_Sleep   tsc_udelay

/// for explicity release the reference's counter
#define __CoroC_Task_Release(T) \
        if (tsc_refcnt_put((tsc_refcnt_t)(T))) (T) = NULL;
#define __CoroC_Chan_Release(C) \
        if (tsc_refcnt_put((tsc_refcnt_t)(C))) (C) = NULL; 

/// for async call APIs
#define __CoroC_async_handler_t tsc_async_callback_t
#define __CoroC_Async_Call tsc_async_request_submit

#endif // defined(__COROC__)

#endif // __LIBCOROC_H__
