add_executable(st-util gdb-remote.c
                       gdb-remote.h
                       gdb-server.c
                       gdb-server.h
                       semihosting.c
                       semihosting.h)
if (WIN32 OR APPLE)
	target_link_libraries(st-util ${STLINK_LIB_STATIC})
else()
	target_link_libraries(st-util ${STLINK_LIB_SHARED})
endif()

install(TARGETS st-util
	RUNTIME DESTINATION bin
)
