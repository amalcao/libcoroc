#ifndef _FUTEX_LOCK_H_
#define _FUTEX_LOCK_H_

#include <stdint.h>

#include "support.h"

/* The futex based lock mechanism,
 * we've imitated the golang's implementation.
 * See [http://golang.org] to get the details .. */

enum {
  MUTEX_UNLOCKED = 0,
  MUTEX_LOCKED = 1,
  MUTEX_SLEEPING = 2,
  ACTIVE_SPIN = 4,
  ACTIVE_SPIN_CNT = 30,
  PASSIVE_SPIN = 1,
};

typedef volatile struct {
  uint32_t key;
#if (ENABLE_DEBUG > 1)
  uint64_t cookie;
#endif
} _tsc_futex_t;

extern void _tsc_futex_init(volatile _tsc_futex_t *lock);
extern void _tsc_futex_lock(volatile _tsc_futex_t *lock);
extern void _tsc_futex_unlock(volatile _tsc_futex_t *lock);
extern int _tsc_futex_trylock(volatile _tsc_futex_t *lock);
extern void _tsc_futex_destroy(volatile _tsc_futex_t *lock);

#endif  // _FUTEX_LOCK_H_
