#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

#include "libtsc.h"

char *server;
int port;
int proxy_task (void*);
int rwtask (void*);

int* mkfd2 (int fd1, int fd2)
{
    int *a;

    a = malloc(2*sizeof a[0]);
    if (a == 0) {
        fprintf (stderr, "out of memory\n");
        abort();
    }

    a[0] = fd1;
    a[1] = fd2;

    return a;
}

int main (int argc, char **argv)
{
    int cfd, fd;
    int rport;
    char remote[16];

    if (argc != 4) {
        fprintf (stderr, "usage: tcpproxy localport server remoteport\n");
        tsc_coroutine_exit(-1);
    }

    server = argv[2];
    port = atoi(argv[3]);

    if ((fd = tsc_net_announce(true, 0, atoi(argv[1]))) < 0) {
        fprintf(stderr, "cannot announce on tcp port %d: %s\n",
            atoi(argv[1]), strerror(errno));
            tsc_coroutine_exit(-1);
    }

    tsc_net_nonblock(fd);
    while ((cfd = tsc_net_accept(fd, remote, &rport)) >= 0) {
        fprintf(stderr, "connection from %s:%d\n", remote, rport);
        tsc_coroutine_allocate (proxy_task, cfd, "proxy", TSC_COROUTINE_NORMAL, 0);
    }

    tsc_coroutine_exit (0);
}

int proxy_task (void *v)
{
    int fd, remotefd;

    fd = (int)v;
    if ((remotefd = tsc_net_dial(true, server, port)) < 0) {
        close (fd);
        tsc_coroutine_exit (-1);
    }

    fprintf(stderr, "connected to %s:%d\n", server, port);

    tsc_coroutine_allocate (rwtask, mkfd2(fd, remotefd), "rwtask", TSC_COROUTINE_NORMAL, 0);
    tsc_coroutine_allocate (rwtask, mkfd2(remotefd, fd), "rwtask", TSC_COROUTINE_NORMAL, 0);

    tsc_coroutine_exit (0);
}


int rwtask (void *v)
{
    int *a, rfd, wfd, n;
    char buf[2048];

    a = v;
    rfd = a[0];
    wfd = a[1];
    free(a);

    while ((n = tsc_net_read(rfd, buf, sizeof buf)) > 0)
        tsc_net_write (wfd, buf, n);

    shutdown (wfd, SHUT_WR);
    close (rfd);

    tsc_coroutine_exit (0);
}
