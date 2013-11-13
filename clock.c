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

	   int index = 0;
	   for (; index < vpu_manager . xt_index; ++index) {
	   		TSC_OS_THREAD_SENDSIG (vpu_manager . vpu[index] . os_thr,
					TSC_CLOCK_SIGNAL);
	   }
    }
}
