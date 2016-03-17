SET(LIBRARY_NAME coroc)
    
ENABLE_LANGUAGE(C ASM)

SET(INCLUDE_FILES ../include/inter/support.h
                  ../include/inter/coroutine.h
                  ../include/inter/channel.h
                  ../include/inter/message.h
                  ../include/inter/context.h
                  ../include/inter/vpu.h
                  ../include/inter/netpoll.h
                  ../include/inter/async.h
                  ../include/inter/refcnt.h
                  ../include/inter/coroc_clock.h
                  ../include/inter/coroc_lock.h
                  ../include/inter/coroc_queue.h
                  ../include/inter/coroc_hash.h
                  ../include/inter/coroc_time.h
                  ../include/inter/coroc_group.h)

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
              coroc_main.c
              hash.c
              vfs.c)

IF(APPLE)
    SET(SRC_FILES ${SRC_FILES}
                  darwin/ucontext.c
                  darwin/pthread_barrier.c
                  darwin/asm.S)
    SET(INCLUDE_FILES ${INCLUDE_FILES}
                      ../include/inter/darwin/pthread_barrier.h
                      ../include/inter/darwin/pthread_spinlock.h
                      ../include/inter/darwin/time_.h)
    
    IF(${ARCH} MATCHES "x86_64")
        SET(INCLUDE_FILES ${INCLUDE_FILES}
                          ../include/inter/darwin/amd64-ucontext.h)
    ELSEIF(${ARCH} MATCHES "x86")
        SET(INCLUDE_FILES ${INCLUDE_FILES}
                          ../include/inter/darwin/386-ucontext.h)
    ELSE()
        SET(INCLUDE_FILES ${INCLUDE_FILES}
                          ../include/inter/darwin/power-ucontext.h)
    ENDIF()
ENDIF(APPLE)

IF(CMAKE_SYSTEM MATCHES "Linux")
    SET(SRC_FILES ${SRC_FILES}
                  netpoll_epoll.c 
                  futex_impl.c)

    IF(ENABLE_NOTIFY)
      SET(SRC_FILES ${SRC_FILES} notify.c)
    ENDIF(ENABLE_NOTIFY)
    
    IF(ENABLE_FUTEX)
        SET(SRC_FILES ${SRC_FILES} futex_lock.c)
    ENDIF(ENABLE_FUTEX)

ELSE()
    IF(APPLE)
        SET(SRC_FILES ${SRC_FILES} netpoll_kqueue.c)
    ELSE()
        SET(SRC_FILES ${SRC_FILES} netpoll_poll.c)
    ENDIF(APPLE)
ENDIF()

ADD_LIBRARY(
    ${LIBRARY_NAME}
    STATIC
    ${SRC_FILES}
    ${INCLUDE_FILES}
)

INSTALL_TARGETS(/lib ${LIBRARY_NAME})
