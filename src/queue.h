#ifndef _TSC_SUPPORT_QUEUE_H_
#define _TSC_SUPPORT_QUEUE_H_

#include <stdint.h>
#include <stdbool.h>
#include "lock.h"

struct queue;

typedef struct queue_item {
	struct queue_item * next;
	struct queue_item * prev;
    struct queue * que;
	void * owner;
} queue_item_t;

typedef struct queue {
	lock lock;
	queue_item_t  * head;
	queue_item_t  * tail;
	volatile uint32_t status;
} queue_t;

/*-----------------------------------*
 * Inlined queue management function *
 * The prev/next fields of the task  *
 * parameter are NULLified her !     *
 *-----------------------------------*/

/*---- Initilization functions ----*/
static inline void queue_item_init (queue_item_t * item, void * owner) {
	item -> next = NULL;
    item -> prev = NULL;
    item -> que = NULL;
	item -> owner = owner;
}

static inline void queue_init (queue_t * queue) {
	queue -> head = NULL;
	queue -> tail = NULL;
	queue -> status = 0;
}

static inline void atomic_queue_init (queue_t * queue) {
	lock_init(& queue -> lock);
	queue_init (queue);
}

static inline int atomic_queue_size (queue_t * queue) {
    int size;
    lock_acquire(& queue -> lock);
    size = queue -> status;
    lock_release(& queue -> lock);
    
    return size;
}

/*---- Add functions ----*/
static inline void queue_add (queue_t * queue, queue_item_t * item) {
	/*---- Clean the links ----*/
	item -> next = NULL;

	/*---- Place the item ----*/
	if (! queue -> status) {
		queue -> head = item;
		queue -> tail = item;
	}
	else {
		queue -> tail -> next = item;
        item -> prev = queue -> tail;
		queue -> tail = item;
	}

    item -> que = queue;
	queue -> status += 1;
}

static inline void atomic_queue_add (queue_t * queue, queue_item_t * item) {
	lock_acquire(& queue -> lock);
	queue_add (queue, item);
	lock_release(& queue -> lock);
}

/*---- Remove function ----*/
static inline void * queue_rem (queue_t * queue) {
	queue_item_t * kitem = NULL;

	/*---- Check if the queue is empty ----*/
	if (! queue -> status) return NULL;

	/*---- Check what item to remove ----*/
	kitem = queue -> head;
	queue -> head = queue -> head -> next;
    if (queue -> head)
        queue -> head -> prev = NULL;
    else
        queue -> tail = queue -> head;

	queue -> status -= 1;

	/*---- Clean the links ----*/
    kitem -> prev = kitem -> next = NULL;
	kitem -> que = NULL;

	return kitem -> owner;
}

static inline void * atomic_queue_rem (queue_t * queue) {
	void * owner = NULL;

	if (! queue -> status) return NULL;

	lock_acquire(& queue -> lock);
	owner = queue_rem (queue);
	lock_release(& queue -> lock);	

	return owner;
}

/*---- Extract function ----*/
static inline void queue_extract (queue_t * queue, queue_item_t * item) {
#if 0
	queue_item_t * kitem = queue -> head;

	if (queue -> head == item) queue -> head = item -> next;
	else {
		while (kitem -> next != item) kitem = kitem -> next;
		kitem -> next = item -> next;
		if (kitem -> next == NULL) queue -> tail = kitem;
	}

	queue -> status -= 1;
#else
    if (item -> que != queue) return;
    assert(queue -> status > 0);
    
    if (item -> prev) 
        item -> prev -> next = item -> next;
    else
        queue -> head = item -> next;

    if (item -> next)
        item -> next -> prev = item -> prev;
    else
        queue -> tail = item -> prev;

    queue -> status --;

    item -> prev = item -> next = NULL;
    item -> que = NULL;

#endif
}

static inline void atomic_queue_extract (queue_t * queue, queue_item_t * item) {
	lock_acquire(& queue -> lock);
	queue_extract (queue, item);
	lock_release(& queue -> lock);	
}

#if 0
/*---- Lookup function ----*/
static inline void * queue_lookup (queue_t * queue, bool (*inspector)(void *, void *), void * value) {
	queue_item_t * item = queue -> head;

	if (! queue -> status) return NULL;

	while (item != NULL) {
		if (inspector (item -> owner, value)) return item -> owner;
		item = item -> next;
	}

	return NULL;
}

static inline void * atomic_queue_lookup (queue_t * queue, bool (*inspector)(void *, void *), void * value) {
	void * owner = NULL;

	if (! queue -> status) return NULL;

	lock_acquire(& queue -> lock);
	owner = queue_lookup (queue, inspector, value);
	lock_release(& queue -> lock);	

	return owner;
}
#endif

/*---- Walk function ----*/
static inline void queue_walk (queue_t * queue, void (*inspector)(void *)) {
	queue_item_t * item = queue -> head;

	if (queue -> status) {;
		while (item != NULL) {
			inspector (item -> owner);
			item = item -> next;
		}
	}
}

static inline void atomic_queue_walk (queue_t * queue, void (*inspector)(void *)) {
	if (queue -> status) {;
		lock_acquire(& queue -> lock);
		queue_walk (queue, inspector);
		lock_release(& queue -> lock);	
	}
}

// -- general pointer inspector callback --
static bool general_inspector (void * p0, void * p1)
{
	return p0 == p1;
}

#endif // _TSC_SUPPORT_QUEUE_H_
