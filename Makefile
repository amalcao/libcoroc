.PHONY: all library app clean

all: library app

enable_timer ?= 0
enable_optimize ?= 0
enable_splitstack ?= 0
enable_deadlock_detect ?= 1
enable_refcnt ?= 1
use_clang ?= 0

ARGS := enable_timer=${enable_timer} \
	enable_optimize=${enable_optimize} \
	enable_splitstack=${enable_splitstack} \
	enable_deadlock_detect=${enable_deadlock_detect} \
	enable_refcnt=${enable_refcnt} \
	use_clang=${use_clang}


library:
	echo ${ARGS}
	cd src && make install $(ARGS)

app:
	cd examples && make install $(ARGS)

clean:
	cd src && make clean
	cd examples && make clean
	rm -rf bin lib
