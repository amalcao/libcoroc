#ifndef _TSC_LOCK_CHAIN_H_
#define _TSC_LOCK_CHAIN_H_

#include <stdlib.h>
#include "lock.h"

#define TSC_LOCKCHAIN_DEFAULT_VOLUME 8

typedef struct {
	bool sorted;
	uint32_t size;
	uint32_t volume;
	lock_t * locks;
} lock_chain_t;

static int lock_addr_comp (lock_t lock1, lock_t lock2)
{
	return *(uint64_t*)(lock1) - *(uint64_t*)(lock2);
}

static void lock_chain_init (lock_chain_t * chain)
{
	chain -> sorted = false;
	chain -> size = 0;
	chain -> volume = TSC_LOCKCHAIN_DEFAULT_VOLUME;
	chain -> locks = TSC_ALLOC(sizeof(lock_t) * chain -> volume);
}

static void lock_chain_fini (lock_chain_t * chain)
{
	TSC_DEALLOC(chain -> locks);
}

static void lock_chain_add (lock_chain_t * chain, lock_t lock)
{
	if (chain -> size == chain -> volume) {
		chain -> volume *= 2;
		chain -> locks = TSC_REALLOC(chain -> locks, sizeof(lock_t) * chain -> volume);
	}

	chain -> locks[chain -> size ++] = lock;
}

static void lock_chain_acquire (lock_chain_t * chain)
{
	if (!chain -> sorted) {
		qsort (chain -> locks, chain -> size,
			sizeof (lock_t), lock_addr_comp);
		chain -> sorted = true;
	}
	
	int i = 0;
	for (; i < chain -> size; i++) 
		lock_acquire (chain -> locks[i]);
}

static void lock_chain_release (lock_chain_t * chain)
{
	int i = chain -> size - 1;
	for (; i>=0; i--)
		lock_release (chain -> locks[i]);
}

#endif // _TSC_LOCK_CHAIN_H_
