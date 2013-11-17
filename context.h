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
#	define mcontext libthread_mcontext
#	define mcontext_t libthread_mcontext_t
#	define ucontext libthread_ucontext
#	define ucontext_t libthread_ucontext_t
# if defined(__i386__)
#	include "386-ucontext.h"
# elif defined(__x86_64__)
#	include "amd64-ucontext.h"
# else
#	include "power-ucontext.h"
# endif 
#else
#define USE_UCONTEXT 1  // default for the linux platform
#endif // __APPLE__

#if USE_UCONTEXT
#include <ucontext.h>
#endif // USE_UCONTEXT

typedef ucontext_t TSC_CONTEXT;

#define TSC_CONTEXT_LOAD setcontext
#define TSC_CONTEXT_SAVE getcontext
#define TSC_CONTEXT_SWAP swapcontext
#define TSC_CONTEXT_MAKE makecontext

extern void TSC_CONTEXT_INIT (TSC_CONTEXT * ctx, void *stack, size_t stack_sz, void * thread);


#endif // _TSC_PLATFORM_CONTEXT_H
