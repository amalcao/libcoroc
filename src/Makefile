OS := $(shell uname)

TSC_HEADERS:= clock.h channel.h message.h context.h lock.h queue.h support.h coroutine.h vpu.h time.h vfs.h netpoll.h
TSC_CFILES := boot.c channel.c message.c clock.c context.c coroutine.c vpu.c time.c vfs.c netpoll.c net.c tsc_main.c

ifeq (${OS}, Darwin)
TSC_HEADERS += darwin/pthread_barrier.h \
				darwin/pthread_spinlock.h \
				darwin/amd64-ucontext.h \
				darwin/386-ucontext.h \ 
				darwin/power-ucontext.h 
TSC_CFILES += darwin/pthread_barrier.c darwin/ucontext.c
TSC_OBJS := darwin/asm.o

CFLAGS += -Idarwin
use_clang := 1
endif

ifeq (${OS}, Linux)
TSC_CFILES += netpoll_epoll.c
else
TSC_CFILES += netpoll_poll.c
endif

TSC_OBJS += $(subst .c,.o,$(TSC_CFILES))

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

ifneq (${enable_deadlock_detect}, 0)
	CFLAGS += -DENABLE_DEADLOCK_DETECT
	CFLAGS += -rdynamic
endif

all: libTSC.a

libTSC.a: $(TSC_OBJS)
	ar r $@ $(TSC_OBJS)

%.o:%.S
	$(CC) -c ${CFLAGS} $<

%.o:%.c
	$(CC) -c ${CFLAGS} $<

.PHONY:clean install
clean:
	rm -f *.o libTSC.a 

install: libTSC.a
	mkdir -p ../lib
	cp libTSC.a ../lib
