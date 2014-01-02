#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include "support.h"
#include "context.h"
#include "thread.h"

extern int __argc;
extern char ** __argv;

typedef int (* main_entry_t) (int, char **);

static void bootstrap (uint32_t low, uint32_t high)
{
    uint64_t tmp = high << 16;
    tmp <<= 16;
    tmp |= low;

    thread_t thread = (thread_t)tmp;
	if (thread -> type == TSC_THREAD_MAIN)
		((main_entry_t)(thread -> entry)) (__argc, __argv);
	else
    	thread -> entry (thread -> arguments);
	thread_exit (0);
}

void TSC_CONTEXT_INIT (TSC_CONTEXT * ctx, void *stack, size_t stack_sz,
 void * thread)
{
    uint32_t low, high;
    uint64_t tmp = (uint64_t)(thread);
    sigset_t mask;

    low = tmp;
    tmp >>= 16;
    high = tmp >> 16;

    sigemptyset (& mask);

    memset (ctx, 0, sizeof(TSC_CONTEXT));
    TSC_CONTEXT_SAVE (ctx);

#ifdef ENABLE_SPLITSTACK
    (ctx -> ctx) . uc_stack . ss_sp = stack;
    (ctx -> ctx) . uc_stack . ss_size = stack_sz;
    (ctx -> ctx) . uc_sigmask = mask;
#else
	ctx -> uc_stack . ss_sp = stack;
	ctx -> uc_stack . ss_size = stack_sz;
	ctx -> uc_sigmask = mask;
#endif

	TSC_CONTEXT_MAKE (ctx, bootstrap, 2, low, high);
}
