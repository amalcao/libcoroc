
#include <assert.h>

#include "vfs.h"
#include "vpu.h"

struct {
    int items;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    queue_t wait_que;
    queue_t ready_que;
    TSC_OS_THREAD_T os_thr;
} tsc_vfs_manager;

// the default file driver
struct tsc_vfs_driver tsc_vfs_file_drv = {
    .open = open,
    .read = read,
    .write = write,
    .lseek = lseek,
    .flush = NULL,
    .close = close,
};

TSC_SIGNAL_MASK_DECLARE

// the vfs sub-system APIs
void *tsc_vfs_routine (void *unused)
{
  for (;;) {

      pthread_mutex_lock (& tsc_vfs_manager . mutex);
      
      if (tsc_vfs_manager . items == 0)
        pthread_cond_wait (& tsc_vfs_manager . cond,
            & tsc_vfs_manager . mutex);
       
      tsc_vfs_ops *ops = atomic_queue_rem (& tsc_vfs_manager . wait_que);
      assert (ops != NULL);

      tsc_vfs_manager . items --;
      
      pthread_mutex_unlock (& tsc_vfs_manager . mutex);

      switch (ops -> ops_type) {
        case TSC_VFS_OPEN:
            {
              int fd = 
                ops -> driver -> open((char*)(ops->buf), (int)(ops->arg0));
              ops -> arg0 = fd;
              break;
            }

        case TSC_VFS_CLOSE:
          ops -> driver -> close(ops -> fd);
          break;

        case TSC_VFS_FLUSH:
          if (ops -> driver -> flush)
            ops -> driver -> flush(ops -> fd);
          break;

        case TSC_VFS_READ:
            {
              ssize_t sz = 
                ops -> driver -> read(ops->fd, ops->buf, (size_t)(ops->arg0));
              ops -> arg0 = sz;
              break;
            }

        case TSC_VFS_WRITE:
            {
              ssize_t sz =
                ops -> driver -> write(ops->fd, ops->buf, (size_t)(ops->arg0));
              ops -> arg0 = sz;
              break;
            }

        case TSC_VFS_LSEEK:
            {
              off_t off;
              if (ops -> driver -> lseek)
                off = ops -> driver -> lseek(ops->fd, (off_t)ops->arg0, (int)ops->arg1);
              ops -> arg0 = off;
              break;
            }

        default:
          assert (0);
      } // switch

      // add the result back to the ready queue ..
      atomic_queue_add (& tsc_vfs_manager . ready_que, & ops -> link);
  } 

  // this routine will never return
}

void tsc_vfs_initialize (void)
{
    // init the vfs_manager ..

    tsc_vfs_manager . items = 0;
    pthread_mutex_init (& tsc_vfs_manager . mutex, NULL);
    pthread_cond_init (& tsc_vfs_manager . cond, NULL);
    atomic_queue_init (& tsc_vfs_manager . wait_que);
    atomic_queue_init (& tsc_vfs_manager . ready_que);

    TSC_OS_THREAD_CREATE(
        & tsc_vfs_manager . os_thr, 
        NULL, tsc_vfs_routine, NULL); 

    return;
}

thread_t tsc_vfs_get_thread (void) 
{
    tsc_vfs_ops * ops = 
        atomic_queue_rem (& tsc_vfs_manager . ready_que);
    if (ops != NULL) 
        return ops -> wait;
    return NULL;
}

static void __tsc_vfs_init_ops (tsc_vfs_ops *ops, int type,
    int fd, int64_t arg0, int64_t arg1, void *buf,
    tsc_vfs_driver_t drv)
{
    ops -> ops_type = type;
    ops -> fd = fd;
    ops -> arg0 = arg0;
    ops -> arg1 = arg1;
    ops -> buf = buf;
    ops -> driver = drv;
    
    queue_item_init (& ops -> link, ops);
}

static void __tsc_vfs_add_ops (tsc_vfs_ops * ops)
{
    TSC_SIGNAL_MASK();

    ops -> wait = thread_self();
    pthread_mutex_lock (& tsc_vfs_manager . mutex);
    
    atomic_queue_add (& tsc_vfs_manager . wait_que, & ops -> link);
    tsc_vfs_manager . items ++;

    if (tsc_vfs_manager . items == 1)
        pthread_cond_signal (& tsc_vfs_manager . cond);

    // suspend current thread and release the lock ..
    vpu_suspend (NULL, 
        & tsc_vfs_manager . mutex,
        (unlock_hander_t)pthread_mutex_unlock);
    
    TSC_SIGNAL_UNMASK();
}

// implementation of APIs
int __tsc_vfs_open (const char *name, int flags, bool sync, tsc_vfs_driver_t drv)
{
    if (drv == NULL) 
        drv = & tsc_vfs_file_drv;
    
    if (sync)
        return drv -> open (name, flags);

    tsc_vfs_ops ops;
    __tsc_vfs_init_ops (& ops, TSC_VFS_OPEN, 0, flags, 0, (void*)name, drv);

    // this call will suspend currrent thread ..
    __tsc_vfs_add_ops (& ops);
    
    // wake up ..
    return (int)(ops . arg0);
}

int __tsc_vfs_close (int fd, bool sync, tsc_vfs_driver_t drv)
{
    if (drv == NULL) 
        drv = & tsc_vfs_file_drv;
    
    if (sync)
        return drv -> close (fd);

    tsc_vfs_ops ops;
    __tsc_vfs_init_ops (& ops, TSC_VFS_CLOSE, fd, 0, 0, NULL, drv);

    // this call will suspend currrent thread ..
    __tsc_vfs_add_ops (& ops);
    
    // wake up ..
    return;
}

void __tsc_vfs_flush (int fd, bool sync, tsc_vfs_driver_t drv)
{
    if (drv == NULL) 
        drv = & tsc_vfs_file_drv;
    
    if (sync && drv -> flush)
        return drv -> flush(fd);

    tsc_vfs_ops ops;
    __tsc_vfs_init_ops (& ops, TSC_VFS_FLUSH, fd, 0, 0, NULL, drv);

    // this call will suspend currrent thread ..
    __tsc_vfs_add_ops (& ops);
    
    // wake up ..
    return;
}

ssize_t __tsc_vfs_read (int fd, void *buf, size_t size, bool sync,
    tsc_vfs_driver_t drv)
{
    if (drv == NULL)
        drv = & tsc_vfs_file_drv;

    if (sync)
        return drv -> read(fd, buf, size);

    tsc_vfs_ops ops;
    __tsc_vfs_init_ops (& ops, TSC_VFS_READ, fd, size, 0, buf, drv);

    __tsc_vfs_add_ops (& ops);

    return (ssize_t)(ops . arg0);
}

ssize_t __tsc_vfs_write (int fd, const void *buf, size_t size, bool sync,
    tsc_vfs_driver_t drv)
{
    if (drv == NULL)
        drv = & tsc_vfs_file_drv;

    if (sync)
        return drv -> write(fd, buf, size);

    tsc_vfs_ops ops;
    __tsc_vfs_init_ops (& ops, TSC_VFS_WRITE, fd, size, 0, (void*)buf, drv);

    __tsc_vfs_add_ops (& ops);

    return (ssize_t)(ops . arg0);
}

off_t __tsc_vfs_lseek (int fd, off_t offset, int whence, bool sync,
    tsc_vfs_driver_t drv)
{
    if (drv == NULL)
        drv = & tsc_vfs_file_drv;

    if (sync) {
        if (drv -> lseek == NULL) 
            return -1;
        return drv -> lseek (fd, offset, whence);
    }

    tsc_vfs_ops ops;
    __tsc_vfs_init_ops (& ops, TSC_VFS_LSEEK, fd, offset, whence, NULL, drv);

    __tsc_vfs_add_ops (& ops);

    return (off_t)(ops . arg0);
}
