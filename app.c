/* - - - - 
 * The User APP using LibTSC ..
 * - - - - */

#include <stdlib.h>
#include <stdio.h>
#include "thread.h"
#include "channel.h"

void sub_task (void * arg)
{
    thread_t parent = (thread_t)arg;
    message_t msg;

    message_recv (& msg, parent, 1);

    printf ("[sub_task:] recv id is %d!\n", *((int*)(msg->buff)));

	srand (arg);

    int i = 0;
    for (; i < rand()%1000000; i++);

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
        message_recv (& msg, NULL, 1);
        printf ("[main_task:] recv id is %d!\n",
            *((int*)(msg->buff)));
        message_deallocate (msg);
    }

    thread_exit (0);

    return 0;
}
