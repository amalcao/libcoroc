#ifndef _TSC_PLATFORM_CONTEXT_H
#define _TSC_PLATFORM_CONTEXT_H

#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_5)
#undef USE_UCONTEXT
#define USE_UCONTEXT 0
#endif
#endif

#if defined(__APPLE__)
#define mcontext libthread_mcontext
#define mcontext_t libthread_mcontext_t
#define ucontext libthread_ucontext
#define ucontext_t libthread_ucontext_t
#if defined(__i386__)
#include "darwin/386-ucontext.h"
#elif defined(__x86_64__)
#include "darwin/amd64-ucontext.h"
#else
#include "darwin/power-ucontext.h"
#endif
#else
#define USE_UCONTEXT 1  // default for the linux platform
#endif                  // __APPLE__

#if USE_UCONTEXT
#include <ucontext.h>
#endif  // USE_UCONTEXT

#if defined(ENABLE_SPLITSTACK)
typedef struct {
  ucontext_t ctx;
  void* stack_ctx[10];
} TSC_CONTEXT;

extern void* __splitstack_makecontext(size_t stack_size, void* context[10],
                                      size_t* size);
extern void __splitstack_setcontext(void* context[10]);
extern void __splitstack_getcontext(void* context[10]);

#define TSC_CONTEXT_LOAD(cp) setcontext(&(cp)->ctx)
#define TSC_CONTEXT_SAVE(cp) getcontext(&(cp)->ctx)
#define TSC_CONTEXT_MAKE(cp, ...) makecontext(&(cp)->ctx, __VA_ARGS__)

#define TSC_STACK_CONTEXT_MAKE(sz, cp, ...) \
  __splitstack_makecontext(sz, &(cp)->stack_ctx[0], __VA_ARGS__)
#define TSC_STACK_CONTEXT_LOAD(cp) __splitstack_setcontext(&(cp)->stack_ctx[0])
#define TSC_STACK_CONTEXT_SAVE(cp) __splitstack_getcontext(&(cp)->stack_ctx[0])

#define TSC_CONTEXT_SIGADDMASK(cp, sig) sigaddset(&(cp)->ctx.uc_sigmask, sig)

#else
typedef ucontext_t TSC_CONTEXT;

#define TSC_CONTEXT_LOAD setcontext
#define TSC_CONTEXT_SAVE getcontext
#define TSC_CONTEXT_SWAP swapcontext
#define TSC_CONTEXT_MAKE makecontext

#define TSC_CONTEXT_SIGADDMASK(cp, sig) sigaddset(&(cp)->uc_sigmask, sig)

#endif

extern void TSC_CONTEXT_INIT(TSC_CONTEXT* ctx, void* stack, size_t stack_sz,
                             void* thread);

#endif  // _TSC_PLATFORM_CONTEXT_H
