#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "vpu.h"
#include "clock.h"
#include "thread.h"

extern void vpu_initialize (int);
extern void clock_initialize (void);
extern void tsc_intertimer_initialize (void);
extern void tsc_vfs_initialize (void);
extern void tsc_netpoll_initialize (void);

int __argc;
char ** __argv;

thread_t init = NULL;

int tsc_boot (int argc, char **argv, int np, thread_handler_t entry)
{
	__argc = argc;
	__argv = argv;

    vpu_initialize (np);
    clock_initialize ();
    tsc_intertimer_initialize ();
    tsc_vfs_initialize ();
    tsc_netpoll_initialize ();

    // -- TODO : more modules later .. --
    //
    if (entry != NULL)
        init = thread_allocate (entry, NULL, "init", 
                    TSC_THREAD_MAIN, NULL);

    clock_routine(); // never return ..

    return 0;
}
