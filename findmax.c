#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "libtsc.h"

#define MAX 16

int * array;

void gen_array_elem (int * array, int size)
{
	int i;

	srand(array);

	for (i = 0; i < size; i++) {
		array[i] = rand();
	}
}

bool check_max_elem (int * array, int size, int max)
{
	int i;
	for (i = 0; i < size; i++) {
		if (array[i] > max)
			return false;
	}

	return true;
}

/* -- the slave entry -- */
int find_max (channel_t chan)
{
	thread_t self = thread_self();
	int size, start, max[2];

	// get the size firstly
	channel_recv (chan, &size);
	if (size <= 0) thread_exit (-1);
	// get my tasks' start index
	channel_recv (chan, &start);

	max[0] = array[start];
	max[1] = array[start+1];
	if (size > 1) {
		if (size > 2) {

			thread_t slave[2];
			channel_t ch[2];
			int sz[2], st[2];

			sz[0] = size / 2; st[0] = start;
			sz[1] = size - sz[0]; st[1] = start + sz[0];

			int i = 0;
			for (; i<2; ++i) {
				ch[i] = channel_allocate (sizeof(int), 0);
				slave[i] = thread_allocate (find_max, ch[i], "", TSC_THREAD_NORMAL, 0);
				channel_send (ch[i], &sz[i]);
				channel_send (ch[i], &st[i]);
			}

			channel_recv (ch[0], &max[0]);
			channel_recv (ch[1], &max[1]);
		}
		if (max[0] < max[1]) max[0] = max[1];
	}

	channel_send (chan, &max[0]);
	thread_exit (0);
}

/* -- the user entry -- */
int user_main (int argc, char ** argv)
{
	int max = 0, size = MAX, start = 0;
	array = malloc (size * sizeof(int));
	thread_t slave = NULL;
	channel_t chan = NULL;

	gen_array_elem (array, size);
	chan = channel_allocate (sizeof(int), 0);
	slave = thread_allocate (find_max, chan, "s", TSC_THREAD_NORMAL, 0);

	channel_send (chan, &size); // send the size of array
	channel_send (chan, &start); // send the start index of array

	channel_recv (chan, &max); // recv the result

	printf ("The MAX element is %d\n", max);
	if (!check_max_elem (array, size, max))
		printf ("The answer is wrong!\n");

	free (array);
	thread_exit (0);
}
