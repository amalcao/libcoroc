#ifndef _TSC_PLATFORM_CONTEXT_H
#define _TSC_PLATFORM_CONTEXT_H

#define USE_UCONTEXT 1  // default for the linux platform

#if USE_UCONTEXT

#include <stdlib.h>
#include <ucontext.h>

typedef ucontext_t TSC_CONTEXT;

#define TSC_CONTEXT_LOAD setcontext
#define TSC_CONTEXT_SAVE getcontext
#define TSC_CONTEXT_SWAP swapcontext
#define TSC_CONTEXT_MAKE makecontext

extern void TSC_CONTEXT_INIT (TSC_CONTEXT * ctx, void *stack, size_t stack_sz, void * thread);


#endif

#endif // _TSC_PLATFORM_CONTEXT_H
