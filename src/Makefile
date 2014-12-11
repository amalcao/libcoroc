OS := $(shell uname)

enable_debug ?= 0

TSC_HEADERS:= channel.h message.h context.h support.h coroutine.h vpu.h vfs.h netpoll.h async.h 
TSC_HEADERS+= tsc_clock.h tsc_lock.h tsc_queue.h tsc_hash.h tsc_time.h tsc_group.h refcnt.h
TSC_CFILES := boot.c channel.c message.c clock.c context.c coroutine.c vpu.c time.c vfs.c netpoll.c net.c tsc_main.c hash.c async.c group.c

CFLAGS += -Werror
CFLAGS += -I../include/inter

CFLAGS += -DIGNORE_CONFIG_H ## do not include the cmake generated config.h

ifeq (${OS}, Darwin)
TSC_HEADERS += darwin/pthread_barrier.h \
				darwin/pthread_spinlock.h \
				darwin/amd64-ucontext.h \
				darwin/386-ucontext.h \
				darwin/power-ucontext.h 
TSC_CFILES += darwin/pthread_barrier.c darwin/ucontext.c
TSC_OBJS := darwin/asm.o
endif

ifeq (${OS}, Linux)
TSC_CFILES += netpoll_epoll.c futex_impl.c
# enable the notify mechanism for linux
CFLAGS += -DENABLE_NOTIFY
TSC_CFILES += notify.c
TSC_HEADERS += notify.h
# enable futex based lock mechanism for linux
ifeq (${enable_futex}, 1)
TSC_HEADERS += futex_lock.h
TSC_CFILES += futex_lock.c
CFLAGS += -DUSE_FUTEX_LOCK
endif

else
TSC_CFILES += netpoll_poll.c
endif

TSC_OBJS += $(subst .c,.o,$(TSC_CFILES))

CFLAGS += -DENABLE_WORKSTEALING

ifeq (${enable_lockfree_runq}, 1)
	CFLAGS += -DENABLE_LOCKFREE_RUNQ
endif

ifeq (${enable_timer}, 1)
	CFLAGS += -DENABLE_TIMESHARE
endif

ifeq (${use_clang}, 1)
	CC := clang
endif

ifeq (${enable_debug}, 0)
	CFLAGS += -O2
else
	CFLAGS += -g3 
endif
CFLAGS += -DENABLE_DEBUG=${enable_debug}

ifeq (${enable_splitstack}, 1)
	CFLAGS += -DENABLE_SPLITSTACK
	CFLAGS += -fsplit-stack
endif

ifneq (${enable_deadlock_detect}, 0)
	CFLAGS += -DENABLE_DEADLOCK_DETECT
ifeq (${use_clang}, 0)
	CFLAGS += -rdynamic
endif
endif

all: libTSC.a

libTSC.a: $(TSC_OBJS)
	ar r $@ $(TSC_OBJS)

%.o:%.S
	$(CC) -c ${CFLAGS} $< -o $@

%.o:%.c ${TSC_HEADERS}
	$(CC) -c ${CFLAGS} $< -o $@

.PHONY:clean install
clean:
	rm -f *.o libTSC.a dist/*.o darwin/*.o

install: libTSC.a
	mkdir -p ../lib
	cp libTSC.a ../lib
