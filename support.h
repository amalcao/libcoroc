#ifndef _TSC_PLATFORM_SUPPORT_H_
#define _TSC_PLATFORM_SUPPORT_H_

#include <stdlib.h>
#include <pthread.h>

#define TSC_NAME_LENGTH 32

// -- for thread APIs -- 
typedef pthread_t TSC_OS_THREAD_T;

#define TSC_OS_THREAD_CREATE    pthread_create
#define TSC_OS_THREAD_SENDSIG   pthread_kill

// -- for thread local storages --
#define TSC_TLS_DEFINE \
   pthread_key_t __vpu_keys;

#define TSC_TLS_DECLARE \
    extern TSC_TLS_DEFINE

#define TSC_TLS_INIT() \
    pthread_key_create (& __vpu_keys, NULL)

#define TSC_TLS_SET(p) \
    pthread_setspecific (__vpu_keys, (p))

#define TSC_TLS_GET() \
    pthread_getspecific (__vpu_keys)

// -- for thread barrier --
#define TSC_BARRIER_DEFINE \
    static pthread_barrier_t __vpu_barrier;

#define TSC_BARRIER_INIT(n) \
    pthread_barrier_init (& __vpu_barrier, NULL, n)

#define TSC_BARRIER_WAIT() \
    pthread_barrier_wait (& __vpu_barrier);

// -- for memory alloc api --
#define TSC_ALLOC   malloc
#define TSC_DEALLOC free

// -- for signals --
#define TSC_CLOCK_SIGNAL    SIGUSR1

#define TSC_SIGNAL_MASK_DEFINE      \
    sigset_t __vpu_sigmask;

#define TSC_SIGNAL_MASK_DECLARE     \
    extern TSC_SIGNAL_MASK_DEFINE

#define TSC_SIGNAL_MASK_INIT()   do { \
    sigemptyset (&__vpu_sigmask);        \
    sigaddset (&__vpu_sigmask, SIGUSR1); } while (0)

#define TSC_SIGNAL_MASK()  \
    sigprocmask (SIG_BLOCK, NULL, &__vpu_sigmask)

#define TSC_SIGNAL_UNMASK() \
    sigprocmask (SIG_UNBLOCK, NULL, &__vpu_sigmask)


#endif // _TSC_PLATFORM_SUPPORT_H_
