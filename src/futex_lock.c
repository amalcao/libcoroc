#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "coroutine.h"

static inline void __procyield (uint32_t cnt)
{
  volatile uint32_t i;

  for (i = 0; i < cnt; ++i) {
#if defined(__i386) || defined(__x86_64__)
      __asm__ __volatile__ ("pause\n":::);
#endif
  }
}

void _tsc_futex_init (volatile _tsc_futex_t *lock)
{
  lock -> key = MUTEX_UNLOCKED;
}

void _tsc_futex_lock (volatile _tsc_futex_t *lock)
{
  uint32_t i, v, wait, spin;
#ifdef ENABLE_DEBUG
  tsc_coroutine_t self = tsc_coroutine_self();
  uint64_t coid = (self != NULL) ? self -> id : (uint64_t)-1;
#endif

  v = TSC_XCHG(& lock->key, MUTEX_LOCKED);
  if (v == MUTEX_UNLOCKED)
    goto __lock_success;

  wait = v;
  spin = ACTIVE_SPIN;

  while (1) {
      for (i = 0; i < spin; i++) {
          while (lock -> key == MUTEX_UNLOCKED)
            if (TSC_CAS(& lock->key, MUTEX_UNLOCKED, wait))
              goto __lock_success;

          __procyield(ACTIVE_SPIN_CNT);
      }

      for (i = 0; i < PASSIVE_SPIN; i++) {
          while (lock -> key == MUTEX_UNLOCKED)
            if (TSC_CAS(& lock->key, MUTEX_UNLOCKED, wait))
              goto __lock_success;
          sched_yield ();
      }

      // sleep 
      v = TSC_XCHG(& lock->key, MUTEX_SLEEPING);
      if (v == MUTEX_UNLOCKED)
        goto __lock_success;
      wait = MUTEX_SLEEPING;
      _tsc_futex_sleep ((uint32_t*)& lock->key, MUTEX_SLEEPING, -1);
  }

__lock_success:
#ifdef ENABLE_DEBUG
  lock -> cookie = coid;
#endif
  return;
}

void _tsc_futex_unlock (volatile _tsc_futex_t *lock)
{
  uint32_t v;

#ifdef ENABLE_DEBUG
  lock -> cookie = (uint64_t)-2;
#endif

  v = TSC_XCHG(& lock->key, MUTEX_UNLOCKED);
  assert (v != MUTEX_UNLOCKED);

  if (v == MUTEX_SLEEPING)
    _tsc_futex_wakeup ((uint32_t*)& lock->key, 1);
}

int _tsc_futex_trylock (volatile _tsc_futex_t *lock)
{
  if (TSC_CAS(& lock->key, MUTEX_UNLOCKED, MUTEX_LOCKED))
    return 0;
  return 1;
}

void _tsc_futex_destroy (volatile _tsc_futex_t *lock)
{
  // TODO
  assert (lock->key == MUTEX_UNLOCKED);
}
