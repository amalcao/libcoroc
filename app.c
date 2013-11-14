/* - - - - 
 * The User APP using LibTSC ..
 * - - - - */

#include <stdio.h>
#include "thread.h"
#include "channel.h"

# if 0
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

# endif

void sub_task (void * arg)
{
    thread_t parent = (thread_t)arg;
    message_t msg;

    message_recv (& msg, parent, 1);

    printf ("[sub_task:] recv id is %d!\n", *((int*)(msg->buff)));

    int i = 0;
    for (; i< 100000; i++);

    message_send (msg, parent);

    message_deallocate (msg);
    thread_exit (0);
}


int user_main (void * arg)
{
    thread_t threads[100];
    message_t msg = message_allocate (sizeof (int));

    int i;
    for (i = 0; i<100; ++i) {
        threads[i] = thread_allocate (sub_task, thread_self(), "", TSC_THREAD_NORMAL, 0);
    }

    for (i = 0; i<100; ++i) {
        *((int *)(msg -> buff)) = i;
        message_send (msg, threads[i]);
    }

    message_deallocate (msg);
    msg = 0;

    for (i=0; i< 100; ++i) {
        message_recv (& msg, threads[i], 1);
        printf ("[main_task:] recv id is %d!\n",
            *((int*)(msg->buff)));
        message_deallocate (msg);
    }

    thread_exit (0);

    return 0;
}
