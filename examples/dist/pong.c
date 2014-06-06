#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "libtsc.h"
#include "libtsc_dist.h"

#include "service.h"

int pong_service_run(tsc_service_t service, tsc_message_t message) {
  assert(message && message->len == sizeof(ppmsg));
  ppmsg *pm = (ppmsg *)(message->content);

  fprintf(stderr, "Str : %s , Count : %d \n", pm->str, pm->count);
  if (pm->count < 10) {
      tsc_message_t replay = tsc_message_alloc(service->id, sizeof(ppmsg));
      ppmsg *pm1 = (ppmsg *)(replay->content);
      pm1->count = pm->count + 1;
      strcpy(pm1->str, "PONG");
      tsc_message_send(replay, message->src);
  } else {
    tsc_service_disconnect(service->id, message->src);
  }
 
  return 0;
}

/* define the PING service .. */
TSC_SERVICE_DECLARE_TCP(pong_service, ID_SERVICE_PONG, "Pong",
                        TSC_SERVICE_INFINITY, false, HOST_SERVICE_PONG,
                        PORT_SERVICE_PONG, pong_service_run, NULL, NULL, NULL);

int main(int argc, char **argv) {
    // must be called before do anything ..
    tsc_init_all_services();
    // blocking until the service quit ..
    tsc_service_start(&pong_service, false);
    // never return here until service quit ..
    return 0;
}
