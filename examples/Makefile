
APPS := findmax.run timeshare.run select.run ticker.run file.run findmax_msg.run chan.run
APPS += primes.run httpload.run tcpproxy.run ## modify from examples in LibTask
APPS += mandelbrot.run spectral-norm.run ## modify from benchmarkgame.org

OS := $(shell uname)

CC ?= gcc-4.8


APP_CFLAGS := -Dmain=user_main -I../include

ifeq (${use_clang}, 1)
	CC := clang
endif

ifeq (${enable_debug}, 0)
	CFLAGS += -O2
else
	CFLAGS += -g3
endif

ifeq (${enable_splitstack}, 1)
	CFLAGS += -DENABLE_SPLITSTACK
	CFLAGS += -fsplit-stack
endif

ifneq (${enable_daedlock_detect}, 0)
	CFLAGS += -DENABLE_DEADLOCK_DETECT
	CFLAGS += -rdynamic
endif

ifneq (${enable_refcnt}, 0)
	CFLAGS += -DENABLE_REFCNT
endif

ifeq (${enable_timer}, 1)
	CFLAGS += -DENABLE_TIMER
endif


ifeq (${enable_dist}, 1)
	APPS += dist/ping.run
	APPS += dist/pong.run
endif

all: ${APPS}

%.o:%.S
	$(CC) -c ${CFLAGS} -Werror $<

%.o:%.c
	$(CC) -c ${CFLAGS} -Werror $<

%.run:%.c ../lib/libTSC.a
	$(CC) $< ${CFLAGS} ${APP_CFLAGS} -L../lib -lTSC -lpthread -lm -o $@

.PHONY:clean install
clean:
	rm -rf *.o *.run*

install: ${APPS}
	mkdir -p ../bin
	cp ${APPS} ../bin/
