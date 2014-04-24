#ifndef _LIBTSC_H_
#define _LIBTSC_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdbool.h>

/* coroutine API */

enum tsc_coroutine_status {
  TSC_COROUTINE_SLEEP = 0xBAFF,
  TSC_COROUTINE_READY = 0xFACE,
  TSC_COROUTINE_RUNNING = 0xBEEF,
  TSC_COROUTINE_WAIT = 0xBADD,
  TSC_COROUTINE_EXIT = 0xDEAD,
};

enum tsc_coroutine_type {
  TSC_COROUTINE_IDLE = 0x0,
  TSC_COROUTINE_NORMAL = 0x1,
  TSC_COROUTINE_MAIN = 0x2,
};

enum tsc_coroutine_deallocate {
  TSC_COROUTINE_UNDETACH = 0x0,
  TSC_COROUTINE_DETACH = 0x1,
};
struct tsc_coroutine;
typedef struct tsc_coroutine_attributes {
  uint32_t stack_size;
  uint32_t timeslice;
  uint32_t affinity;
} tsc_coroutine_attributes_t;

typedef struct tsc_coroutine *tsc_coroutine_t;
typedef int (*tsc_coroutine_handler_t)(void *);

void tsc_coroutine_attr_init(tsc_coroutine_attributes_t *);

tsc_coroutine_t tsc_coroutine_allocate(tsc_coroutine_handler_t, void *,
                                       const char *, uint32_t type,
                                       tsc_coroutine_attributes_t *);

void tsc_coroutine_exit(int);
void tsc_coroutine_yield(void);
tsc_coroutine_t tsc_coroutine_self(void);

/* tsc_chan API */

enum { CHAN_SUCCESS = 0, CHAN_AWAKEN = 1, CHAN_BUSY = 2, CHAN_CLOSED = 4, };

struct tsc_chan;
typedef struct tsc_chan *tsc_chan_t;

tsc_chan_t tsc_chan_allocate(int32_t elemsize, int32_t bufsize);
void tsc_chan_dealloc(tsc_chan_t chan);

extern int _tsc_chan_send(tsc_chan_t chan, void *buf, bool block);
extern int _tsc_chan_recv(tsc_chan_t chan, void *buf, bool block);

#define tsc_chan_send(chan, buf) _tsc_chan_send(chan, buf, true)
#define tsc_chan_recv(chan, buf) _tsc_chan_recv(chan, buf, true)
#define tsc_chan_nbsend(chan, buf) _tsc_chan_send(chan, buf, false)
#define tsc_chan_nbrecv(chan, buf) _tsc_chan_recv(chan, buf, false)

extern int tsc_chan_close(tsc_chan_t chan);

typedef struct tsc_chan_set *tsc_chan_set_t;

/* multi-channel send / recv , like select clause in GoLang .. */
tsc_chan_set_t tsc_chan_set_allocate(int n);
void tsc_chan_set_dealloc(tsc_chan_set_t set);

void tsc_chan_set_send(tsc_chan_set_t set, tsc_chan_t chan, void *buf);
void tsc_chan_set_recv(tsc_chan_set_t set, tsc_chan_t chan, void *buf);

extern int _tsc_chan_set_select(tsc_chan_set_t set, bool block,
                                tsc_chan_t *active);

#define tsc_chan_set_select(set, pchan) _tsc_chan_set_select(set, true, pchan)
#define tsc_chan_set_nbselect(set, pchan) \
  _tsc_chan_set_select(set, false, pchan)

/* -- Asynchronzied Message APIs -- */
typedef struct tsc_msg {
  int32_t size;
  void *msg;
} tsc_msg_t;

extern int tsc_send(struct tsc_coroutine *, void *, int32_t);
extern int tsc_recv(void *, int32_t, bool);

extern int tsc_sendp(struct tsc_coroutine *, void *, int32_t);
extern int tsc_recvp(void **, int32_t *, bool);

/* -- Ticker/Timer API -- */
struct tsc_timer;
typedef struct tsc_timer *tsc_timer_t;

/* userspace api */

tsc_timer_t tsc_timer_allocate(uint32_t period, void (*func)(void));
void tsc_timer_dealloc(tsc_timer_t);
tsc_chan_t tsc_timer_after(tsc_timer_t, uint64_t);
tsc_chan_t tsc_timer_at(tsc_timer_t, uint64_t);

int tsc_timer_start(tsc_timer_t);
int tsc_timer_stop(tsc_timer_t);
int tsc_timer_reset(tsc_timer_t, uint64_t);

void tsc_udelay(uint64_t us);

/* -- Vitual FS API -- */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct tsc_vfs_driver;
typedef struct tsc_vfs_driver *tsc_vfs_driver_t;

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
#define tsc_vfs_close(fd) __tsc_vfs_close(NULL, false, fd)
#define tsc_vfs_flush(fd) __tsc_vfs_flush(NULL, false, fd)
#define tsc_vfs_read(...) __tsc_vfs_read(NULL, false, __VA_ARGS__)
#define tsc_vfs_write(...) __tsc_vfs_write(NULL, false, __VA_ARGS__)
#define tsc_vfs_lseek(...) __tsc_vfs_lseek(NULL, false, __VA_ARGS__)
#define tsc_vfs_ioctl(...) __tsc_vfs_ioctl(NULL, false, __VA_ARGS__)

// synchronized APIs
#define tsc_vfs_open_sync(...) __tsc_vfs_open(NULL, true, __VA_ARGS__)
#define tsc_vfs_close_sync(fd) __tsc_vfs_close(NULL, true, fd)
#define tsc_vfs_flush_sync(fd) __tsc_vfs_flush(NULL, true, fd)
#define tsc_vfs_read_sync(...) __tsc_vfs_read(NULL, true, __VA_ARGS__)
#define tsc_vfs_write_sync(...) __tsc_vfs_write(NULL, true, __VA_ARGS__)
#define tsc_vfs_lseek_sync(...) __tsc_vfs_lseek(NULL, true, __VA_ARGS__)
#define tsc_vfs_ioctl_sync(...) __tsc_vfs_ioctl(NULL, true, __VA_ARGS__)

/* --- NET API --- */

int tsc_net_nonblock(int fd);
int tsc_net_read(int fd, void *buf, int size);
int tsc_net_write(int fd, void *buf, int size);
int tsc_net_wait(int fd, int mode);

int tsc_net_announce(bool istcp, char *server, int port);
int tsc_net_accept(int fd, char *server, int *port);
int tsc_net_lookup(char *name, uint32_t *ip);
int tsc_net_dial(bool istcp, char *server, int port);

/* --- libtsc startup call --- */
int tsc_boot(int argc, char **argv, int np, tsc_coroutine_handler_t entry);

/* --- some macro for external library func calling --- */
#ifdef ENABLE_TIMER

extern sigset_t __vpu_sigmask;
extern __thread int __vpu_sigmask_nest;

#define TSC_SIGNAL_MASK()                           \
  do {                                              \
    if (__vpu_sigmask_nest++ == 0)                  \
      sigprocmask(SIG_BLOCK, &__vpu_sigmask, NULL); \
  } while (0)

#define TSC_SIGNAL_UNMASK()                           \
  do {                                                \
    if (--__vpu_sigmask_nest == 0)                    \
      sigprocmask(SIG_UNBLOCK, &__vpu_sigmask, NULL); \
  } while (0)

#define TSC_SAFE_CALL(func, type, ...) \
  ({                                   \
    TSC_SIGNAL_MASK();                 \
    type __ret = func(__VA_ARGS__);    \
    TSC_SIGNAL_UNMASK();               \
    __ret;                             \
  })

#define TSC_SAFE_CALL_NO_RET(func, ...) \
  ({                                    \
    TSC_SIGNAL_MASK();                  \
    func(__VA_ARGS__);                  \
    TSC_SIGNAL_UNMASK();                \
  })

#else

#define TSC_SAFE_CALL(func, type, ...) func(__VA_ARGS__)
#define TSC_SAFE_CALL_NO_RET(func, ...) func(__VA_ARGS__)

#endif  // ENABLE_TIMER

// -- for atomic add op --
#if defined(__APPLE__) && !defined(__i386__) && !defined(__x86_64__)
#define TSC_ATOMIC_INC(n) (++(n))
#define TSC_ATOMIC_DEC(n) (--(n))
#define TSC_SYNC_ALL()
#else
#define TSC_ATOMIC_INC(n) (__sync_add_and_fetch(&(n), 1))
#define TSC_ATOMIC_DEC(n) (__sync_add_and_fetch(&(n), -1))
#define TSC_SYNC_ALL() __sync_synchronize()
#endif

#ifdef ENABLE_REFCNT
/* -- inline APIs for reference counting mechanism -- */

typedef void (*release_handler_t)(void *);

typedef struct tsc_refcnt {
  uint32_t count;
  release_handler_t release;
} *tsc_refcnt_t;

static inline void tsc_refcnt_init(tsc_refcnt_t ref,
                                   release_handler_t release) {
  ref->count = 0;
  ref->release = release;
}

static inline void *tsc_refcnt_get(tsc_refcnt_t ref) {
  TSC_ATOMIC_INC(ref->count);
  return (void *)ref;
}

static inline void tsc_refcnt_put(tsc_refcnt_t ref) {
  if (TSC_ATOMIC_DEC(ref->count) == 0) (ref->release)(ref);
}

#endif  // ENABLE_REFCNT

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _LIBTSC_H_
