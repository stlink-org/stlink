###
# Build test executables
###

set(TESTEXEC usb sg)

foreach (test ${TESTEXEC})
    add_executable(test-${test} ${test}.c)
    add_dependencies(test-${test} ${STLINK_LIB_SHARED})
    target_link_libraries(test-${test} ${STLINK_LIB_SHARED} ${SSP_LIB})
    add_test(test-${test} ${CMAKE_BINARY_DIR}/bin/test-${test})
endforeach ()

add_executable(test-flash flash.c "${CMAKE_SOURCE_DIR}/src/st-flash/flash_opts.c")
target_link_libraries(test-flash ${STLINK_LIB_SHARED} ${SSP_LIB})
add_test(test-flash ${CMAKE_BINARY_DIR}/bin/test-flash)
