
APPS := findmax_co.run primes_co.run
APPS += mandelbrot_co.run spectral-norm_co.run ## modify from benchmarkgame.org
APPS += httpload_co.run tcpproxy_co.run
APPS += ticker_co.run select_co.run
APPS += group_co.run file_co.run

OS := $(shell uname)

COROC 	?= clang-co
CC  	?= gcc-4.8
FORMATER?= clang-format


APP_CFLAGS := -I../../include #-Wno-return-type

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
	CFLAGS += -DENABLE_TIMER
endif

all: ${APPS}

%.o:%.S
	$(CC) -c ${CFLAGS} -Werror $<

%.c:%.co
	$(COROC) -rewrite-coroc ${APP_CFLAGS} $< 
	## $(FORMATER) -i -style=LLVM $@

%.run:%.c ../../lib/libTSC.a
	$(CC) $< ${CFLAGS} ${APP_CFLAGS} ${APP_LDFLAGS} -o $@

.PHONY:clean install
clean:
	rm -rf *.c *.o *.run*

install: ${APPS}
	mkdir -p ../../bin
	cp ${APPS} ../../bin/
