add_executable(st-util gdb-remote.c
                       gdb-remote.h
                       gdb-server.c
                       gdb-server.h
                       semihosting.c
                       semihosting.h)
target_link_libraries(st-util ${PROJECT_NAME})
install(TARGETS st-util
	RUNTIME DESTINATION bin
)
