
IF(WIN32)
    MESSAGE(FATAL_ERROR "LibCoroC not support Windows now!")
ELSE(WIN32)
    CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0 FATAL_ERROR)
ENDIF(WIN32)

PROJECT(LibCoroC)

## Get the architecture of current platform
EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCH)
MESSAGE(STATUS "Architecture: ${ARCH}")

## Default include path:
INCLUDE_DIRECTORIES(${LibCoroC_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${LibCoroC_SOURCE_DIR}/include/inter)

## Check the version of GCC to decide if it support `-fsplit-stack'
## option or not.
MACRO(SET_SPLITSTACK_FLAGS)
    IF(${CMAKE_C_COMPILER_ID} MATCHES "GNU")
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    
        IF(NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
            MESSAGE(FATAL_ERROR "\"split-stack\" requires gcc 4.7 or greater.")
        ELSE()
            SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsplit-stack")
        ENDIF()
    ELSE()
        MESSAGE(FATAL_ERROR "\"split-stack\" requires gcc 4.7 or greater.")
    ENDIF()
ENDMACRO()

## Option to enable Split-stack
OPTION(ENABLE_SPLITSTACK "Enable the split-stack for coroutines" OFF)
IF(ENABLE_SPLITSTACK)
    IF(CMAKE_SYSTEM MATCHES "Linux")
        IF(${ARCH} MATCHES "x86.*")
            SET_SPLITSTACK_FLAGS()
        ELSE()
            MESSAGE(FATAL_ERROR "split-stack only supported on x86/amd64 architures!")
        ENDIF()
    ELSE()
        MESSAGE(FATAL_ERROR "split-stack only supported on Linux!")
    ENDIF()
ENDIF(ENABLE_SPLITSTACK)

## Option to enable time-sharing schedule
OPTION(ENABLE_TIMESHARE "Enable the time-sharing scheduler" OFF)

## Option to enable deadlock detect
OPTION(ENABLE_DEADLOCK_DETECT "Enable deadlock detection at runtime" ON)
IF(ENABLE_DEADLOCK_DETECT AND CMAKE_SYSTEM MATCHES "Linux")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -rdynamic")
ENDIF()

OPTION(ENABLE_NOTIFY "Enable kernel notify (Linux only)" ON)

## Option to enable futex-based lock
OPTION(ENABLE_FUTEX "Enable futex based locks (Linux only)" OFF)
IF(ENABLE_FUTEX OR ENABLE_NOTIFY)
    IF(NOT CMAKE_SYSTEM MATCHES "Linux")
        MESSAGE(FATAL_ERROR "FUTEX / NOTIFY is only supported on Linux!")
    ENDIF()
ENDIF(ENABLE_FUTEX OR ENABLE_NOTIFY)

## Option to enable the lock-free scheduler
OPTION(ENABLE_LOCKFREE_RUNQ "Enable the lock-free scheduler" OFF)

## Option to enable the work-stealing
OPTION(ENABLE_WORKSTEALING "Enable the work-stealing scheduler" ON)

IF(ENABLE_LOCKFREE_RUNQ)
    IF(NOT ENABLE_WORKSTEALING)
        MESSAGE(FATAL_ERROR "Must enable work-stealing when using lock-free scheduler!")
    ENDIF(NOT ENABLE_WORKSTEALING)
ENDIF(ENABLE_LOCKFREE_RUNQ)

## Option to use the tc-malloc 
OPTION(ENABLE_TCMALLOC "Using tc-malloc for memory management" OFF)
IF(ENABLE_TCMALLOC)
    FIND_LIBRARY(LIB_TCMALLOC "libtcmalloc_minimal")
    IF(LIB_TCMALLOC)
      MESSAGE(STATUS "found libtcmalloc as ${LIB_TCMALLOC}")
    ELSE()
      MESSAGE(FATAL_ERROR "libtcmalloc not found")
    ENDIF()
ENDIF(ENABLE_TCMALLOC)

## The config header auto-generating in config time
CONFIGURE_FILE(include/inter/config.h.in include/inter/config.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/include/inter)

##########################################
## Optional build components

# C Examples
OPTION(BUILD_C_EXAMPLES "Enable to build C Examples" ON)

# CoroC Examples
OPTION(BUILD_COROC_EXAMPLES "Enable to build CoroC Examples" OFF)

# Core Library
ADD_SUBDIRECTORY(src)

IF(BUILD_C_EXAMPLES)
    ADD_SUBDIRECTORY(examples/C)
ENDIF(BUILD_C_EXAMPLES)

IF(BUILD_COROC_EXAMPLES)
    FIND_PROGRAM(CLANG_COROC "clang-co")
    IF(CLANG_COROC)
        MESSAGE(STATUS "Found CLANG_COROC as ${CLANG_COROC}")
        ADD_SUBDIRECTORY(examples/CoroC)
    ELSE(CLANG_COROC)
        MESSAGE(FATAL_ERROR "clang-co not found -- ${CLANG_COROC}")
    ENDIF(CLANG_COROC)
ENDIF(BUILD_COROC_EXAMPLES)

ADD_SUBDIRECTORY(include)

## install the auto-generated config header
INSTALL_FILES(/include/inter FILES ${CMAKE_CURRENT_BINARY_DIR}/include/inter/config.h)
