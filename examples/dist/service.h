#ifndef __TSC_EXAMPLE_DIST_SERVICE_H__
#define __TSC_EXAMPLE_DIST_SERVICE_H__

// nothing but define the global service id table ..
enum { ID_SERVICE_PING = 1, ID_SERVICE_PONG, };

// define the port ..
#define PORT_SERVICE_PONG 65344
#define HOST_SERVICE_PONG "localhost"

// define the ping/pong message type
typedef struct {
  int count;
  char str[5];
} ppmsg;

#endif  // __TSC_EXAMPLE_DIST_SERVICE_H__
