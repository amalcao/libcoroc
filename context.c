#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include "support.h"
#include "context.h"
#include "thread.h"

static void bootstrap (uint32_t low, uint32_t high)
{
    uint64_t tmp = high << 16;
    tmp <<= 16;
    tmp |= low;

    thread_t thread = (thread_t)tmp;
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

	ctx -> uc_stack . ss_sp = stack;
	ctx -> uc_stack . ss_size = stack_sz;
	ctx -> uc_sigmask = mask;

	TSC_CONTEXT_MAKE (ctx, bootstrap, 2, low, high);
}
