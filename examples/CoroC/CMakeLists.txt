SET(INCLUDE_FILES ${LibCoroC_SOURCE_DIR}/include/libcoroc.h)
SET(COROC_LIB coroc)
LINK_DIRECTORIES(${LibCoroC_SOURCE_DIR}/src)

SET(COROC_DIR ${LibCoroC_SOURCE_DIR}/examples/CoroC)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Dmain=user_main")

SET(COROC_FLAGS "-rewrite-coroc"
                "-I${LibCoroC_SOURCE_DIR}/include"
                "-I${LibCoroC_SOURCE_DIR}/include/inter"
                "-I${LibCoroC_BINARY_DIR}/include/inter")

## Fix this for OS X Yosemite.
IF(APPLE)
  SET(COROC_FLAGS "-isysroot"
                  ${CMAKE_OSX_SYSROOT}
                  ${COROC_FLAGS})
ENDIF(APPLE)

MACRO(add_libcoroc_coroc_example name)
    
  SET(COROC_COMMAND ${CLANG_COROC} 
                    ${COROC_FLAGS}
                    ${COROC_DIR}/${name}_co.co)

  ADD_CUSTOM_COMMAND(
      OUTPUT ${name}_co.c
      PRE_BUILD
      COMMAND ${COROC_COMMAND}
      DEPENDS ${name}_co.co)

  ADD_EXECUTABLE(
      ${name}_co.run
      ${name}_co.c
      ${INCLUDE_FILES}
  )

  IF(LIB_TCMALLOC)
     TARGET_LINK_LIBRARIES(
        ${name}_co.run
        ${COROC_LIB} ${LIB_TCMALLOC} pthread m
     )
  ELSE(LIB_TCMALLOC)
     TARGET_LINK_LIBRARIES(
        ${name}_co.run
        ${COROC_LIB} pthread m
     )
  ENDIF(LIB_TCMALLOC)

ENDMACRO(add_libcoroc_coroc_example)

add_libcoroc_coroc_example(findmax)
add_libcoroc_coroc_example(file)
add_libcoroc_coroc_example(httpload)
add_libcoroc_coroc_example(mandelbrot)
add_libcoroc_coroc_example(primes)
add_libcoroc_coroc_example(select)
add_libcoroc_coroc_example(spectral-norm)
add_libcoroc_coroc_example(tcpproxy)
add_libcoroc_coroc_example(ticker)
add_libcoroc_coroc_example(smart-pointer)
add_libcoroc_coroc_example(smart-pointer-2)
