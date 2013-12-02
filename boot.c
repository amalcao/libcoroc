#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "vpu.h"
#include "clock.h"
#include "thread.h"

extern int user_main (void*);
int __argc;
char ** __argv;

int main (int argc, char **argv)
{
	int n = TSC_NP_ONLINE();
	__argc = argc;
	__argv = argv;

    vpu_initialize (n);
    clock_initialize ();
    // -- TODO : more modules later .. --
    //
    thread_t init = thread_allocate ((thread_handler_t)user_main,
								NULL, "init", TSC_THREAD_MAIN, NULL);

    clock_routine(); // never return ..

    return 0;
}
