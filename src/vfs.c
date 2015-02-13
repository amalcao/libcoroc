
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdarg.h>

#include "async.h"
#include "vfs.h"
#include "vpu.h"

// the default file driver
struct tsc_vfs_driver tsc_vfs_file_drv = {.open = open,
                                          .read = read,
                                          .write = write,
                                          .lseek = lseek,
                                          .flush = NULL,
                                          .close = close,
                                          .ioctl = NULL, };

// the vfs sub-system APIs
void tsc_vfs_callback(tsc_vfs_ops *ops) {
  assert(ops != NULL);

  switch (ops->ops_type) {
    case TSC_VFS_OPEN: {
      int fd = ops->driver->open((char *)(ops->buf), (int)(ops->arg0),
                                 (int)(ops->arg1));
      ops->arg0 = fd;
      break;
    }

    case TSC_VFS_CLOSE:
      ops->driver->close(ops->fd);
      break;

    case TSC_VFS_FLUSH:
      if (ops->driver->flush) ops->driver->flush(ops->fd);
      break;

    case TSC_VFS_READ: {
      ssize_t sz = ops->driver->read(ops->fd, ops->buf, (size_t)(ops->arg0));
      ops->arg0 = sz;
      break;
    }

    case TSC_VFS_WRITE: {
      ssize_t sz = ops->driver->write(ops->fd, ops->buf, (size_t)(ops->arg0));
      ops->arg0 = sz;
      break;
    }

    case TSC_VFS_LSEEK: {
      off_t off;
      if (ops->driver->lseek)
        off = ops->driver->lseek(ops->fd, (off_t)ops->arg0, (int)ops->arg1);
      ops->arg0 = off;
      break;
    }

    case TSC_VFS_IOCTL: {
      int ret = -1;
      if (ops->driver->ioctl)
        ret = ops->driver->ioctl(ops->fd, (int)ops->arg0, (int)ops->arg1);
      ops->arg0 = ret;
      break;
    }

    default:
      assert(0);
  }  // switch

  return ;
}

static inline void __tsc_vfs_add_ops(tsc_vfs_ops *ops) {
  tsc_async_request_submit((tsc_async_callback_t)tsc_vfs_callback, ops);
}

static void __tsc_vfs_init_ops(tsc_vfs_ops *ops, int type, int fd, int64_t arg0,
                               int64_t arg1, void *buf, tsc_vfs_driver_t drv) {
  ops->ops_type = type;
  ops->fd = fd;
  ops->arg0 = arg0;
  ops->arg1 = arg1;
  ops->buf = buf;
  ops->driver = drv;
}

// implementation of APIs
int __tsc_vfs_open(tsc_vfs_driver_t drv, bool sync, const char *name, int flags,
                   ...) {
  va_list ap;
  mode_t mode = 0644;

  if (drv == NULL) drv = &tsc_vfs_file_drv;

  if (flags & O_CREAT) {
    va_start(ap, flags);
#ifdef __APPLE__
    // FIXME: it is a BUG that mode_t in OS X is defned as a unsigned short,
    // which cannot be supported by var_arg in OS X, so we can only pass a int..
    mode = (mode_t)va_arg(ap, int);
#else
    mode = va_arg(ap, mode_t);
#endif
    va_end(ap);
  }

  if (sync) return drv->open(name, flags, mode);

  tsc_vfs_ops ops;
  __tsc_vfs_init_ops(&ops, TSC_VFS_OPEN, 0, flags, mode, (void *)name, drv);

  // this call will suspend currrent thread ..
  __tsc_vfs_add_ops(&ops);

  // wake up ..
  return (int)(ops.arg0);
}

int __tsc_vfs_close(tsc_vfs_driver_t drv, bool sync, int fd) {
  if (drv == NULL) drv = &tsc_vfs_file_drv;

  if (sync) return drv->close(fd);

  tsc_vfs_ops ops;
  __tsc_vfs_init_ops(&ops, TSC_VFS_CLOSE, fd, 0, 0, NULL, drv);

  // this call will suspend currrent thread ..
  __tsc_vfs_add_ops(&ops);

  // wake up ..
  return (int)(ops.arg0);
}

void __tsc_vfs_flush(tsc_vfs_driver_t drv, bool sync, int fd) {
  if (drv == NULL) drv = &tsc_vfs_file_drv;

  if (sync && drv->flush) return drv->flush(fd);

  tsc_vfs_ops ops;
  __tsc_vfs_init_ops(&ops, TSC_VFS_FLUSH, fd, 0, 0, NULL, drv);

  // this call will suspend currrent thread ..
  __tsc_vfs_add_ops(&ops);

  // wake up ..
  return;
}

ssize_t __tsc_vfs_read(tsc_vfs_driver_t drv, bool sync, int fd, void *buf,
                       size_t size) {
  if (drv == NULL) drv = &tsc_vfs_file_drv;

  if (sync) return drv->read(fd, buf, size);

  tsc_vfs_ops ops;
  __tsc_vfs_init_ops(&ops, TSC_VFS_READ, fd, size, 0, buf, drv);

  __tsc_vfs_add_ops(&ops);

  return (ssize_t)(ops.arg0);
}

ssize_t __tsc_vfs_write(tsc_vfs_driver_t drv, bool sync, int fd,
                        const void *buf, size_t size) {
  if (drv == NULL) drv = &tsc_vfs_file_drv;

  if (sync) return drv->write(fd, buf, size);

  tsc_vfs_ops ops;
  __tsc_vfs_init_ops(&ops, TSC_VFS_WRITE, fd, size, 0, (void *)buf, drv);

  __tsc_vfs_add_ops(&ops);

  return (ssize_t)(ops.arg0);
}

off_t __tsc_vfs_lseek(tsc_vfs_driver_t drv, bool sync, int fd, off_t offset,
                      int whence) {
  if (drv == NULL) drv = &tsc_vfs_file_drv;

  if (sync) {
    if (drv->lseek == NULL) return -1;
    return drv->lseek(fd, offset, whence);
  }

  tsc_vfs_ops ops;
  __tsc_vfs_init_ops(&ops, TSC_VFS_LSEEK, fd, offset, whence, NULL, drv);

  __tsc_vfs_add_ops(&ops);

  return (off_t)(ops.arg0);
}

int __tsc_vfs_ioctl(tsc_vfs_driver_t drv, bool sync, int fd, int cmd, ...) {
  va_list ap;
  int option;

  if (drv == NULL) drv = &tsc_vfs_file_drv;

  va_start(ap, cmd);
  option = va_arg(ap, int);
  va_end(ap);

  if (sync) {
    if (drv->ioctl == NULL) return -1;
    return drv->ioctl(fd, cmd, option);
  }

  tsc_vfs_ops ops;
  __tsc_vfs_init_ops(&ops, TSC_VFS_IOCTL, fd, cmd, option, NULL, drv);

  __tsc_vfs_add_ops(&ops);

  return (int)(ops.arg0);
}
