set(STUTIL_SOURCE
    gdb-remote.c
    gdb-remote.h
    gdb-server.c
    gdb-server.h
    semihosting.c
    semihosting.h)

if (MSVC)
    # We need a getopt from somewhere...
    set(STUTIL_SOURCE "${STUTIL_SOURCE};../getopt/getopt.c")
    # Use string.h rather than strings.h and disable annoying warnings
    add_definitions(-DHAVE_STRING_H -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS /wd4710)
endif()

add_executable(st-util ${STUTIL_SOURCE})

if (WIN32 OR APPLE)
	target_link_libraries(st-util ${STLINK_LIB_STATIC})
else()
	target_link_libraries(st-util ${STLINK_LIB_SHARED})
endif()

install(TARGETS st-util
	RUNTIME DESTINATION bin
)
