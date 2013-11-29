#ifndef _TSC_SUPPORT_LOCK_H_
#define _TSC_SUPPORT_LOCK_H_

#include <pthread.h>
#if defined(__APPLE__)
#include "pthread_spinlock.h"
#endif

typedef pthread_spinlock_t lock;
typedef lock * lock_t;

#define lock_init(lock) pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE)
#define lock_acquire(lock) pthread_spin_lock(lock)
#define lock_release(lock) pthread_spin_unlock(lock)
#define lock_finit(lock) pthread_spin_destroy(lock)

#endif // _TSC_SUPPORT_LOCK_H_
