
APPS := findmax.run primes.run timeshare.run select.run
APPS += mandelbrot.run spectral-norm.run

OS := $(shell uname)

CC := gcc

TSC_HEADERS:= clock.h channel.h context.h lock.h queue.h support.h thread.h vpu.h 
TSC_CFILES := boot.c channel.c clock.c context.c thread.c vpu.c 

ifeq (${OS}, Darwin)
TSC_HEADERS += pthread_barrier.h pthread_spinlock.h amd64-ucontext.h 386-ucontext.h power-ucontext.h
TSC_CFILES += pthread_barrier.c ucontext.c
TSC_OBJS := asm.o
use_clang := 1
endif

TSC_OBJS += $(subst .c,.o,$(TSC_CFILES))

#CFLAGS += -DENABLE_QUICK_RESPONSE
CFLAGS += -DENABLE_WORKSTEALING
CFLAGS += -DENABLE_CHANNEL_SELECT

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

all: ${APPS}

libTSC.a: $(TSC_OBJS)
	ar r $@ $(TSC_OBJS)

%.o:%.S
	$(CC) -c ${CFLAGS} $<

%.o:%.c
	$(CC) -c ${CFLAGS} $<

%.run:%.o libTSC.a
	$(CC) $< ${CFLAGS} -L. -lTSC -lpthread -lm -o $@

.PHONY:clean
clean:
	rm -f *.o *.run libTSC.a 
