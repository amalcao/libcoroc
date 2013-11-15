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
    thread_t parent = (thread_t)arg;
    int id;
    size_t size = sizeof (int);

    recv (parent, &size, &id, true);

    printf ("[sub_task:] recv id is %d!\n", id);

	srand (&arg);

    int i = 0;
    for (; i < rand()%1000000; i++);

    send (parent, sizeof(int), &id);

    thread_exit (0);
}


int user_main (void * arg)
{
    thread_t threads[100];

    int i;
    for (i = 0; i<100; ++i) {
        threads[i] = thread_allocate (sub_task, thread_self(), "", TSC_THREAD_NORMAL, 0);
    }

    for (i = 0; i<100; ++i) {
        send (threads[i], sizeof(int), &i);
    }

    for (i=0; i< 100; ++i) {
        int id;
        size_t size = sizeof (int);
        recv (NULL, &size, &id,  true);
        printf ("[main_task:] recv id is %d!\n", id);
    }

    thread_exit (0);

    return 0;
}
