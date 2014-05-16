#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "libtsc.h"
#include "libtsc_dist.h"

#define COUNT 2

int main(int argc, char **argv) {
  int i;
  char string[20];
  tsc_message_t message;

  tsc_chan_t conn = tsc_service_connect_by_host("localhost", 65344);
  assert(conn != NULL);

  for (i = 0; i < COUNT; ++i) {
    snprintf(string, 20, "hello world! %d", i);
    message = tsc_message_alloc(strlen(string) + 1);
    strncpy(message->buf, string, message->len);
    tsc_chan_send(conn, &message);
    sleep(1);
  }

  message = tsc_message_alloc(1);
  message->buf[0] = 'Q';
  tsc_chan_send(conn, &message);

  tsc_service_disconnect(conn);
  return 0;
}
