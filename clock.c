#include <signal.h>
#include <stdint.h>
#include "clock.h"
#include "vpu.h"

clock_manager_t clock_manager;

void clock_initialize (void)
{
    sigset_t sigmask;
    struct sigaction act;

    sigemptyset (&sigmask);
    sigaddset (&sigmask, TSC_CLOCK_SIGNAL);

    act . sa_handler = vpu_clock_handler;
    act . sa_flags = SA_RESTART;

    sigaction (TSC_CLOCK_SIGNAL, &act, NULL);
}

void clock_routine (void)
{
    while (true) {
       struct timespec period = {0, 100000};
       nanosleep (& period, NULL);
       vpu_sendsig (TSC_CLOCK_SIGNAL); 
    }
}
