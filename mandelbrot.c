#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "thread.h"
#include "channel.h"

const double ZERO = 0;
const double LIMIT = 2.0;
const int ITER = 50;
const int SIZE = 16000;

uint8_t * rows;
int bytesPerRow;

int w, h, iter;

// This func is responsible for rendering a row of pixels,
// and when complete writing it out to the file.

void renderRow (channel_t * chans)
{
	channel_t workChan = chans[0];
	channel_t finishChan = chans[1];

	double Zr, Zi, Tr, Ti, Cr, Ci;
	int x, y, i;
	int offset;

	channel_recv (workChan, & y);
	offset = y * bytesPerRow;
	Ci = (2 * (double)y/(double)h - 1.0);

	for (x = 0; x < w; x++) {
		Zr = Zi = Tr = Ti = ZERO;
		Cr = (2 * (double)x/(double)w - 1.5);

		for (i = 0; i < iter && Tr+Ti <= LIMIT*LIMIT; i++) {
			Zi = 2 * Zi * Zr + Ci;
			Zr = Tr - Ti + Cr;
			Tr = Zr * Zr;
			Ti = Zi * Zi;
		}

		// Store tje value in the array of ints
		if (Tr + Ti <= LIMIT*LIMIT) 
			rows[offset + x/8] |= (1 << (uint32_t)(7 - (x%8)));
	}

	bool finish = true;
	channel_send (finishChan, & finish);

}

#define POOL 4
#define WORK_CHAN 0
#define FINISH_CHAN 1

void user_main (int argc, char ** argv)
{
	int size, y;
	channel_t chans[2];

	size = (argc > 1) ? atoi(argv[1]) : SIZE;

	iter = ITER;
	w = h = size;
	bytesPerRow = w / 8;
	
	rows = malloc (sizeof(uint8_t) * bytesPerRow * h);
    memset (rows, 0, bytesPerRow * h);

	chans[WORK_CHAN] = channel_allocate (sizeof(int), 2*POOL+1);
	chans[FINISH_CHAN] = channel_allocate (sizeof(bool), 0);

	for (y = 0; y < size; y++) {
		thread_t slave = thread_allocate (renderRow, chans, "", TSC_THREAD_NORMAL, NULL);
		channel_send (chans[WORK_CHAN], & y);
	}

	for (y = 0; y < size; y++) {
		bool finish;
		channel_recv (chans[FINISH_CHAN], & finish);
	}

#if 0 // for check the result ..
    fwrite (rows, h * w / 8, 1, stdout);
#endif
    free (rows);

	return ;
}
