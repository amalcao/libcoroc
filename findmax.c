#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "thread.h"
#include "channel.h"

#define MAX 2048

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
int find_max (void * arg)
{
	thread_t parent = (thread_t)arg;
	int size, * buf;
	size_t recvsz = sizeof(int);

	// firstly, get the size ..
	recv (parent, &recvsz, &size, true);
	if (size <= 0) thread_exit (-1);

	buf = malloc (size * sizeof(int));
	recvsz = size * sizeof(int);
	recv (parent, &recvsz, buf, true);

	if (size > 1) {
		if (size > 2) {

			thread_t slave0, slave1;
			int sz0, sz1;

			sz0 = size / 2;
			sz1 = size - sz0;

			slave0 = thread_allocate (find_max, thread_self(), "", TSC_THREAD_NORMAL, 0);
			send (slave0, sizeof(int), &sz0);
			send (slave0, sz0*sizeof(int), buf);

			slave1 = thread_allocate (find_max, thread_self(), "", TSC_THREAD_NORMAL, 0);
			send (slave1, sizeof(int), &sz1);
			send (slave1, sz1*sizeof(int), buf + sz0);
			
			recvsz = sizeof (int);
			recv (NULL, &recvsz, & buf[0], true);
			recv (NULL, &recvsz, & buf[1], true);
		}
		if (buf[0] < buf[1]) buf[0] = buf[1];
	}

	send (parent, sizeof(int), buf);

	free (buf);
	thread_exit (0);
}

/* -- the user entry -- */
int user_main (void * arg)
{
	int max = 0, size = MAX;
	int * array = malloc (size * sizeof(int));
	size_t recvsz = sizeof(int);
	thread_t slave = NULL;

	gen_array_elem (array, size);
	slave = thread_allocate (find_max, thread_self(), "", TSC_THREAD_NORMAL, 0);

	send (slave, sizeof(int), &size); // send the size of array
	send (slave, size * sizeof(int), array); // send the content of array
	recv (slave, &recvsz, &max, true);

	printf ("The MAX element is %d\n", max);
	if (!check_max_elem (array, size, max))
		printf ("The answer is wrong!\n");

	free (array);
	thread_exit (0);
}
