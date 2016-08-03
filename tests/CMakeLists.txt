set(TESTS
	usb
	sg
)

foreach(test ${TESTS})
	add_executable(${test} ${test}.c)
	add_dependencies(${test} ${PROJECT_NAME})
	target_link_libraries(${test} ${PROJECT_NAME})
endforeach()
