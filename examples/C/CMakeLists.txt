SET(INCLUDE_FILES ${LibCoroC_SOURCE_DIR}/include/libtsc.h)
SET(COROC_LIB coroc)
LINK_DIRECTORIES(${LibCoroC_SOURCE_DIR}/src)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Dmain=user_main")

MACRO(add_libcoroc_c_example name)
  ADD_EXECUTABLE(
        ${name}.run
        ${name}.c
        ${INCLUDE_FILES}
  )
     
  IF(LIB_TCMALLOC)
     TARGET_LINK_LIBRARIES(
        ${name}.run
        ${COROC_LIB} ${LIB_TCMALLOC} pthread m
     )
  ELSE(LIB_TCMALLOC)
     TARGET_LINK_LIBRARIES(
        ${name}.run
        ${COROC_LIB} pthread m
     )
  ENDIF(LIB_TCMALLOC)
ENDMACRO(add_libcoroc_c_example)

add_libcoroc_c_example(chan)
add_libcoroc_c_example(findmax)
add_libcoroc_c_example(findmax_msg)
add_libcoroc_c_example(file)
add_libcoroc_c_example(httpload)
add_libcoroc_c_example(mandelbrot)
add_libcoroc_c_example(primes)
add_libcoroc_c_example(select)
add_libcoroc_c_example(spectral-norm)
add_libcoroc_c_example(tcpproxy)
add_libcoroc_c_example(ticker)

IF(ENABLE_TIMESHARE)
    add_libcoroc_c_example(timeshare)
ENDIF(ENABLE_TIMESHARE)                  
