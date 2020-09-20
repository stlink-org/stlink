###
# General cmake settings
###

cmake_minimum_required(VERSION 3.4.2)
cmake_policy(SET CMP0042 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


###
# General project settings
###

project(stlink C)
set(PROJECT_DESCRIPTION "Open source version of the STMicroelectronics ST-LINK Tools")
include(GNUInstallDirs) # Define GNU standard installation directories

## Determine project version
include(${CMAKE_MODULE_PATH}/get_version.cmake)

## Set C build flags
if (NOT MSVC)
    include(${CMAKE_MODULE_PATH}/c_flags.cmake)
else ()
    message(STATUS "MSVC C Flags override to /MT")
    set(CMAKE_C_FLAGS_DEBUG_INIT          "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_C_FLAGS_MINSIZEREL_INIT     "/MT /O1 /Ob1 /D NDEBUG")
    set(CMAKE_C_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /D NDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /D NDEBUG")
endif ()


###
# Dependencies
###

find_package(libusb REQUIRED)

## Check for system-specific additional header files and libraries

include(CheckIncludeFile)
include(CheckLibraryExists)

CHECK_LIBRARY_EXISTS(ssp __stack_chk_fail "" _stack_chk_fail_exists)
if (_stack_chk_fail_exists)
    if (WIN32)
        set(SSP_LIB -static ssp)
    else ()
        set(SSP_LIB ssp)
    endif ()
else ()
    set(SSP_LIB "")
endif ()

CHECK_INCLUDE_FILE(sys/mman.h STLINK_HAVE_SYS_MMAN_H)
if (STLINK_HAVE_SYS_MMAN_H)
    add_definitions(-DSTLINK_HAVE_SYS_MMAN_H)
endif()

CHECK_INCLUDE_FILE(unistd.h STLINK_HAVE_UNISTD_H)
if (STLINK_HAVE_UNISTD_H)
    add_definitions(-DSTLINK_HAVE_UNISTD_H)
endif ()

if (MSVC)
    # Use string.h rather than strings.h and disable annoying warnings
    add_definitions(-DHAVE_STRING_H -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS /wd4710)
endif ()


###
# Main build process
###

## Define include directories to avoid absolute paths for header defines
include_directories(${LIBUSB_INCLUDE_DIR})

include_directories(${PROJECT_SOURCE_DIR}/inc) # contains top-level header files
include_directories(${PROJECT_BINARY_DIR}/inc) # contains version.h

include_directories(src)
include_directories(src/st-flash)
include_directories(src/stlink-lib)

## Set installation directory for header files
set(STLINK_INCLUDE_PATH ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME} CACHE PATH "Main include install directory")

## Subordinate CMakeLists for version config & header installation
add_subdirectory(inc)

## Define source- and headerfiles for stlink library
set(STLINK_HEADERS
        inc/backend.h
        inc/stlink.h
        src/stlink-lib/commands.h
        src/stlink-lib/libusb_settings.h
        src/stlink-lib/reg.h
        src/stlink-lib/chipid.h
        src/stlink-lib/flash_loader.h
        src/stlink-lib/logging.h
        src/stlink-lib/md5.h
        src/stlink-lib/sg.h
        src/stlink-lib/usb.h
        )

set(STLINK_SOURCE
        src/common.c
        src/stlink-lib/chipid.c
        src/stlink-lib/flash_loader.c
        src/stlink-lib/logging.c
        src/stlink-lib/md5.c
        src/stlink-lib/sg.c
        src/stlink-lib/usb.c
        )

if (WIN32)
    include_directories(src/win32)
    set(STLINK_SOURCE "${STLINK_SOURCE};src/win32/win32_socket.c")
    set(STLINK_HEADERS "${STLINK_HEADERS};src/win32/win32_socket.h")

    if (MSVC)
        # Add drop-in replacement for unistd.h to sources
        include_directories(src/win32/unistd)
        set(STLINK_HEADERS "${STLINK_HEADERS};src/win32/unistd/unistd.h")
    endif ()

    if (NOT STLINK_HAVE_SYS_MMAN_H)
        include_directories(src/win32/mmap)
        set(STLINK_SOURCE "${STLINK_SOURCE};src/win32/mmap.c")
        set(STLINK_HEADERS "${STLINK_HEADERS};src/win32/mmap.h")
    endif()
endif ()

## Include test execution for test-targets for target Debug
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    include(CTest)
endif ()


###
# Libraries
###

set(STLINK_LIBRARY_PATH ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Main library install directory")

# Set the environment variable LD_LIBRARY_PATH to point to /usr/local/lib (per default).
execute_process(COMMAND bash -c "export LD_LIBRARY_PATH=${CMAKE_INSTALL_LIBDIR}")


###
# Shared library
###

# Set library name
if (NOT WIN32)
    set(STLINK_LIB_SHARED ${PROJECT_NAME})
else (WIN32)
    set(STLINK_LIB_SHARED ${PROJECT_NAME}-shared)
endif ()

add_library(${STLINK_LIB_SHARED} SHARED ${STLINK_HEADERS} ${STLINK_SOURCE})

set(STLINK_SHARED_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
message(STATUS "STLINK_LIB_SHARED: ${STLINK_LIB_SHARED}")
message(STATUS "PROJECT_VERSION_MAJOR: ${PROJECT_VERSION_MAJOR}")
message(STATUS "VERSION: ${STLINK_SHARED_VERSION}")

set_target_properties(${STLINK_LIB_SHARED} PROPERTIES
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${STLINK_SHARED_VERSION}
        OUTPUT_NAME ${PROJECT_NAME}
        )

# Link shared library
if (APPLE)     # ... with Apple macOS libraries
    find_library(ObjC objc)
    find_library(CoreFoundation CoreFoundation)
    find_library(IOKit IOKit)
    target_link_libraries(${STLINK_LIB_SHARED} ${LIBUSB_LIBRARY} ${SSP_LIB} ${ObjC} ${CoreFoundation} ${IOKit})
elseif (WIN32) # ... with Windows libraries
    target_link_libraries(${STLINK_LIB_SHARED} ${LIBUSB_LIBRARY} ${SSP_LIB} wsock32 ws2_32)
else ()
    target_link_libraries(${STLINK_LIB_SHARED} ${LIBUSB_LIBRARY} ${SSP_LIB})
endif ()

install(TARGETS ${STLINK_LIB_SHARED} DESTINATION ${STLINK_LIBRARY_PATH})


###
# Static library
###

# Set library name
set(STLINK_LIB_STATIC ${PROJECT_NAME}-static)

add_library(${STLINK_LIB_STATIC} STATIC ${STLINK_HEADERS} ${STLINK_SOURCE})

set(STLINK_STATIC_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
message(STATUS "STLINK_LIB_STATIC: ${STLINK_LIB_STATIC}")
message(STATUS "PROJECT_VERSION_MAJOR: ${PROJECT_VERSION_MAJOR}")
message(STATUS "VERSION: ${STLINK_STATIC_VERSION}")

set_target_properties(${STLINK_LIB_STATIC} PROPERTIES
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${STLINK_STATIC_VERSION}
        OUTPUT_NAME ${PROJECT_NAME}
        )

# Link static library
if (APPLE)     # ... with Apple macOS libraries
    find_library(ObjC objc)
    find_library(CoreFoundation CoreFoundation)
    find_library(IOKit IOKit)
    target_link_libraries(${STLINK_LIB_STATIC} ${LIBUSB_LIBRARY} ${SSP_LIB} ${ObjC} ${CoreFoundation} ${IOKit})
elseif (WIN32) # ... with Windows libraries
    target_link_libraries(${STLINK_LIB_STATIC} ${LIBUSB_LIBRARY} ${SSP_LIB} wsock32 ws2_32)
else ()
    target_link_libraries(${STLINK_LIB_STATIC} ${LIBUSB_LIBRARY} ${SSP_LIB})
endif ()

install(TARGETS ${STLINK_LIB_STATIC} ARCHIVE DESTINATION ${STLINK_LIBRARY_PATH})


###
# Build toolset executables
###

set(ST-FLASH_SOURCES src/st-flash/flash.c src/st-flash/flash_opts.c)
set(ST-INFO_SOURCES src/st-info/info.c)
set(ST-UTIL_SOURCES src/st-util/gdb-remote.c src/st-util/gdb-server.c src/st-util/semihosting.c)

if (MSVC)
    # Add getopt to sources
    include_directories(src/win32/getopt)
    set(ST-UTIL_SOURCES "${ST-UTIL_SOURCES};src/win32/getopt/getopt.c")
endif ()

add_executable(st-flash ${ST-FLASH_SOURCES})
add_executable(st-info ${ST-INFO_SOURCES})
add_executable(st-util ${ST-UTIL_SOURCES})

if (WIN32 OR APPLE)
    target_link_libraries(st-flash ${STLINK_LIB_STATIC} ${SSP_LIB})
    target_link_libraries(st-info ${STLINK_LIB_STATIC} ${SSP_LIB})
    target_link_libraries(st-util ${STLINK_LIB_STATIC} ${SSP_LIB})
else ()
    target_link_libraries(st-flash ${STLINK_LIB_SHARED} ${SSP_LIB})
    target_link_libraries(st-info ${STLINK_LIB_SHARED} ${SSP_LIB})
    target_link_libraries(st-util ${STLINK_LIB_SHARED} ${SSP_LIB})
endif ()

install(TARGETS st-flash DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-info DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-util DESTINATION ${CMAKE_INSTALL_BINDIR})


###
# Device configuration (Linux only)
###

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    ## Install modprobe.d conf files to /etc/modprobe.d/ (explicitly hardcoded)
    set(STLINK_MODPROBED_DIR "/etc/modprobe.d" CACHE PATH "modprobe.d directory")
    install(FILES ${CMAKE_SOURCE_DIR}/config/modprobe.d/stlink_v1.conf DESTINATION ${STLINK_MODPROBED_DIR})

    ## Install udev rules files to /lib/udev/rules.d/ (explicitly hardcoded)
    set(STLINK_UDEV_RULES_DIR "/lib/udev/rules.d" CACHE PATH "udev rules directory")
    file(GLOB RULES_FILES ${CMAKE_SOURCE_DIR}/config/udev/rules.d/*.rules)
    install(FILES ${RULES_FILES} DESTINATION ${STLINK_UDEV_RULES_DIR})
endif ()


###
# Additional build tasks
###

add_subdirectory(src/stlink-gui)  # contains subordinate CMakeLists to build GUI
add_subdirectory(tests)           # contains subordinate CMakeLists to build test executables
add_subdirectory(cmake/packaging) # contains subordinate CMakeLists to build packages

option(STLINK_GENERATE_MANPAGES "Generate manpages with pandoc" OFF)
add_subdirectory(doc/man)         # contains subordinate CMakeLists to generate manpages


###
# Uninstall target
###

if (NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
        )
    add_custom_target(
        uninstall COMMAND ${CMAKE_COMMAND}
        -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake
        )
endif ()
