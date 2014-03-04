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
#include "thread.h"

// -- the virual file system modules --

// the operation types 
enum {
    TSC_VFS_OPEN = 0,
    TSC_VFS_CLOSE,
    TSC_VFS_READ,
    TSC_VFS_WRITE,
    TSC_VFS_LSEEK,
    TSC_VFS_FLUSH,
};

// define the callback handlers' types 
typedef int (*open_callback_t) (const char*, int, ...);
typedef ssize_t (*rdwr_callback_t) (int, void*, size_t);
typedef off_t (*lseek_callback_t) (int, off_t, int);
typedef void (*flush_callback_t) (int);
typedef void (*close_callback_t) (int);

// the file operation driver
typedef struct tsc_vfs_driver {
    open_callback_t open;
    rdwr_callback_t read;
    rdwr_callback_t write;
    lseek_callback_t lseek;
    flush_callback_t flush;
    close_callback_t close;
} *tsc_vfs_driver_t;

// the default file driver
struct tsc_vfs_driver tsc_vfs_file_drv;

typedef struct {
    int     fd;
    int     ops_type;
    int64_t arg0; // also for retval
    int64_t arg1; // optional
    void    *buf;
    struct tsc_vfs_driver *driver; // optional
    thread_t wait;
    queue_item_t link;
} tsc_vfs_ops;

// the internal APIs ..
thread_t tsc_vfs_get_thread ();
void tsc_vfs_initialize ();

// the APIs ..
int __tsc_vfs_open (const char *name, int flags, bool sync, tsc_vfs_driver_t drv);
void __tsc_vfs_close (int fd, bool sync, tsc_vfs_driver_t drv);
void __tsc_vfs_flush (int fd, bool sync, tsc_vfs_driver_t drv);
ssize_t __tsc_vfs_read (int fd, void *buf, size_t size, bool sync, tsc_vfs_driver_t drv);
ssize_t __tsc_vfs_write (int fd, void *buf, size_t size, bool sync, tsc_vfs_driver_t drv);
off_t __tsc_vfs_lseek (int fd, off_t offset, int whence, bool sync, tsc_vfs_driver_t drv);

// asynchronized APIs
#define tsc_vfs_open(...) __tsc_vfs_open(__VA_ARGS__, false, NULL)
#define tsc_vfs_close(fd) __tsc_vfs_close(fd, false, NULL)
#define tsc_vfs_flush(fd) __tsc_vfs_flush(fd, false, NULL)
#define tsc_vfs_read(...) __tsc_vfs_read(__VA_ARGS__, false, NULL)
#define tsc_vfs_write(...) __tsc_vfs_write(__VA_ARGS__, false, NULL)
#define tsc_vfs_lseek(...) __tsc_vfs_lseek(__VA_ARGS__, false, NULL)

// synchronized APIs
#define tsc_vfs_open_sync(...) __tsc_vfs_open(__VA_ARGS__, true, NULL)
#define tsc_vfs_close_sync(fd) __tsc_vfs_close(fd, true, NULL)
#define tsc_vfs_flush_sync(fd) __tsc_vfs_flush(fd, true, NULL)
#define tsc_vfs_read_sync(...) __tsc_vfs_read(__VA_ARGS__, true, NULL)
#define tsc_vfs_write_sync(...) __tsc_vfs_write(__VA_ARGS__, true, NULL)
#define tsc_vfs_lseek_sync(...) __tsc_vfs_lseek(__VA_ARGS__, true, NULL)

#endif // _TSC_VFS_H_
