// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#include <stdio.h>
#include "libcoroc.h"

const char wbuf[] = "Hello the world!\n";
char rbuf[100];

int main(int argc, char **argv) {
  int fd;

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    coroc_coroutine_exit(-1);
  }

  fd = coroc_vfs_open(argv[1], O_RDWR | O_APPEND | O_CREAT, 0644);
  coroc_vfs_write(fd, wbuf, sizeof(wbuf));
  coroc_vfs_flush(fd);
  coroc_vfs_close(fd);

  fd = coroc_vfs_open_sync(argv[1], O_RDONLY);
  coroc_vfs_read(fd, rbuf, sizeof(wbuf));
  printf("%s\n", rbuf);

  coroc_vfs_close_sync(fd);

  coroc_coroutine_exit(0);
}
