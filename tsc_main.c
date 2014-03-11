#include <stdlib.h>

#include "thread.h"

extern int user_main (void*);
extern int tsc_boot (int, char**, int, thread_handler_t);

int __tsc_main (int argc, char **argv)
{
    return tsc_boot (argc, argv, 0, (thread_handler_t)user_main);	
}

extern int main (int, char **) __attribute__((weak, alias ("__tsc_main")));
