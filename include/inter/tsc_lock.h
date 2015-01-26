#ifndef _TSC_SUPPORT_LOCK_H_
#define _TSC_SUPPORT_LOCK_H_

#ifdef ENABLE_FUTEXT
/* use a customized futex lock like Go */
#include "futex_lock.h"

typedef _tsc_futex_t tsc_lock;
typedef tsc_lock* lock_t;

#define lock_init _tsc_futex_init
#define lock_acquire _tsc_futex_lock
#define lock_try_acquire _tsc_futex_trylock
#define lock_release _tsc_futex_unlock
#define lock_fini _tsc_futex_destroy

#else

/* use pthread_spinlock */
#include <pthread.h>
#if defined(__APPLE__)
#include "darwin/pthread_spinlock.h"
#endif

typedef pthread_spinlock_t tsc_lock;
typedef tsc_lock* lock_t;

#define lock_init(lock) pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE)
#define lock_acquire pthread_spin_lock
#define lock_try_acquire pthread_spin_trylock
#define lock_release pthread_spin_unlock
#define lock_fini pthread_spin_destroy

#endif  // ENABLE_FUTEXT

#endif  // _TSC_SUPPORT_LOCK_H_
