###
# System-specific definitions, header files and libraries
###

# Check for system-specific additional header files and libraries
include(CheckIncludeFile)
include(CheckLibraryExists)

set(STLINK_DEFINITIONS)      # collect all system-specific compiler definitions here
set(STLINK_COMPILE_OPTIONS)  # collect all system-specific compiler flags here

# ====================================================================================================

## Windows-specific build configuration
if (WIN32)
    message(STATUS "Configuring for Windows (Windows 10+)")

    # Compiler definitions for Windows
    list(APPEND STLINK_DEFINITIONS 
        -D_WIN32_WINNT=0x0A00   # Minimum system requirement: Windows 10
        -D_WIN32=1              # Define _WIN32 for platform specific code
    )

    # Include directories containing windows-specific header files
    include_directories(src/stlink-env/win32)

    link_libraries(ws2_32)      # Link Windows-Socket library

    # TODO: Additional win32_socket ???
    list(APPEND STLINK_SOURCES src/stlink-env/win32/win32_socket.c)
    list(APPEND STLINK_HEADERS src/stlink-env/win32/win32_socket.h)

endif()

## Additional fixes for MSVC
if (MSVC)
    # Use string.h rather than strings.h and disable annoying warnings
    list(APPEND STLINK_DEFINITIONS
        -DHAVE_STRING_H
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS
        )
    list(APPEND STLINK_COMPILE_OPTIONS /wd4710)
endif()

# ====================================================================================================

### libusb

find_package(libusb REQUIRED)
list(APPEND STLINK_DEFINITIONS ${LIBUSB_DEFINITIONS}) # define ssize_t: only affects WIN32, MSVC & MINGW


### Stack Smashing Protection Library (SSP)

CHECK_LIBRARY_EXISTS(ssp __stack_chk_fail "" _stack_chk_fail_exists)
if (_stack_chk_fail_exists)
    if (WIN32)
        set(SSP_LIB -static ssp) # static linking
    else ()
        set(SSP_LIB ssp)
    endif()
else ()
    set(SSP_LIB "")
endif()


### Check for C standard library include files [POSIX]

CHECK_INCLUDE_FILE(sys/mman.h STLINK_HAVE_SYS_MMAN_H)
if (STLINK_HAVE_SYS_MMAN_H)
    list(APPEND STLINK_DEFINITIONS -DSTLINK_HAVE_SYS_MMAN_H)
else ()
    if (WIN32 OR MSVC OR MINGW)
        list(APPEND STLINK_SOURCES src/stlink-env/win32/sys_mmap.c)
        list(APPEND STLINK_HEADERS src/stlink-env/win32/sys_mmap.h)
    endif()
endif()

CHECK_INCLUDE_FILE(sys/time.h STLINK_HAVE_SYS_TIME_H)
if (STLINK_HAVE_SYS_TIME_H)
    list(APPEND STLINK_DEFINITIONS -DSTLINK_HAVE_SYS_TIME_H)
else ()
    if (WIN32 OR MSVC OR MINGW)
        list(APPEND STLINK_SOURCES src/stlink-env/win32/sys_time.c)
        list(APPEND STLINK_HEADERS src/stlink-env/win32/sys_time.h)
    endif()
endif()

CHECK_INCLUDE_FILE(unistd.h STLINK_HAVE_UNISTD_H)
if (STLINK_HAVE_UNISTD_H)
    list(APPEND STLINK_DEFINITIONS -DSTLINK_HAVE_UNISTD_H)
else ()
    if (WIN32 OR MSVC OR MINGW)
        list(APPEND STLINK_HEADERS src/stlink-env/win32/unistd.h)
    endif()
endif()


### Check for non C standard library include files [POSIX]

if (NOT WIN32)
    CHECK_INCLUDE_FILE(dirent.h STLINK_HAVE_DIRENT_H)
    if (STLINK_HAVE_DIRENT_H)
        list(APPEND STLINK_DEFINITIONS -DSTLINK_HAVE_DIRENT_H)
    else ()
        message(FATAL_ERROR "dirent.h not found! init_chipids can not be compiled for Unix/Linux.")
    endif()
else () # WIN32
    CHECK_INCLUDE_FILE(windows.h STLINK_HAVE_WINDOWS_H)
    message(STATUS "Windows platform detected. Using Windows API for init_chipids.")
    list(APPEND STLINK_DEFINITIONS -DSTLINK_HAVE_WINDOWS_H)
    # use GetModuleFileNameA and FindFirstFileA
endif()

# ====================================================================================================

### Propagate system-specific additional definitions, sources, headers and libraries to parent scope
set(STLINK_SOURCES ${STLINK_SOURCES} PARENT_SCOPE)
set(STLINK_HEADERS ${STLINK_HEADERS} PARENT_SCOPE)
set(STLINK_DEFINITIONS ${STLINK_DEFINITIONS} PARENT_SCOPE)
set(STLINK_COMPILE_OPTIONS ${STLINK_COMPILE_OPTIONS} PARENT_SCOPE)