#ifndef _TSC_TIME_H_
#define _TSC_TIME_H_

#include "support.h"
#include "vpu.h"
#include "channel.h"
#include "coroutine.h"

/* internal timer type */
typedef struct {
    uint64_t when;
    uint32_t period;
    void (*func)(void*);
    void* args;
} tsc_inter_timer_t; 

/* userspace timer type */
typedef struct tsc_timer {
    tsc_inter_timer_t timer;
    tsc_chan_t chan;
} *tsc_timer_t;

/* userspace api */

tsc_timer_t tsc_timer_allocate (uint32_t period, void (*func)(void));
void tsc_timer_dealloc (tsc_timer_t );
tsc_chan_t tsc_timer_after (tsc_timer_t, uint64_t);
tsc_chan_t tsc_timer_at (tsc_timer_t, uint64_t);

int tsc_timer_start (tsc_timer_t);
int tsc_timer_stop (tsc_timer_t);
int tsc_timer_reset (tsc_timer_t, uint64_t);

/* internal api */
int tsc_add_intertimer (tsc_inter_timer_t*);
int tsc_del_intertimer (tsc_inter_timer_t*);

#endif // _TSC_TIME_H_ 
