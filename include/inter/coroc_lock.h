// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_SUPPORT_LOCK_H_
#define _TSC_SUPPORT_LOCK_H_

#ifdef ENABLE_FUTEX
/* use a customized futex lock like Go */
#include "futex_lock.h"

typedef _coroc_futex_t coroc_lock;
typedef coroc_lock* lock_t;

#define lock_init _coroc_futex_init
#define lock_acquire _coroc_futex_lock
#define lock_try_acquire _coroc_futex_trylock
#define lock_release _coroc_futex_unlock
#define lock_fini _coroc_futex_destroy

#else

/* use pthread_spinlock */
#include <pthread.h>
#if defined(__APPLE__)
#include "darwin/pthread_spinlock.h"
#endif

typedef pthread_spinlock_t coroc_lock;
typedef coroc_lock* lock_t;

#define lock_init(lock) pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE)
#define lock_acquire pthread_spin_lock
#define lock_try_acquire pthread_spin_trylock
#define lock_release pthread_spin_unlock
#define lock_fini pthread_spin_destroy

#endif  // ENABLE_FUTEXT

#endif  // _TSC_SUPPORT_LOCK_H_
