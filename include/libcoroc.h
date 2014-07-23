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
#define __chan_t        tsc_chan_t
#define __task_t        tsc_coroutine_t

/// for reference-counting ops
#define __refcnt_t      tsc_refcnt_t
#define __refcnt_get(R) (typeof(R))tsc_refcnt_get((tsc_refcnt_t)(R))
#define __refcnt_put(R) tsc_refcnt_put((tsc_refcnt_t)(R))

/// for coroutine ops
#define __CoroC_spawn_handler_t     tsc_coroutine_handler_t
#define __CoroC_Spawn(F, A) \
            tsc_coroutine_allocate((F), (A), "", TSC_COROUTINE_NORMAL, NULL)
#define __CoroC_Yield   tsc_coroutine_yield
#define __CoroC_Quit    tsc_coroutine_exit

/// for channel ops
#define __CoroC_Chan tsc_chan_allocate
#define __CoroC_Chan_Send(C, V) tsc_chan_sende(C, V)
#define __CoroC_Chan_Recv(C, P) tsc_chan_recv(C, (void*)(P))
// TODO : channel select op ..

#endif // __LIBCOROC_H__
