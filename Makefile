all: libTSC.a

TSC_HEADERS:= clock.h context.h lock.h queue.h support.h thread.h vpu.h
TSC_CFILES := boot.c clock.c context.c thread.c vpu.c
TSC_OBJS := $(subst .c,.o,$(TSC_CFILES))

%.o:%.S
	gcc -c -g3 $<

%.o:%.c
	gcc -c -g3 $<

app: app.o libTSC.a
	gcc app.o -L. -lTSC -lpthread -o $@

timer: app.o libTSC.a
	gcc app.o  -DENABLE_TIMER -L. -lTSC -lpthread -o $@

libTSC.a: $(TSC_OBJS)
	ar r $@ $(TSC_OBJS)

.PHONY:clean
clean:
	rm -f *.o libTSC.a app timer
