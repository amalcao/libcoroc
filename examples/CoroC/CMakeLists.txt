SET(INCLUDE_FILES ${LibCoroC_SOURCE_DIR}/include/libcoroc.h)
SET(COROC_LIB coroc)
LINK_DIRECTORIES(${LibCoroC_SOURCE_DIR}/src)

SET(COROC_DIR ${LibCoroC_SOURCE_DIR}/examples/CoroC)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Dmain=user_main")



MACRO(add_libcoroc_coroc_example name)
    
    SET(COROC_COMMAND ${CLANG_COROC} 
                      "-rewrite-coroc"
                      "-I${LibCoroC_SOURCE_DIR}/include"
                      "-I${LibCoroC_SOURCE_DIR}/include/inter"
                      "-I${LibCoroC_BINARY_DIR}/include/inter"
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

    TARGET_LINK_LIBRARIES(
        ${name}_co.run
        ${COROC_LIB} pthread m
    )
 
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
