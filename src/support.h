#ifndef _TSC_PLATFORM_SUPPORT_H_
#define _TSC_PLATFORM_SUPPORT_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __APPLE__

#include "pthread_barrier.h"

#define _XOPEN_SOURCE 700

#endif  // __APPLE__

#define TSC_NAME_LENGTH 32

// -- for thread APIs --
typedef pthread_t TSC_OS_THREAD_T;

#define TSC_OS_THREAD_ATTR pthread_attr_t
#define TSC_OS_THREAD_ATTR_INIT pthread_attr_init
#define TSC_OS_THREAD_ATTR_SETSTACKSZ pthread_attr_setstacksize

#define TSC_OS_THREAD_CREATE pthread_create
#define TSC_OS_THREAD_SENDSIG pthread_kill

// -- for thread local storages --
#if USE_PTHREAD_TLS
#define TSC_TLS_DEFINE pthread_key_t __vpu_keys;

#define TSC_TLS_DECLARE extern TSC_TLS_DEFINE

#define TSC_TLS_INIT() pthread_key_create(&__vpu_keys, NULL)

#define TSC_TLS_SET(p) pthread_setspecific(__vpu_keys, (p))

#define TSC_TLS_GET() pthread_getspecific(__vpu_keys)

#else  // USE_GCC_TLS
#define TSC_TLS_DEFINE __thread vpu_t* __vpu;

#define TSC_TLS_DECLARE extern TSC_TLS_DEFINE

#define TSC_TLS_INIT() \
  do {                 \
  } while (0)

#define TSC_TLS_SET(p) __vpu = (vpu_t*)(p)

#define TSC_TLS_GET() (__vpu)
#endif

// -- for thread barrier --
#define TSC_BARRIER_DEFINE static pthread_barrier_t __vpu_barrier;

#define TSC_BARRIER_INIT(n) pthread_barrier_init(&__vpu_barrier, NULL, n)

#define TSC_BARRIER_WAIT() pthread_barrier_wait(&__vpu_barrier);

#if !defined(PTHREAD_STACK_MIN)
#define PTHREAD_STACK_MIN (16 * 1024)  // 16KB
#endif

// -- for memory alloc api --
#define TSC_ALLOC malloc
#define TSC_REALLOC realloc
#define TSC_DEALLOC free

// -- for signals --
#define TSC_CLOCK_SIGNAL SIGUSR1

#ifdef ENABLE_TIMER

#define TSC_SIGNAL_MASK_DEFINE \
  sigset_t __vpu_sigmask;      \
  __thread int __vpu_sigmask_nest = 0;

#define TSC_SIGNAL_MASK_DECLARE  \
  extern sigset_t __vpu_sigmask; \
  extern __thread int __vpu_sigmask_nest;

#define TSC_SIGNAL_MASK_INIT()          \
  do {                                  \
    sigemptyset(&__vpu_sigmask);        \
    sigaddset(&__vpu_sigmask, SIGUSR1); \
  } while (0)

#define TSC_SIGNAL_MASK()                           \
  do {                                              \
    if (__vpu_sigmask_nest++ == 0)                  \
      sigprocmask(SIG_BLOCK, &__vpu_sigmask, NULL); \
  } while (0)

#define TSC_SIGNAL_UNMASK()                           \
  do {                                                \
    if (--__vpu_sigmask_nest == 0)                    \
      sigprocmask(SIG_UNBLOCK, &__vpu_sigmask, NULL); \
  } while (0)

#define TSC_SIGNAL_STATE_SAVE(p) *(p) = __vpu_sigmask_nest

#if defined(__APPLE__)
#define TSC_SIGNAL_STATE_LOAD(p)                                   \
  do {                                                             \
    __vpu_sigmask_nest = *(p);                                     \
    if (*(p) == 0) sigprocmask(SIG_UNBLOCK, &__vpu_sigmask, NULL); \
  } while (0)
#else
#define TSC_SIGNAL_STATE_LOAD(p) __vpu_sigmask_nest = *(p)
#endif

#else
#define TSC_SIGNAL_MASK_DEFINE
#define TSC_SIGNAL_MASK_DECLARE
#define TSC_SIGNAL_MASK_INIT()
#define TSC_SIGNAL_MASK()
#define TSC_SIGNAL_UNMASK()
#define TSC_SIGNAL_STATE_SAVE(p)
#define TSC_SIGNAL_STATE_LOAD(p)
#endif

#define TSC_RESCHED_THRESHOLD 5

// -- for atomic add op --
#if defined(__APPLE__) && !defined(__i386__) && !defined(__x86_64__)
#define TSC_ATOMIC_INC(n) (++(n))
#define TSC_ATOMIC_DEC(n) (--(n))
#define TSC_SYNC_ALL()
// TODO # define TSC_CAS(pval, old, new)
#else
#define TSC_ATOMIC_INC(n) __sync_add_and_fetch(&(n), 1)
#define TSC_ATOMIC_DEC(n) __sync_add_and_fetch(&(n), -1)

#define TSC_ATOMIC_READ(n) __atomic_load_n(&(n), __ATOMIC_SEQ_CST)
#define TSC_ATOMIC_WRITE(n, v) __atomic_store_n(&(n), v, __ATOMIC_SEQ_CST)

#define TSC_CAS(pval, old, new) __sync_bool_compare_and_swap(pval, old, new)
#define TSC_XCHG(pval, new) __atomic_exchange_n(pval, new, __ATOMIC_SEQ_CST)

#define TSC_SYNC_ALL() __sync_synchronize()
#endif

#if defined(__GNUC__) && (__GNUC__ < 4 || __GNUC_MINOR__ < 7)
#undef TSC_SYNC_ALL
#undef TSC_XCHG
#undef TSC_ATOMIC_READ
#undef TSC_ATOMIC_WRITE

#define TSC_SYNC_ALL() __asm__ __volatile__("" :: : "memory")

#define TSC_XCHG(pval, new)                 \
  ({                                        \
    uint32_t __old;                         \
    for (;;) {                              \
      __old = *pval;                        \
      if (TSC_CAS(pval, __old, new)) break; \
    }                                       \
    __old;                                  \
  })

#define TSC_ATOMIC_READ(n) __sync_add_and_fetch(&(n), 0)
#define TSC_ATOMIC_WRITE(n, v)           \
  ({                                     \
    volatile uint32_t* __pval = &(n);    \
    for (;;) {                           \
      uint32_t __old = *__pval;          \
      if (TSC_CAS(__pval, __old)) break; \
    }                                    \
  })

#endif

#ifdef USE_FUTEX_LOCK
extern void _tsc_futex_sleep(uint32_t*, uint32_t, int64_t);
extern void _tsc_futex_wakeup(uint32_t*, uint32_t);
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
static int inline TSC_NP_ONLINE(void) {
  int num_cpu;
  int mib[4];
  size_t len = sizeof(num_cpu);

  /* set the mib for hw.ncpu */
  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;

  /* get the number of CPUs from the system */
  sysctl(mib, 2, &num_cpu, &len, NULL, 0);
  if (num_cpu <= 0) return 1;
  return num_cpu;
}
#else  // linux platforms ..
#include <sys/sysinfo.h>
#define TSC_NP_ONLINE() get_nprocs()
#endif

/* define the tsc_word_t which match the arch word size */
#if __x86_64__  // the only 64-bit arch we support now is x86-64
typedef long long tsc_word_t;
#else  // other such as i386, armv7 ..
typedef long tsc_word_t;
#endif

static inline void __procyield(uint32_t cnt) {
  volatile uint32_t i;

  for (i = 0; i < cnt; ++i) {
#if defined(__i386) || defined(__x86_64__)
    __asm__ __volatile__("pause\n" ::);
#endif
  }
}

/* define the debug output interface */
#if (ENABLE_DEBUG > 1)
#define TSC_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define TSC_DEBUG(...)
#endif  // ENABLE_DEBUG

#endif  // _TSC_PLATFORM_SUPPORT_H_
