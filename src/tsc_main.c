#include <stdlib.h>

#include "coroutine.h"

extern int user_main (void*);
extern int tsc_boot (int, char**, int, tsc_coroutine_handler_t);

int __tsc_main (int argc, char **argv)
{
    return tsc_boot (argc, argv, 0, (tsc_coroutine_handler_t)user_main);	
}

extern int main (int, char **) __attribute__((weak, alias ("__tsc_main")));
