#include <stdlib.h>
#include <stdio.h>

#include "thread.h"
#include "channel.h"
#include "time.h"

int user_main (int argc, char ** argv)
{
    uint64_t awaken = 0;
    tsc_timer_t timer = timer_allocate (0, 0);
    channel_t chan = timer_after (timer, 1000 * 2); // 2 seconds later

    printf ("waiting for 2 seconds!\n");

    channel_recv(chan, &awaken);

    printf ("awaken, time is %llu!\n", awaken);

	thread_exit (0);
}
