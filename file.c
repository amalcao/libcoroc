#include <stdio.h>
#include "libtsc.h"

const char wbuf[] = "Hello the world!\n";
char rbuf[100];

int main (int argc, char **argv)
{
    int fd = tsc_vfs_open("/tmp/123.txt", O_RDWR | O_APPEND | O_CREAT);
    tsc_vfs_write(fd, wbuf, sizeof(wbuf));
    tsc_vfs_flush(fd);
    tsc_vfs_close(fd);

    fd = tsc_vfs_open_sync("/tmp/123.txt", O_RDONLY);
    tsc_vfs_read(fd, rbuf, sizeof(wbuf));
    printf("%s\n", rbuf);

    tsc_vfs_close_sync(fd);

    thread_exit (0);
}
