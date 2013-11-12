#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "vpu.h"
#include "clock.h"
#include "thread.h"

extern int user_main (void*);

int main (int argc, char **argv)
{
    int n = 4; // default ..
    if (argc > 1) {
        n = atoi (argv[1]);
    }

    assert (n > 0);

    vpu_initialize (n);
    clock_initialize ();
    // -- TODO : more modules later .. --
    //
    thread_t init = thread_allocate (user_main, NULL, "init", TSC_THREAD_MAIN, NULL);

    clock_routine(); // never return ..

    return 0;
}
