set(TESTS
	usb
	sg
)

foreach(test ${TESTS})
	add_executable(${test} ${test}.c)
	add_dependencies(${test} ${PROJECT_NAME})
	target_link_libraries(${test} ${PROJECT_NAME})
	add_test(${test} ${CMAKE_CURRENT_BINARY_DIR}/${test})
endforeach()

add_executable(flash flash.c "${CMAKE_SOURCE_DIR}/src/tools/flash_opts.c")
target_link_libraries(flash ${PROJECT_NAME})
add_test(flash ${CMAKE_CURRENT_BINARY_DIR}/flash)
