#include <stdlib.h>
#include <stdio.h>

#include "libtsc.h"

#define SIZE 4

int sub_task (tsc_chan_t chan)
{
	int id = random();
	tsc_chan_send (chan, &id);

	tsc_coroutine_exit (0);
}

int main (int argc, char ** argv)
{
	tsc_coroutine_t thrds[SIZE];
	tsc_chan_t chans[SIZE];
	tsc_chan_set_t set = tsc_chan_set_allocate ();
	int i = 0, id;

	for (; i < SIZE; i++) {
		chans[i] = tsc_chan_allocate (sizeof(int), 0);
		thrds[i] = tsc_coroutine_allocate (sub_task, chans[i], "", TSC_COROUTINE_NORMAL, NULL);
		tsc_chan_set_recv (set, chans[i], &id);
	}

	for (i = 0; i < SIZE; i++) {
		tsc_chan_t ch = NULL;
		tsc_chan_set_select(set, &ch);
		if (ch != NULL) {
			printf ("[main task]: recv id is %d!\n", id);
		}
	}

	tsc_chan_set_dealloc (set);
	for (i = 0; i < SIZE; i++)
		tsc_chan_dealloc (chans[i]);

	tsc_coroutine_exit (0);
}
