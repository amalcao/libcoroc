/* - - - - 
 * The User APP using LibTSC ..
 * - - - - */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "thread.h"
#include "channel.h"

void sub_task (void * arg)
{
    uint64_t id = (uint64_t)arg;

    printf ("[sub_task:] id is %i!\n", id);

	for (;;) {
#ifndef ENABLE_TIMER
		thread_yield ();
#endif 
	}

    thread_exit (0);
}


int user_main (void * arg)
{
    thread_t threads[100];

    int i;
    for (i = 0; i<100; ++i) {
        threads[i] = thread_allocate (sub_task, i, "", TSC_THREAD_NORMAL, 0);
    }

	for (;;) {
#ifndef ENABLE_TIMER
		thread_yield ();
#endif 
	}

    thread_exit (0);

    return 0;
}
