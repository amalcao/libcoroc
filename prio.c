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
    int id, i;

    recv (parent, sizeof(int), &id, true);
    // printf ("[sub_task:] recv id is %d!\n", id);

    for (i=0; i<50000000; i++);

    send (parent, sizeof(int), &id);

    thread_exit (0);
}


int user_main (void * arg)
{
    thread_t threads[100];
    thread_attributes_t attr;
    thread_attr_init (& attr);
        // thread_attr_set_stacksize (& attr, 8*1024*1024); // 4MB stack

    int i;
    for (i = 0; i<100; ++i) {
        if (i % 2) thread_attr_set_timeslice (& attr, 2);
        else thread_attr_set_timeslice (& attr, 4);

        threads[i] = thread_allocate (sub_task, thread_self(), "", TSC_THREAD_NORMAL, & attr);
    }

    for (i = 0; i<100; ++i) {
        send (threads[i], sizeof(int), &i);
    }

    for (i=0; i< 100; ++i) {
        int id;
        recv (NULL, sizeof(int), &id,  true);
        printf ("[main_task:] recv id is %d!\n", id);
    }

    thread_exit (0);

    return 0;
}
