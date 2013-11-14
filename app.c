/* - - - - 
 * The User APP using LibTSC ..
 * - - - - */

#include <stdio.h>
#include "thread.h"

void sub_task(void *id)
{
	int i, j;
	for (i=0; i<10; ++i) {
		printf("Hello ! my id is %d!\n", (int)id);
        for (j=0; j<10000000; ++j);
#ifndef ENABLE_TIMER
		thread_yeild();
#endif
	}
	
	thread_exit(0);
}

int user_main (void* id)
{	
	int i;
	for (i=1; i<100; i++) {
	    thread_t thread = thread_allocate (sub_task, i, "", TSC_THREAD_NORMAL, 0);
    }
	sub_task (0);
}

