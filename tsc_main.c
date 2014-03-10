#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "vpu.h"
#include "clock.h"
#include "thread.h"

extern int user_main (void*);
extern int tsc_boot (int, char**, int, thread_handler_t);

int main (int argc, char **argv)
{
	int n = 0;
    char *endp = NULL;
    const char *env = getenv("TSC_NP");

    if (env != NULL)
        n = strtol (env, &endp, 0);
    if (n <= 0 || endp == env)
        n = TSC_NP_ONLINE();

    return tsc_boot (argc, argv, n, (thread_handler_t)user_main);	
}
