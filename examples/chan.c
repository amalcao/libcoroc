#include <stdlib.h>
#include <stdio.h>

#include "libtsc.h"

int subtask (tsc_chan_t chan)
{
    int ret, id;
    while ((ret = tsc_chan_recv (chan, &id)) != CHAN_CLOSED) {
        printf ("[subtask:%x] recv id is %d.\n", ret, id);
    }

    tsc_refcnt_put ((tsc_refcnt_t)chan);
    tsc_coroutine_exit (0);
}

int main (int argc, char ** argv)
{
  uint64_t awaken = 0;
  int i = 0;
  tsc_chan_t chan = (tsc_chan_t)tsc_refcnt_get(
        tsc_chan_allocate (sizeof(int), 0) );

  tsc_timer_t timer = tsc_timer_allocate (1000000 * 1, NULL);
  tsc_timer_after (timer, 1000000 * 1); // 1 seconds later

  tsc_coroutine_allocate (subtask, tsc_refcnt_get(chan), "sub", TSC_COROUTINE_NORMAL, 0);
  for (i = 0; i < 10; i++) {
      printf ("waiting for 1 seconds!\n");
      tsc_chan_recv((tsc_chan_t)timer, &awaken);
      tsc_chan_send(chan, &i);
  }

  printf ("release the timer ..\n");
  tsc_chan_close (chan);
  tsc_refcnt_put ((tsc_refcnt_t)chan);

  tsc_timer_stop (timer);
  tsc_timer_dealloc (timer);

  tsc_coroutine_exit (0);
}
