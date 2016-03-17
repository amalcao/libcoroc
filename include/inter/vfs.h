// Copyright 2016 Amal Cao (amalcaowei@gmail.com). All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file.

#ifndef _TSC_VFS_H_
#define _TSC_VFS_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "support.h"

// -- the virual file system modules --

// the operation types
enum {
  TSC_VFS_OPEN = 0,
  TSC_VFS_CLOSE,
  TSC_VFS_READ,
  TSC_VFS_WRITE,
  TSC_VFS_LSEEK,
  TSC_VFS_FLUSH,
  TSC_VFS_IOCTL,
};

// define the callback handlers' types
typedef int (*open_callback_t)(const char *, int, ...);
typedef ssize_t (*read_callback_t)(int, void *, size_t);
typedef ssize_t (*write_callback_t)(int, const void *, size_t);
typedef off_t (*lseek_callback_t)(int, off_t, int);
typedef void (*flush_callback_t)(int);
typedef int (*ioctl_callback_t)(int, int, ...);
typedef int (*close_callback_t)(int);

// the file operation driver
typedef struct coroc_vfs_driver {
  open_callback_t open;
  read_callback_t read;
  write_callback_t write;
  lseek_callback_t lseek;
  flush_callback_t flush;
  ioctl_callback_t ioctl;
  close_callback_t close;
} *coroc_vfs_driver_t;

// the default file driver
extern struct coroc_vfs_driver coroc_vfs_file_drv;

typedef struct {
  int fd;
  int ops_type;
  int64_t arg0;  // also for retval
  int64_t arg1;  // optional
  void *buf;
  struct coroc_vfs_driver *driver;  // optional
} coroc_vfs_ops;

// the APIs ..
int __coroc_vfs_open(coroc_vfs_driver_t drv, bool sync, const char *name, int flags,
                   ...);
int __coroc_vfs_close(coroc_vfs_driver_t drv, bool sync, int fd);
int __coroc_vfs_ioctl(coroc_vfs_driver_t drv, bool sync, int fd, int cmd, ...);
void __coroc_vfs_flush(coroc_vfs_driver_t drv, bool sync, int fd);
ssize_t __coroc_vfs_read(coroc_vfs_driver_t drv, bool sync, int fd, void *buf,
                       size_t size);
ssize_t __coroc_vfs_write(coroc_vfs_driver_t drv, bool sync, int fd,
                        const void *buf, size_t size);
off_t __coroc_vfs_lseek(coroc_vfs_driver_t drv, bool sync, int fd, off_t offset,
                      int whence);

// asynchronized APIs
#define coroc_vfs_open(...) __coroc_vfs_open(NULL, false, __VA_ARGS__)
#define coroc_vfs_close(...) __coroc_vfs_close(NULL, false, __VA_ARGS__)
#define coroc_vfs_flush(...) __coroc_vfs_flush(NULL, false, __VA_ARGS__)
#define coroc_vfs_read(...) __coroc_vfs_read(NULL, false, __VA_ARGS__)
#define coroc_vfs_write(...) __coroc_vfs_write(NULL, false, __VA_ARGS__)
#define coroc_vfs_lseek(...) __coroc_vfs_lseek(NULL, false, __VA_ARGS__)
#define coroc_vfs_ioctl(...) __coroc_vfs_ioctl(NULL, false, __VA_ARGS__)

// synchronized APIs
#define coroc_vfs_open_sync(...) __coroc_vfs_open(NULL, true, __VA_ARGS__)
#define coroc_vfs_close_sync(...) __coroc_vfs_close(NULL, true, __VA_ARGS__)
#define coroc_vfs_flush_sync(...) __coroc_vfs_flush(NULL, true, __VA_ARGS__)
#define coroc_vfs_read_sync(...) __coroc_vfs_read(NULL, true, __VA_ARGS__)
#define coroc_vfs_write_sync(...) __coroc_vfs_write(NULL, true, __VA_ARGS__)
#define coroc_vfs_lseek_sync(...) __coroc_vfs_lseek(NULL, true, __VA_ARGS__)
#define coroc_vfs_ioctl_sync(...) __coroc_vfs_ioctl(NULL, true, __VA_ARGS__)

#endif  // _TSC_VFS_H_
