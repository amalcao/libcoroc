SET(LIBRARY_NAME coroc)

SET(INCLUDE_FILES ../include/inter/support.h
                  ../include/inter/coroutine.h
                  ../include/inter/channel.h
                  ../include/inter/message.h
                  ../include/inter/context.h
                  ../include/inter/vpu.h
                  ../include/inter/netpoll.h
                  ../include/inter/async.h
                  ../include/inter/refcnt.h
                  ../include/inter/tsc_clock.h
                  ../include/inter/tsc_lock.h
                  ../include/inter/tsc_queue.h
                  ../include/inter/tsc_hash.h
                  ../include/inter/tsc_time.h
                  ../include/inter/tsc_group.h)

SET(SRC_FILES boot.c 
              vpu.c 
              coroutine.c 
              context.c
              channel.c 
              message.c
              time.c
              netpoll.c
              net.c
              async.c
              group.c
              clock.c
              tsc_main.c
              hash.c
              vfs.c)

IF(APPLE)
    SET(SRC_FILES ${SRC_FILES}
                  darwin/ucontext.c
                  darwin/pthread_barrier.c
                  darwin/asm.S)
    SET(INCLUDE_FILES ${INCLUDE_FILES}
                      ../include/inter/darwin/*)
ENDIF(APPLE)

IF(CMAKE_SYSTEM MATCHES "Linux")
    SET(SRC_FILES ${SRC_FILES}
                  netpoll_epoll.c 
                  futex_impl.c
                  notify.c)

    ADD_DEFINITIONS(-DENABLE_NOTIFY)
    
    IF(ENABLE_FUTEX)
        SET(SRC_FILES ${SRC_FILES} futex_lock.c)
    ENDIF(ENABLE_FUTEX)

ELSE()
    SET(SRC_FILES ${SRC_FILES} netpoll_poll.c)
ENDIF()

ADD_LIBRARY(
    ${LIBRARY_NAME}
    STATIC
    ${SRC_FILES}
    ${INCLUDE_FILES}
)

INSTALL_TARGETS(/lib ${LIBRARY_NAME})
