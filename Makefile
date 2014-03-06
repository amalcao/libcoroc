
APPS := findmax.run timeshare.run select.run ticker.run file.run 
APPS += primes.run httpload.run tcpproxy.run ## modify from examples in LibTask
APPS += mandelbrot.run spectral-norm.run ## modify from benchmarkgame.org

OS := $(shell uname)

CC := gcc-4.8

TSC_HEADERS:= clock.h channel.h context.h lock.h queue.h support.h thread.h vpu.h time.h vfs.h netpoll.h
TSC_CFILES := boot.c channel.c clock.c context.c thread.c vpu.c time.c vfs.c netpoll.c net.c

ifeq (${OS}, Darwin)
TSC_HEADERS += pthread_barrier.h pthread_spinlock.h amd64-ucontext.h 386-ucontext.h power-ucontext.h 
TSC_CFILES += pthread_barrier.c ucontext.c
TSC_OBJS := asm.o
use_clang := 1
endif

ifeq (${OS}, Linux)
TSC_CFILES += netpoll_epoll.c
else
TSC_CFILES += netpoll_poll.c
endif

TSC_OBJS += $(subst .c,.o,$(TSC_CFILES))

#CFLAGS += -DENABLE_QUICK_RESPONSE
CFLAGS += -DENABLE_WORKSTEALING
CFLAGS += -DENABLE_CHANNEL_SELECT
CFLAGS += -DENABLE_VFS

ifeq (${enable_timer}, 1)
	CFLAGS += -DENABLE_TIMER
endif

ifeq (${use_clang}, 1)
	CC := clang
endif

ifeq (${enable_optimize}, 1)
	CFLAGS += -O2
else
	CFLAGS += -g3
endif

ifeq (${enable_splitstack}, 1)
	CFLAGS += -DENABLE_SPLITSTACK
	CFLAGS += -fsplit-stack
endif

ifeq (${enable_daedlock_detect}, 1)
	CFLAGS += -DENABLE_DAEDLOCK_DETECT
	CFLAGS += -rdynamic
endif

all: ${APPS}

libTSC.a: $(TSC_OBJS)
	ar r $@ $(TSC_OBJS)

%.o:%.S
	$(CC) -c ${CFLAGS} -Werror $<

%.o:%.c
	$(CC) -c ${CFLAGS} -Werror $<

%.run:%.c libTSC.a
	$(CC) $< ${CFLAGS} -L. -lTSC -lpthread -lm -o $@

.PHONY:clean
clean:
	rm -f *.o *.run libTSC.a 
