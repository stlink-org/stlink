###
# Build test executables
###

set(TESTEXEC usb sg_legacy)

if (WIN32)
    set(TEST_DEPENDENCY ${STLINK_LIB_STATIC})
else ()
    set(TEST_DEPENDENCY ${STLINK_LIB_SHARED})
endif()

foreach (test ${TESTEXEC})
    add_executable(test-${test} ${test}.c)
    add_dependencies(test-${test} ${TEST_DEPENDENCY})
    target_link_libraries(test-${test} ${TEST_DEPENDENCY} ${SSP_LIB})
    add_test(test-${test} ${CMAKE_BINARY_DIR}/bin/test-${test})
endforeach ()

add_executable(test-flash flash.c "${CMAKE_SOURCE_DIR}/src/st-flash/flash_opts.c")
target_include_directories(test-flash PRIVATE ${CMAKE_SOURCE_DIR}/src/st-flash)
add_dependencies(test-flash ${TEST_DEPENDENCY})
target_link_libraries(test-flash ${TEST_DEPENDENCY} ${SSP_LIB})
add_test(test-flash ${CMAKE_BINARY_DIR}/bin/test-flash)
