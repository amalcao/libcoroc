#ifndef _TSC_SUPPORT_LOCK_H_
#define _TSC_SUPPORT_LOCK_H_

#include <pthread.h>
#if defined(__APPLE__)
#include "pthread_spinlock.h"
#endif

typedef pthread_spinlock_t * lock_t;

static inline lock_t lock_allocate (void)
{
    lock_t lock = malloc (sizeof(pthread_spinlock_t));
    pthread_spin_init (lock, PTHREAD_PROCESS_PRIVATE);
    return lock;
}

static inline void lock_deallocate (lock_t lock)
{
    pthread_spin_destroy(lock);
    free (lock);
}

#define lock_acquire pthread_spin_lock
#define lock_release pthread_spin_unlock

#endif // _TSC_SUPPORT_LOCK_H_
