
APPS := findmax.run timeshare.run select.run ticker.run file.run findmax_msg.run chan.run
APPS += primes.run httpload.run tcpproxy.run ## modify from examples in LibTask
APPS += mandelbrot.run spectral-norm.run ## modify from benchmarkgame.org

OS := $(shell uname)

CC ?= gcc-4.8


APP_CFLAGS := -Dmain=user_main -I../../include

APP_LDFLAGS := -L../../lib -lTSC -lpthread -lm 

ifneq (${OS}, Darwin)
	APP_LDFLAGS += -lrt
endif

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

CFLAGS += -DENABLE_REFCNT

ifeq (${enable_timer}, 1)
	CFLAGS += -DENABLE_TIMESHARE
endif

all: ${APPS}

%.o:%.S
	$(CC) -c ${CFLAGS} -Werror $<

%.o:%.c
	$(CC) -c ${CFLAGS} -Werror $<

%.run:%.c ../../lib/libTSC.a
	$(CC) $< ${CFLAGS} ${APP_CFLAGS} ${APP_LDFLAGS} -o $@

.PHONY:clean install
clean:
	rm -rf *.o *.run*

install: ${APPS}
	mkdir -p ../../bin
	cp ${APPS} ../../bin/
