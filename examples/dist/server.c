#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "libtsc.h"
#include "libtsc_dist.h"

int helloworld_service_run (tsc_service_t *service, tsc_message_t message) {
    assert (message && message->len > 0);

    if (message->len == 1 && message->buf[0] == 'Q') {
        fprintf(stderr, "Service receive the exiting signal!\n");
        tsc_service_quit(service, 0);
    } else {
        fprintf(stderr, "The content of the message is : %s \n", message->buf);
    }
    return 0;
}

/* define the hello world service .. */
TSC_SERVICE_DECLARE_TCP(helloworld_service, 
    "Hello World", TSC_SERVICE_INFINITY, false, 65344, 
    helloworld_service_run, NULL, NULL, NULL);

int main (int argc, char **argv) {
    // must be called before do anything ..
    tsc_init_all_services();
    // blocking until the service quit ..
    tsc_service_start(&helloworld_service, false); 
    return 0;
}
