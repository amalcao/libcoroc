#include <stdlib.h>
#include <stdio.h>

#include "thread.h"
#include "channel.h"

#define SIZE 4

int sub_task (channel_t chan)
{
	int id = random();
	channel_send (chan, &id);

	thread_exit (0);
}

int user_main (void * args)
{
	thread_t thrds[SIZE];
	channel_t chans[SIZE];
	chan_set_t set = chan_set_allocate ();
	int i = 0, id;

	for (; i < SIZE; i++) {
		chans[i] = channel_allocate (sizeof(int), 0);
		thrds[i] = thread_allocate (sub_task, chans[i], "", TSC_THREAD_NORMAL, NULL);
		chan_set_recv (set, chans[i]);
	}

	for (i = 0; i < SIZE; i++) {
		channel_t ch = NULL;
		chan_set_select(set, &id, &ch);
		if (ch != NULL) {
			printf ("[main task]: recv id is %d!\n", id);
		}
	}

	chan_set_dealloc (set);
	for (i = 0; i < SIZE; i++)
		channel_dealloc (chans[i]);

	thread_exit (0);
}
