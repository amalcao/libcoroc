// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef __LIBCOROC_H__
#define __LIBCOROC_H__

#if (defined(__COROC__) || !defined(__GEN_BY_CLANG_COROC__))

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <setjmp.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "inter/support.h"
#include "inter/coroutine.h"
#include "inter/channel.h"
#include "inter/message.h"
#include "inter/async.h"
#include "inter/netpoll.h"
#include "inter/coroc_group.h"
#include "inter/coroc_time.h"
#include "inter/vfs.h"

#ifdef __cplusplus
}
#endif // __cplusplus

#elif defined(__GEN_BY_CLANG_COROC__)

/* standard definations for CoroC internals */

/* NOTE: These macros must be designed carefully to avoid 
 *       include other macros which defined in other header files! */

/// basic builtin types

#if !defined(NULL)
# define NULL ((void*)0)
#endif

#define __chan_t        coroc_chan_t
#define __task_t        coroc_coroutine_t
#define __select_set_t  coroc_chan_set_t
#define __coroc_time_t  uint64_t
#define __group_t       coroc_group_t

/// for reference-counting ops
#define __refcnt_t      coroc_refcnt_t
#define __refcnt_get(R) (typeof(R))__coroc_refcnt_get((coroc_refcnt_t)(R))
#define __refcnt_put(R) __coroc_refcnt_put((coroc_refcnt_t)(R))

#define __CoroC_release_handler_t release_handler_t

#define __CoroC_New(F0, T0, T, S, F, A) ({ \
            T0 *ref = calloc(sizeof(T0) + (S-1)*sizeof(T) + A, 1); \
            ref->__refcnt.release = ((release_handler_t)(F0)); \
            ref->__user_fini = ((typeof(ref->__user_fini))(F));\
            ref->__obj_num = (S);               \
            __coroc_refcnt_get((coroc_refcnt_t)(ref)); })

#define __CoroC_New_Basic(T0, T, S) ({ \
            T0 *ref = calloc(sizeof(T0) + (S-1)*sizeof(T), 1); \
            ref->__refcnt.release = ((release_handler_t)free); \
            __coroc_refcnt_get((coroc_refcnt_t)(ref)); })

/* more interfaces for new auto scope branch */
#define __refcnt_assign(D, S) \
            (typeof(D)) __coroc_refcnt_assign( \
                            (coroc_refcnt_t*)(&(D)), \
                            (coroc_refcnt_t)(S) )

#define __refcnt_assign_expr(D, E) \
            (typeof(D)) __coroc_refcnt_assign_expr( \
                            (coroc_refcnt_t*)(&(D)), \
                            (coroc_refcnt_t)(E) )

#define __refcnt_put_array(A, N) ({ \
            int i = 0;  \
            for (i = 0; i < (N); ++i) \
              __coroc_refcnt_put((coroc_refcnt_t)((A)[i])); })

/// for coroutine ops
#define __CoroC_spawn_entry_t     coroc_coroutine_handler_t
#define __CoroC_spawn_cleanup_t   coroc_coroutine_cleanup_t

#define __CoroC_Spawn(F, A, P, CB) \
            coroc_coroutine_allocate((F), (A), "", \
                                   TSC_COROUTINE_NORMAL, \
                                   P, (CB))

#define __CoroC_Yield   coroc_coroutine_yield
#define __CoroC_Self    coroc_coroutine_self
#define __CoroC_Quit    coroc_coroutine_exit
#define __CoroC_Exit    coroc_coroutine_exit

#define __CoroC_Set_Name    coroc_coroutine_set_name
#define __CoroC_Set_Prio    coroc_coroutine_set_priority

/// for group ops
#define __CoroC_Group       coroc_group_alloc
#define __CoroC_Add_Task    coroc_group_add_task
#define __CoroC_Notify      coroc_group_notify
#define __CoroC_Sync(G) ({      \
    int ret = coroc_group_sync(G);\
    if (G) { free(G); G = 0; } ret; })

/// for channel ops
#define __CoroC_Chan _coroc_chan_allocate
#define __CoroC_Chan_Close(C)  coroc_chan_close((C))
#define __CoroC_Null       NULL

#define __CoroC_Chan_SendExpr(C, V) ({ \
    typeof(V) __temp = (V); \
    _Bool ret = _coroc_chan_send(C, (void*)(&__temp), 1) != CHAN_CLOSED; \
    ret;})
#define __CoroC_Chan_Send(C, P) ({ \
    _Bool ret = _coroc_chan_send(C, (void*)(P), 1) != CHAN_CLOSED; \
    ret;})
#define __CoroC_Chan_Recv(C, P) ({ \
    _Bool ret = _coroc_chan_recv(C, (void*)(P), 1) != CHAN_CLOSED; \
    ret;})

#define __CoroC_Chan_SendExpr_NB(C, V) ({ \
    typeof(V) __temp = (V); \
    _Bool ret = _coroc_chan_send(C, (void*)(&__temp), 0) == CHAN_SUCCESS; \
    ret;})
#define __CoroC_Chan_Send_NB(C, P) ({ \
    _Bool ret = _coroc_chan_send(C, (void*)(P), 0) == CHAN_SUCCESS; \
    ret;})
#define __CoroC_Chan_Recv_NB(C, P) ({ \
    _Bool ret = _coroc_chan_recv(C, (void*)(P), 0) == CHAN_SUCCESS; \
    ret;})

#define __CoroC_Chan_SendRef(C, R) ({   \
    _Bool ret = 1;                      \
    __coroc_refcnt_get((coroc_refcnt_t)(*(R)));  \
    if (_coroc_chan_send(C, (void*)(R), 1) == CHAN_CLOSED) {  \
      ret = 0; __coroc_refcnt_put((coroc_refcnt_t)(*(R))); }         \
    ret;})
#define __CoroC_Chan_RecvRef(C, R) ({       \
    __coroc_refcnt_put((coroc_refcnt_t)(*(R)));   \
    _Bool ret = _coroc_chan_recv(C, (void*)(R), 1) != CHAN_CLOSED; \
    ret;})

#define __CoroC_Chan_SendRef_NB(C, R) ({ \
    __coroc_refcnt_get((coroc_refcnt_t)(*(R)));\
    _Bool ret = _coroc_chan_send(C, (void*)(R), 0) == CHAN_SUCCESS; \
    if (!ret) __coroc_refcnt_put((coroc_refcnt_t)(*(R))); \
    ret;})
#define __CoroC_Chan_RecvRef_NB(C, R) ({ \
    __coroc_refcnt_put((coroc_refcnt_t)(*(R)));\
    _Bool ret = _coroc_chan_recv(C, (void*)(R), 0) == CHAN_SUCCESS; \
    ret;})

///  channel select ops ..
#if 0
#define __CoroC_Select_Size(N)      \
    (sizeof(struct coroc_chan_set) +  \
      (N) * (sizeof(coroc_scase_t) + sizeof(lock_t)))

#define __CoroC_Select_Alloc(N) alloca(__CoroC_Select_Size(N))
#endif

#define __CoroC_Select_Alloc(N)   coroc_chan_set_allocate(N)
#define __CoroC_Select_Dealloc(S) coroc_chan_set_dealloc(S)
#define __CoroC_Select_Init(S, N) coroc_chan_set_init(S, N)

#define __CoroC_Select(S, B) ({ \
    coroc_chan_t active = NULL;   \
    _coroc_chan_set_select(S, B, &active); \
    active; })
#define __CoroC_Select_SendExpr(S, C, E) \
    do { typeof(E) __temp = E; \
         coroc_chan_set_send(S, C, &__temp); } while (0)
#define __CoroC_Select_Send     coroc_chan_set_send
#define __CoroC_Select_Recv     coroc_chan_set_recv

#define __CoroC_Select_SendRef(S, C, R) do {        \
            __coroc_refcnt_get((coroc_refcnt_t)(*(R)));   \
            coroc_chan_set_send(S, C, (void*)(R)); } while (0)
#define __CoroC_Select_RecvRef(S, C, R) do {        \
            __coroc_refcnt_put((coroc_refcnt_t)(*(R))); \
            coroc_chan_set_recv(S, C, (void*)(R)); } while (0)

/// for Task based send / recv
#define __CoroC_Task_Send(task, buf, sz) coroc_send(*(task), buf, sz)
#define __CoroC_Task_Recv(buf, sz) coroc_recv(buf, sz, 1)
#define __CoroC_Task_Recv_NB(buf, sz) coroc_recv(buf, sz, 0)

/// for Timer / Ticker ..
#define __CoroC_Timer_At(at) ({ \
    coroc_timer_t __timer = coroc_timer_allocate(0, NULL); \
    coroc_timer_at(__timer, at); \
    (coroc_chan_t)(__timer); })

#define __CoroC_Timer_After(after) ({ \
    coroc_timer_t __timer = coroc_timer_allocate(0, NULL); \
    coroc_timer_after(__timer, after); \
    (coroc_chan_t)(__timer); })

#define __CoroC_Ticker(period) ({ \
    coroc_timer_t __ticker = coroc_timer_allocate(period, NULL); \
    coroc_timer_after(__ticker, period);  \
    (coroc_chan_t)(__ticker); })

#define __CoroC_Stop(T)  coroc_timer_stop((coroc_timer_t)(T))

/// for time management
#define __CoroC_Now     coroc_getmicrotime
#define __CoroC_Sleep   coroc_udelay

/// for explicity release the reference's counter
#define __CoroC_Task_Release(T) \
        if (__coroc_refcnt_put((coroc_refcnt_t)(T))) (T) = NULL;
#define __CoroC_Chan_Release(C) \
        if (__coroc_refcnt_put((coroc_refcnt_t)(C))) (C) = NULL; 

/// for async call APIs
#define __CoroC_async_handler_t coroc_async_callback_t
#define __CoroC_Async_Call coroc_async_request_submit

#endif // defined(__COROC__)

#endif // __LIBCOROC_H__
