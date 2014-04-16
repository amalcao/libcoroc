#ifndef _TSC_SUPPORT_LOCK_H_
#define _TSC_SUPPORT_LOCK_H_

#include <pthread.h>
#if defined(__APPLE__)
#include "pthread_spinlock.h"
#endif

typedef pthread_spinlock_t lock;
typedef lock * lock_t;

#define lock_init(lock) pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE)
#define lock_acquire pthread_spin_lock
#define lock_try_acquire pthread_spin_trylock
#define lock_release pthread_spin_unlock
#define lock_fini pthread_spin_destroy

#endif // _TSC_SUPPORT_LOCK_H_
