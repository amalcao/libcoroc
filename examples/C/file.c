#include <stdio.h>
#include "libtsc.h"

const char wbuf[] = "Hello the world!\n";
char rbuf[100];

int main(int argc, char **argv) {
  int fd;

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    tsc_coroutine_exit(-1);
  }

  fd = tsc_vfs_open(argv[1], O_RDWR | O_APPEND | O_CREAT, 0644);
  tsc_vfs_write(fd, wbuf, sizeof(wbuf));
  tsc_vfs_flush(fd);
  tsc_vfs_close(fd);

  fd = tsc_vfs_open_sync(argv[1], O_RDONLY);
  tsc_vfs_read(fd, rbuf, sizeof(wbuf));
  printf("%s\n", rbuf);

  tsc_vfs_close_sync(fd);

  tsc_coroutine_exit(0);
}
