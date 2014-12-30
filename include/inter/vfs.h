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
typedef struct tsc_vfs_driver {
  open_callback_t open;
  read_callback_t read;
  write_callback_t write;
  lseek_callback_t lseek;
  flush_callback_t flush;
  ioctl_callback_t ioctl;
  close_callback_t close;
} *tsc_vfs_driver_t;

// the default file driver
extern struct tsc_vfs_driver tsc_vfs_file_drv;

typedef struct {
  int fd;
  int ops_type;
  int64_t arg0;  // also for retval
  int64_t arg1;  // optional
  void *buf;
  struct tsc_vfs_driver *driver;  // optional
} tsc_vfs_ops;

// the APIs ..
int __tsc_vfs_open(tsc_vfs_driver_t drv, bool sync, const char *name, int flags,
                   ...);
int __tsc_vfs_close(tsc_vfs_driver_t drv, bool sync, int fd);
int __tsc_vfs_ioctl(tsc_vfs_driver_t drv, bool sync, int fd, int cmd, ...);
void __tsc_vfs_flush(tsc_vfs_driver_t drv, bool sync, int fd);
ssize_t __tsc_vfs_read(tsc_vfs_driver_t drv, bool sync, int fd, void *buf,
                       size_t size);
ssize_t __tsc_vfs_write(tsc_vfs_driver_t drv, bool sync, int fd,
                        const void *buf, size_t size);
off_t __tsc_vfs_lseek(tsc_vfs_driver_t drv, bool sync, int fd, off_t offset,
                      int whence);

// asynchronized APIs
#define tsc_vfs_open(...) __tsc_vfs_open(NULL, false, __VA_ARGS__)
#define tsc_vfs_close(...) __tsc_vfs_close(NULL, false, __VA_ARGS__)
#define tsc_vfs_flush(...) __tsc_vfs_flush(NULL, false, __VA_ARGS__)
#define tsc_vfs_read(...) __tsc_vfs_read(NULL, false, __VA_ARGS__)
#define tsc_vfs_write(...) __tsc_vfs_write(NULL, false, __VA_ARGS__)
#define tsc_vfs_lseek(...) __tsc_vfs_lseek(NULL, false, __VA_ARGS__)
#define tsc_vfs_ioctl(...) __tsc_vfs_ioctl(NULL, false, __VA_ARGS__)

// synchronized APIs
#define tsc_vfs_open_sync(...) __tsc_vfs_open(NULL, true, __VA_ARGS__)
#define tsc_vfs_close_sync(...) __tsc_vfs_close(NULL, true, __VA_ARGS__)
#define tsc_vfs_flush_sync(...) __tsc_vfs_flush(NULL, true, __VA_ARGS__)
#define tsc_vfs_read_sync(...) __tsc_vfs_read(NULL, true, __VA_ARGS__)
#define tsc_vfs_write_sync(...) __tsc_vfs_write(NULL, true, __VA_ARGS__)
#define tsc_vfs_lseek_sync(...) __tsc_vfs_lseek(NULL, true, __VA_ARGS__)
#define tsc_vfs_ioctl_sync(...) __tsc_vfs_ioctl(NULL, true, __VA_ARGS__)

#endif  // _TSC_VFS_H_
