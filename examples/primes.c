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
	tsc_chan_t c, nc;
	unsigned long p, i;
	c = arg;

	tsc_chan_recv (c, & p);

	if(p > goal)
		exit (0);

	if(!quiet)
		printf("%d\n", p);

	nc = tsc_chan_allocate (sizeof(unsigned long), buffer);
	tsc_coroutine_allocate (primetask, nc, "", TSC_COROUTINE_NORMAL, NULL);
	for(;;){
		tsc_chan_recv (c, & i);
		if(i%p)
			tsc_chan_send (nc, & i);
	}
	return 0;
}

void main(int argc, char ** argv)
{
	unsigned long i;
	tsc_chan_t c;

	printf("goal=%d\n", goal);

	c = tsc_chan_allocate(sizeof(unsigned long), buffer);
	tsc_coroutine_allocate (primetask, c, "", TSC_COROUTINE_NORMAL, NULL);
	for(i=2;; i++)
		tsc_chan_send (c, &i);
}

