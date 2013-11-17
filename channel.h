#ifndef _TSC_CORE_CHANNEL_H_
#define _TSC_CORE_CHANNEL_H_

#include <stdint.h>
#include <stdlib.h>
#include "support.h"
#include "queue.h"
#include "thread.h"

typedef int status_t;

enum {
    TSC_MSG_SOFT = 1,
    TSC_MSG_HARD = 2,
};

typedef struct message {

    uint32_t type;  // TODO : point-to-point or broadcast
    thread_t send_tid;
    thread_t recv_tid;

    size_t size;
    void * buff;

    queue_item_t message_link;
    
} * message_t; 

message_t message_allocate (size_t size, void * buff);
status_t message_send (message_t msg, thread_t to);
status_t message_recv (message_t * msg, thread_t from, bool block);
message_t message_clone (message_t msg);
void message_deallocate (message_t msg);

// -- public API --
status_t send (thread_t to, size_t size, void * buff);
status_t recv (thread_t from, size_t * size, void * buff, bool block);

#endif // _TSC_CORE_CHANNEL_H_
