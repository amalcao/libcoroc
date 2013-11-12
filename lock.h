#ifndef _TSC_SUPPORT_LOCK_H_
#define _TSC_SUPPORT_LOCK_H_

#include <pthread.h>

typedef pthread_mutex_t * lock_t;

static inline lock_t lock_allocate (void)
{
    lock_t lock = malloc (sizeof(pthread_mutex_t));
    pthread_mutex_init (lock, NULL);
    return lock;
}

static inline void lock_deallocate (lock_t lock)
{
    pthread_mutex_destroy(lock);
    free (lock);
}

#define lock_acquire pthread_mutex_lock
#define lock_release pthread_mutex_unlock

#endif // _TSC_SUPPORT_LOCK_H_
