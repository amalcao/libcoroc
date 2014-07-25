
APPS := findmax_co.run primes_co.run
APPS += mandelbrot_co.run spectral-norm_co.run ## modify from benchmarkgame.org
APPS += httpload_co.run tcpproxy_co.run
APPS += ticker_co.run 

OS := $(shell uname)

COROC 	?= clang
CC  	?= gcc-4.8
CXX 	?= g++-4.8


APP_CFLAGS := -I../../include

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
	CFLAGS += -DENABLE_TIMER
endif

all: ${APPS}

%.o:%.S
	$(CC) -c ${CFLAGS} -Werror $<

%.o:%.c
	$(CC) -c ${CFLAGS} -Werror $<

%.cc:%.co
	$(COROC) -rewrite-coro-c ${APP_CFLAGS} $< 

%.run:%.cc ../../lib/libTSC.a
	$(CXX) $< ${CFLAGS} ${APP_CFLAGS} -L../../lib -lTSC -lpthread -lm -o $@

.PHONY:clean install
clean:
	rm -rf *.cc *.o *.run*

install: ${APPS}
	mkdir -p ../../bin
	cp ${APPS} ../../bin/
