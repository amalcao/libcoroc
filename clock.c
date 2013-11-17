#include <signal.h>
#include <stdint.h>
#include "clock.h"
#include "vpu.h"

#define TSC_CLOCK_PERIOD_NANOSEC    100000

clock_manager_t clock_manager;

void clock_initialize (void)
{
#ifdef ENABLE_TIMER
    sigset_t sigmask;
    struct sigaction act;

    sigemptyset (&sigmask);
    sigaddset (&sigmask, TSC_CLOCK_SIGNAL);

    act . sa_handler = vpu_clock_handler;
    act . sa_flags = SA_RESTART;

    sigaction (TSC_CLOCK_SIGNAL, &act, NULL);
#endif
}

void clock_routine (void)
{
    struct timespec period = {0, TSC_CLOCK_PERIOD_NANOSEC};
    sigset_t sigmask;
    sigfillset (&sigmask);

    while (true) {
        pselect (0, NULL, NULL, NULL, &period, &sigmask);

#ifdef ENABLE_TIMER
        int index = 0;
        for (; index < vpu_manager . xt_index; ++index) {
            TSC_OS_THREAD_SENDSIG (vpu_manager . vpu[index] . os_thr,
                    TSC_CLOCK_SIGNAL);
        }
#endif // ENABLE_TIMER
    }
}
