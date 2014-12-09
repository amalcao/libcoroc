
IF(WIN32)
    MESSAGE(FATAL_ERROR "LibCoroC not support Windows now!")
ELSE(WIN32)
    CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0 FATAL_ERROR)
ENDIF(WIN32)

PROJECT(LibCoroC)

INCLUDE_DIRECTORIES(${LibCoroC_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${LibCoroC_SOURCE_DIR}/include/inter)

## Option to enable Split-stack
OPTION(ENABLE_SPLITSTACK OFF)
IF(ENABLE_SPLITSTACK)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsplit-stack")
ENDIF(ENABLE_SPLITSTACK)

## Option to enable time-sharing schedule
OPTION(ENABLE_TIMESHARE OFF)

## Option to enable deadlock detect
OPTION(ENABLE_DEADLOCK_DETECT ON)
IF(ENABLE_DEADLOCK_DETECT AND CMAKE_SYSTEM MATCHES "Linux")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -rdynamic")
ENDIF()


## Option to enable futex-based lock
OPTION(ENABLE_FUTEX OFF)
IF(ENABLE_FUTEX)
    IF(NOT CMAKE_SYSTEM MATCHES "Linux")
        MESSAGE(FATAL_ERROR "FUTEX is only supported on Linux!")
    ENDIF()
ENDIF(ENABLE_FUTEX)

## Option to enable the work-stealing
OPTION(ENABLE_WORKSTEALING ON)

## The config header auto-generating in config time
CONFIGURE_FILE(include/inter/config.h.in include/inter/config.h)
ADD_DEFINITIONS(-DUSE_CMAKE)

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
