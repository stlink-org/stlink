include(CheckCCompilerFlag)

function(add_cflag_if_supported flag)
    string(REPLACE "-" "_" flagclean ${flag})
    string(REPLACE "=" "_" flagclean ${flagclean})
    string(REPLACE "+" "_" flagclean ${flagclean})
    string(REPLACE "," "_" flagclean ${flagclean})
    string(TOUPPER ${flagclean} flagclean)

    check_c_compiler_flag(${flag} C_SUPPORTS${flagclean})

    if (C_SUPPORTS${flagclean})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
endfunction()

add_cflag_if_supported("-std=gnu99")
add_cflag_if_supported("-Wall")
add_cflag_if_supported("-Wextra")
add_cflag_if_supported("-Wshadow")
add_cflag_if_supported("-D_FORTIFY_SOURCE=2")
add_cflag_if_supported("-fstrict-aliasing")
add_cflag_if_supported("-Wundef")
add_cflag_if_supported("-Wformat")
add_cflag_if_supported("-Wformat-security")
add_cflag_if_supported("-Wmaybe-uninitialized")
add_cflag_if_supported("-Wmissing-variable-declarations")
add_cflag_if_supported("-Wshorten-64-to-32")
add_cflag_if_supported("-Wimplicit-function-declaration")

##
# On OpenBSD the system headers suck so we need to disable redundant declaration check
# /usr/include/unistd.h:429: warning: redundant redeclaration of 'truncate'
# /usr/include/sys/types.h:218: warning: previous declaration of 'truncate' was here
##
if (NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
	add_cflag_if_supported("-Wredundant-decls")
endif ()

if (NOT WIN32)
	add_cflag_if_supported("-fPIC")
endif ()

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	add_cflag_if_supported("-ggdb")
	add_cflag_if_supported("-O0")
else()
	add_cflag_if_supported("-O2")
    add_cflag_if_supported("-Werror")
endif()
