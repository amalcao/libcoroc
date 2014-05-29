#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "libtsc.h"
#include "libtsc_dist.h"

#include "service.h"

static tsc_service_id_t pong = ID_SERVICE_PONG;

int ping_service_run(tsc_service_t service, tsc_message_t message) {
  assert(message && message->len == sizeof(ppmsg));
  ppmsg *pm = (ppmsg *)(message->content);

  fprintf(stderr, "Str : %s , Count : %d \n", pm->str, pm->count);
  tsc_message_t replay = tsc_message_alloc(service->id, sizeof(ppmsg));
  ppmsg *pm1 = (ppmsg *)(replay->content);
  pm1->count = pm->count + 1;
  strcpy(pm1->str, "PING");

  tsc_message_send(replay, message->src);

  if (pm1->count >= 10) {
    // disconnect ..
    tsc_service_disconnect(service, pong);
    // wait for the messages dispatched ..
    int i;
    for (i = 0; i < 10; i++) tsc_coroutine_yield();
    fprintf(stderr, "Service exiting!\n");
    tsc_service_quit(service, 0);
  }
  return 0;
}

int ping_service_init(tsc_service_t service, void *dummy) {
  assert(service != NULL);

  // connect to the pong service ..
  uint32_t ip;
  int port = PORT_SERVICE_PONG;

  tsc_net_lookup(HOST_SERVICE_PONG, &ip);

  pong |= (((uint64_t)port) << 16);
  pong |= (((uint64_t)ip) << 32);

  tsc_service_connect(service, pong);

  // send the first message
  tsc_message_t msg = tsc_message_alloc(service->id, sizeof(ppmsg));
  ppmsg *pm = (ppmsg *)(msg->content);
  strcpy(pm->str, "PING");
  pm->count = 0;

  tsc_message_send(msg, pong);
  return 0;
}

/* define the PING service .. */
TSC_SERVICE_DECLARE_LOCAL(ping_service, ID_SERVICE_PING, "Ping",
                          TSC_SERVICE_INFINITY, false, ping_service_run,
                          ping_service_init, NULL, NULL);

int main(int argc, char **argv) {
  // must be called before do anything ..
  tsc_init_all_services();
  // blocking until the service quit ..
  tsc_service_start(&ping_service, false);
  // never return here until service quit ..
  sleep(1);
  return 0;
}
