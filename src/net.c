#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "support.h"
#include "netpoll.h"
#include "coroutine.h"

// This is the NET API wrapper for libtsc,
// copy & modify from the LibTask project.

int tsc_net_lookup(const char *name, uint32_t *ip);

int tsc_net_announce(bool istcp, const char *server, int port) {
  int fd, n, proto;
  struct sockaddr_in sa;
  socklen_t sn;
  uint32_t ip;

  proto = istcp ? SOCK_STREAM : SOCK_DGRAM;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;

  if (server != NULL && strcmp(server, "*") != 0) {
    if (tsc_net_lookup(server, &ip) < 0) return -1;
    memmove(&sa.sin_addr, &ip, 4);
  }

  sa.sin_port = htons(port);
  if ((fd = socket(AF_INET, proto, 0)) < 0) return -1;

  // set reuse flag for tcp ..
  if (istcp && getsockopt(fd, SOL_SOCKET, SO_TYPE, (void *)&n, &sn) >= 0) {
    n = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n));
  }

  if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    close(fd);
    return -1;
  }

  if (proto == SOCK_STREAM) listen(fd, port);

  tsc_net_nonblock(fd);
  return fd;
}

int tsc_net_timed_accept(int fd, char *server, int *port, int64_t usec) {
  int cfd, one;
  struct sockaddr_in sa;
  uint8_t *ip;
  socklen_t len;

  if (usec <= 0)
    tsc_net_wait(fd, TSC_NETPOLL_READ);
  else if (!tsc_net_timedwait(fd, TSC_NETPOLL_READ, usec))
    return -1;

  len = sizeof sa;
  if ((cfd = accept(fd, (void *)&sa, &len)) < 0) return -1;

  if (server) {
    ip = (uint8_t *)&sa.sin_addr;
    snprintf(server, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  }

  if (port) *port = ntohs(sa.sin_port);

  tsc_net_nonblock(cfd);
  one = 1;
  setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof one);

  return cfd;
}

int tsc_net_accept(int fd, char *server, int *port) {
    return tsc_net_timed_accept(fd, server, port, 0);
}

#define CLASS(p) ((*(unsigned char *)(p)) >> 6)
static int __tsc_parseip(const char *name, uint32_t *ip) {
  uint8_t addr[4];
  char *p;
  int i, x;

  p = (char *)name;
  for (i = 0; i < 4 && *p; i++) {
    x = strtoul(p, &p, 0);
    if (x < 0 || x > 256) return -1;
    if (*p != '.' && *p != 0) return -1;
    if (*p == '.') p++;
    addr[i] = x;
  }

  switch (CLASS(addr)) {
    case 0:
    case 1:
      if (i == 3) {
        addr[3] = addr[2];
        addr[2] = addr[1];
        addr[1] = 0;
      } else if (i == 2) {
        addr[3] = addr[1];
        addr[2] = 0;
        addr[1] = 0;
      } else if (i != 4)
        return -1;
      break;

    case 2:
      if (i == 3) {
        addr[3] = addr[2];
        addr[2] = 0;
      } else if (i != 4)
        return -1;
      break;
  }

  *ip = *(uint32_t *)addr;
  return 0;
}

int tsc_net_lookup(const char *name, uint32_t *ip) {
  struct hostent *he;

  if (__tsc_parseip(name, ip) >= 0) return 0;

  if ((he = gethostbyname(name)) != 0) {
    *ip = *(uint32_t *)he->h_addr;
    return 0;
  }

  return -1;
}

int tsc_net_timed_dial(bool istcp, char *server, int port, int64_t usec) {
  int proto, fd, n;
  uint32_t ip;
  struct sockaddr_in sa;
  socklen_t sn;

  if (tsc_net_lookup(server, &ip) < 0) return -1;

  proto = istcp ? SOCK_STREAM : SOCK_DGRAM;
  if ((fd = socket(AF_INET, proto, 0)) < 0) return -1;

  tsc_net_nonblock(fd);

  if (!istcp) {
    n = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &n, sizeof n);
  }

  /* start connecting */
  memset(&sa, 0, sizeof sa);
  memmove(&sa.sin_addr, &ip, 4);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);

  if (connect(fd, (struct sockaddr *)&sa, sizeof sa) < 0 &&
      errno != EINPROGRESS) {
    close(fd);
    return -1;
  }

  // wait for the connection finish ..
  if (usec <= 0) {
    tsc_net_wait(fd, TSC_NETPOLL_WRITE);
  } else if (!tsc_net_timedwait(fd, TSC_NETPOLL_WRITE, usec)) {
    close(fd);
    return -1;
  }

  sn = sizeof sa;
  if (getpeername(fd, (struct sockaddr *)&sa, &sn) >= 0) return fd;

  // report the error
  sn = sizeof n;
  getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&n, &sn);
  if (n == 0) n = ECONNREFUSED;
  close(fd);
  errno = n;

  return -1;
}

int tsc_net_dial(bool istcp, char *server, int port) {
    return tsc_net_timed_dial(istcp, server, port, 0);
}
