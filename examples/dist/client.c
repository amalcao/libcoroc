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
  struct tsc_message message;

  tsc_chan_t conn = tsc_service_connect_by_host("localhost", 65344);
  assert(conn != NULL);

  for (i = 0; i < COUNT; ++i) {
    snprintf(string, 20, "hello world! %d", i);
    message.len = strlen(string) + 1;
    message.buf = string;
    tsc_chan_send(conn, &message);
    sleep(1);
  }

  message.len = 1;
  message.buf = "Q";
  tsc_chan_send(conn, &message);

  tsc_service_disconnect(conn);
  return 0;
}
