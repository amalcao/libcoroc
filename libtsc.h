#ifndef _LIBTSC_H_
#define _LIBTSC_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>

/* thread API */

enum thread_status {
    TSC_THREAD_SLEEP    = 0xBAFF,
    TSC_THREAD_READY    = 0xFACE,
    TSC_THREAD_RUNNING  = 0xBEEF,
    TSC_THREAD_WAIT     = 0xBADD,
    TSC_THREAD_EXIT     = 0xDEAD,
};

enum thread_type {
    TSC_THREAD_IDLE     = 0x0,
    TSC_THREAD_NORMAL   = 0x1,
    TSC_THREAD_MAIN     = 0x2,
};

enum thread_deallocate {
    TSC_THREAD_UNDETACH = 0x0,
    TSC_THREAD_DETACH = 0x1,
};
struct thread;
typedef struct thread_attributes {
    uint32_t stack_size;
    uint32_t timeslice;
    uint32_t affinity;
} thread_attributes_t;

typedef struct thread* thread_t;
typedef int (*thread_handler_t)(void *);


void thread_attr_init (thread_attributes_t*);

thread_t thread_allocate (thread_handler_t, void*, 
    const char*, uint32_t type, thread_attributes_t*);

void thread_exit (int);
void thread_yield (void);
thread_t thread_self (void);

/* channel API */

enum {
	CHAN_SUCCESS = 0,
	CHAN_AWAKEN,
	CHAN_BUSY,
};
struct channel;
struct chan_set;

typedef struct channel* channel_t ;
typedef struct chan_set* chan_set_t ;

channel_t channel_allocate (int32_t elemsize, int32_t bufsize);
void channel_dealloc (channel_t chan);

extern int _channel_send (channel_t chan, void * buf, bool block);
extern int _channel_recv (channel_t chan, void * buf, bool block);

#define channel_send(chan, buf) _channel_send(chan, buf, true)
#define channel_recv(chan, buf) _channel_recv(chan, buf, true)
#define channel_nbsend(chan, buf) _channel_send(chan, buf, false)
#define channel_nbrecv(chan, buf) _channel_recv(chan, buf, false)

/* multi-channel send / recv , like select clause in GoLang .. */
chan_set_t chan_set_allocate (void);
void chan_set_dealloc (chan_set_t set);

void chan_set_send (chan_set_t set, channel_t chan, void * buf);
void chan_set_recv (chan_set_t set, channel_t chan, void * buf);

extern int _chan_set_select (chan_set_t set, bool block, channel_t * active);

#define chan_set_select(set, pchan) _chan_set_select(set, true, pchan)
#define chan_set_nbselect(set, pchan) _chan_set_select(set, false, pchan)

/* -- Ticker/Timer API -- */
struct tsc_timer;
typedef struct tsc_timer* tsc_timer_t ;

/* userspace api */

tsc_timer_t timer_allocate (uint32_t period, void (*func)(void));
void timer_dealloc (tsc_timer_t );
channel_t timer_after (tsc_timer_t, uint64_t);
channel_t timer_at (tsc_timer_t, uint64_t);

int timer_start (tsc_timer_t);
int timer_stop (tsc_timer_t);
int timer_reset (tsc_timer_t, uint64_t);

/* -- Vitual FS API -- */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct tsc_vfs_driver;
typedef struct tsc_vfs_driver *tsc_vfs_driver_t;

// the userspace APIs ..
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


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBTSC_H_
