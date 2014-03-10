/* Copyright (c) 2005 Russ Cox, MIT; see COPYRIGHT */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libtsc.h"

int quiet = 0;
int goal = 100;
int buffer = 0;

int primetask(void *arg)
{
	channel_t c, nc;
	unsigned long p, i;
	c = arg;

	channel_recv (c, & p);

	if(p > goal)
		exit (0);

	if(!quiet)
		printf("%d\n", p);

	nc = channel_allocate (sizeof(unsigned long), buffer);
	thread_allocate (primetask, nc, "", TSC_THREAD_NORMAL, NULL);
	for(;;){
		channel_recv (c, & i);
		if(i%p)
			channel_send (nc, & i);
	}
	return 0;
}

void main(int argc, char ** argv)
{
	unsigned long i;
	channel_t c;

	printf("goal=%d\n", goal);

	c = channel_allocate(sizeof(unsigned long), buffer);
	thread_allocate (primetask, c, "", TSC_THREAD_NORMAL, NULL);
	for(i=2;; i++)
		channel_send (c, &i);
}

