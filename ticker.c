#include <stdlib.h>
#include <stdio.h>

#include "libtsc.h"

void callback (void)
{
    printf ("\ttime out!\n");
}

int main (int argc, char ** argv)
{
    uint64_t awaken = 0;
    int i = 0;
    tsc_timer_t timer = timer_allocate (1000000 * 2, callback);
    channel_t chan = timer_after (timer, 1000000 * 2); // 2 seconds later

    for (i = 0; i < 10; i++) {
        printf ("waiting for 2 seconds!\n");
        channel_recv(chan, &awaken);
        printf ("awaken, time is %llu!\n", (long long unsigned)awaken);
    }

    printf ("release the timer ..\n");
    timer_stop (timer);
    timer_dealloc (timer);
    
	thread_exit (0);
}
