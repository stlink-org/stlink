###
# General cmake settings
###

cmake_minimum_required(VERSION 3.20)

# Enable modern install destination normalization (fixes CMP0177 warning)
cmake_policy(SET CMP0063 NEW)
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

###
#
# Default cmake directories:
#
# | Target Type             | GNUInstallDirs Variable        | Built-In Default      |
# | ---                     | ---                            | ---                   |
# | RUNTIME                 | ${CMAKE_INSTALL_BINDIR}        | bin                   |
# | LIBRARY                 | ${CMAKE_INSTALL_LIBDIR}        | lib                   |
# | ARCHIVE                 | ${CMAKE_INSTALL_LIBDIR}        | lib                   |
# | PRIVATE_HEADER          | ${CMAKE_INSTALL_INCLUDEDIR}    | include               |
# | PUBLIC_HEADER           | ${CMAKE_INSTALL_INCLUDEDIR}    | include               |
# | FILE_SET (type HEADERS) | ${CMAKE_INSTALL_INCLUDEDIR}    | include               |
#
# | TYPE Argument           | GNUInstallDirs Variable        | Built-In Default      |
# | ---                     | ---                            | ---                   |
# | BIN                     | ${CMAKE_INSTALL_BINDIR}        | bin                   |
# | SBIN                    | ${CMAKE_INSTALL_SBINDIR}       | sbin                  |
# | LIB                     | ${CMAKE_INSTALL_LIBDIR}        | lib                   |
# | INCLUDE                 | ${CMAKE_INSTALL_INCLUDEDIR}    | include               |
# | SYSCONF                 | ${CMAKE_INSTALL_SYSCONFDIR}    | etc                   |
# | SHAREDSTATE             | ${CMAKE_INSTALL_SHARESTATEDIR} | com                   |
# | LOCALSTATE              | ${CMAKE_INSTALL_LOCALSTATEDIR} | var                   |
# | RUNSTATE                | ${CMAKE_INSTALL_RUNSTATEDIR}   | <LOCALSTATE dir>/run  |
# | DATA                    | ${CMAKE_INSTALL_DATADIR}       | <DATAROOT dir>        |
# | INFO                    | ${CMAKE_INSTALL_INFODIR}       | <DATAROOT dir>/info   |
# | LOCALE                  | ${CMAKE_INSTALL_LOCALEDIR}     | <DATAROOT dir>/locale |
# | MAN                     | ${CMAKE_INSTALL_MANDIR}        | <DATAROOT dir>/man    |
# | DOC                     | ${CMAKE_INSTALL_DOCDIR}        | <DATAROOT dir>/doc    |
#
# ${CMAKE_BINARY_DIR}
# This is the full path to the top level of the current CMake build tree.
# For an in-source build, this would be the same as CMAKE_SOURCE_DIR.
#
# ${CMAKE_SOURCE_DIR}
# This is the full path to the top level of the current CMake source tree.
# For an in-source build, this would be the same as CMAKE_BINARY_DIR.
#
# ${CMAKE_CURRENT_BINARY_DIR}
# The path to the binary directory currently being processed.
# This is the full path to the build directory that is currently being processed by cmake.
# Each directory added by add_subdirectory() will create a binary directory in the build tree,
# and as it is being processed this variable will be set.
# For in-source builds this is the current source directory being processed.
#
# ${CMAKE_CURRENT_SOURCE_DIR}
# The path to the source directory currently being processed.
# This is the full path to the source directory that is currently being processed by cmake.
#
###


###
# General Project Settings
###

project(stlink C)
set(PROJECT_DESCRIPTION "Open source version of the STMicroelectronics ST-LINK Tools")
include(${CMAKE_MODULE_PATH}/get_version.cmake) # Determine project version

include(GNUInstallDirs) # Define GNU standard installation directories

## Include test execution for test-targets for target Debug
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    include(CTest)
endif()

# Check operating system
cmake_host_system_information(RESULT OS_NAME QUERY OS_NAME)
message(STATUS "Checking for OS_NAME: ${OS_NAME}")

# Threading support
find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON) # pthreads on Unix, winpthread on WIN32, MSVC & MINGW

# Set C build flags
if (NOT MSVC)
    include(${CMAKE_MODULE_PATH}/c_flags.cmake)
else ()
    message(STATUS "MSVC C Flags override to /MT")
    set(CMAKE_C_FLAGS_DEBUG_INIT          "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_C_FLAGS_MINSIZEREL_INIT     "/MT /O1 /Ob1 /D NDEBUG")
    set(CMAKE_C_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /D NDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /D NDEBUG")
endif()


###
# Dependencies
###

# Set source and header file variables
set(STLINK_HEADERS)
set(STLINK_SOURCES)

# Subordinate CMakeLists
# for system-specific definitions, external libraries, source and header files
add_subdirectory(src/stlink-env)
include_directories(src/stlink-env)
add_definitions(${STLINK_DEFINITIONS})
add_compile_options(${STLINK_COMPILE_OPTIONS})


###
# Main build process
###

# Set system-specific installation directory for header files
if (WIN32)
    set(STLINK_INCLUDE_PATH ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Main include install directory")
else ()
    set(STLINK_INCLUDE_PATH ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME} CACHE PATH "Main include install directory")
endif()

# Define include directories to avoid absolute paths for header defines
include_directories(${LIBUSB_INCLUDE_DIR})

# Include directories containing header files
include_directories(src/stlink-lib)
include_directories(inc)                        # contains top-level header files

# Project version file
include_directories(${PROJECT_BINARY_DIR}/inc)  # contains version.h
configure_file("${PROJECT_SOURCE_DIR}/inc/version.h.in"
               "${CMAKE_BINARY_DIR}/inc/version.h")

# Define source- and headerfiles for stlink library
file(GLOB STLINK_LIB_HEADERS
    "${CMAKE_SOURCE_DIR}/inc/*.h"
    "${CMAKE_SOURCE_DIR}/src/stlink-lib/*.h"
    "${CMAKE_BINARY_DIR}/inc/version.h")
list(APPEND STLINK_HEADERS ${STLINK_LIB_HEADERS})

file(GLOB STLINK_LIB_SOURCES
    "${CMAKE_SOURCE_DIR}/src/stlink-lib/*.c")
list(APPEND STLINK_SOURCES ${STLINK_LIB_SOURCES})

# Install library header files
install(FILES ${STLINK_HEADERS} DESTINATION ${STLINK_INCLUDE_PATH})


###
# Shared library
###

# Set library name
set(STLINK_LIB_SHARED ${PROJECT_NAME}-shared)

add_library(${STLINK_LIB_SHARED} SHARED ${STLINK_HEADERS} ${STLINK_SOURCES})

set(STLINK_SHARED_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
message(STATUS "STLINK_LIB_SHARED: ${STLINK_LIB_SHARED}")
message(STATUS "VERSION: ${STLINK_SHARED_VERSION}")

set_target_properties(${STLINK_LIB_SHARED} PROPERTIES
                        SOVERSION ${PROJECT_VERSION_MAJOR}
                        VERSION ${STLINK_SHARED_VERSION}
                        OUTPUT_NAME ${PROJECT_NAME}
                        )

# Since we're not using __declspec(dllexport), WINDOWS_EXPORT_ALL_SYMBOLS is mandatory for MSVC.
# This call will only affect MSVC, compilers/OSes ignore it.
set_target_properties(${STLINK_LIB_SHARED} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Link shared library using modern imported target
if (WIN32)
    target_link_libraries(${STLINK_LIB_SHARED} 
        libusb::libusb
        Threads::Threads
        ${SSP_LIB}
        wsock32
        ws2_32
    )
else ()
    target_link_libraries(${STLINK_LIB_SHARED}
        libusb::libusb
        Threads::Threads
        ${SSP_LIB}
    )
endif()

# Installation rules for shared library (DLL vs. SO)
if (MSVC)
    # For MSVC:
    # shared libraries (.dll) go to `bin`
    # their Import Libraries (aka: ImpLibs; .lib files) go to `lib`
    install(TARGETS ${STLINK_LIB_SHARED}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )
else()
    # For non-MSVC:
    # shared libraries go to `lib`
    # Archive destination is set to `lib` as failsafe
    install(TARGETS ${STLINK_LIB_SHARED}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )
endif()


###
# Static library
###

# Set library name
set(STLINK_LIB_STATIC ${PROJECT_NAME}-static)
set(STLINK_LIB_STATIC_OUTPUT_NAME ${PROJECT_NAME})
if (MSVC)
    set(STLINK_LIB_STATIC_OUTPUT_NAME ${PROJECT_NAME}-static)
endif()

add_library(${STLINK_LIB_STATIC} STATIC ${STLINK_HEADERS} ${STLINK_SOURCES})

set(STLINK_STATIC_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
message(STATUS "STLINK_LIB_STATIC: ${STLINK_LIB_STATIC}")
message(STATUS "VERSION: ${STLINK_STATIC_VERSION}")

set_target_properties(${STLINK_LIB_STATIC} PROPERTIES
                        SOVERSION ${PROJECT_VERSION_MAJOR}
                        VERSION ${STLINK_STATIC_VERSION}
                        OUTPUT_NAME ${STLINK_LIB_STATIC_OUTPUT_NAME}
                        )

# Link static library using modern imported target
if (WIN32)
    target_link_libraries(${STLINK_LIB_STATIC}
        libusb::libusb
        Threads::Threads
        ${SSP_LIB}
        wsock32
        ws2_32
    )
else ()
    target_link_libraries(${STLINK_LIB_STATIC}
        libusb::libusb
        Threads::Threads
        ${SSP_LIB}
    )
endif()

install(TARGETS ${STLINK_LIB_STATIC} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})


###
# Build toolset executables
###

# Set source files for tools
set(ST-INFO_SOURCES src/st-info/info.c)
set(ST-TRACE_SOURCES src/st-trace/trace.c)
set(ST-SERVER_SOURCES src/st-server/server.c)

set(ST-FLASH_SOURCES
    src/st-flash/flash.c
    src/st-flash/flash_opts.c
    )

set(ST-UTIL_SOURCES
    src/st-util/gdb-remote.c
    src/st-util/gdb-server.c
    src/st-util/semihosting.c
    )

if (WIN32 OR MSVC OR MINGW)
    # Add getopt to sources
    include_directories(src/stlink-env/getopt)
    list(APPEND ST-UTIL_SOURCES src/stlink-env/getopt/getopt.c)
    list(APPEND ST-TRACE_SOURCES src/stlink-env/getopt/getopt.c)
    list(APPEND ST-SERVER_SOURCES src/stlink-env/getopt/getopt.c)
endif()

# Create tool executables
add_executable(st-info ${ST-INFO_SOURCES})
add_executable(st-trace ${ST-TRACE_SOURCES})
add_executable(st-server ${ST-SERVER_SOURCES})
add_executable(st-flash ${ST-FLASH_SOURCES})
add_executable(st-util ${ST-UTIL_SOURCES})

# Include directories containing header files
target_include_directories(st-info PRIVATE src/st-info)
target_include_directories(st-trace PRIVATE src/st-trace)
target_include_directories(st-server PRIVATE src/st-server)
target_include_directories(st-flash PRIVATE src/st-flash)
target_include_directories(st-util PRIVATE src/st-util)

# Platform-specific library linking
if (WIN32)
    set(STLINK_LIB_TO_LINK ${STLINK_LIB_STATIC})
else ()
    set(STLINK_LIB_TO_LINK ${STLINK_LIB_SHARED})
endif()

message(STATUS "STLINK_LIB_TO_LINK: ${STLINK_LIB_TO_LINK}")

target_link_libraries(st-info PRIVATE ${STLINK_LIB_TO_LINK})
target_link_libraries(st-trace PRIVATE ${STLINK_LIB_TO_LINK})
target_link_libraries(st-server PRIVATE ${STLINK_LIB_TO_LINK})
target_link_libraries(st-flash PRIVATE ${STLINK_LIB_TO_LINK})
target_link_libraries(st-util PRIVATE ${STLINK_LIB_TO_LINK})

# Install tools
install(TARGETS st-info DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-trace DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-server DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-flash DESTINATION ${CMAKE_INSTALL_BINDIR})
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
endif()


###
# Additional build tasks
###

# MCU chip configuration files
if (WIN32)
    set(CMAKE_CHIPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config/chips)
else ()
    set(CMAKE_CHIPS_DIR ${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}/config/chips)
endif()

add_definitions( -DSTLINK_CHIPS_DIR="${CMAKE_CHIPS_DIR}" )
file(GLOB CHIP_FILES ${CMAKE_SOURCE_DIR}/config/chips/*.chip)
install(FILES ${CHIP_FILES} DESTINATION ${CMAKE_CHIPS_DIR})

# Documentation / manpages
option(STLINK_GENERATE_MANPAGES "Generate manpages with pandoc" OFF)
add_subdirectory(doc/man)         # contains subordinate CMakeLists to generate manpages

# Optional build components
add_subdirectory(src/stlink-gui)  # contains subordinate CMakeLists to build GUI
add_subdirectory(cmake/packaging) # contains subordinate CMakeLists to build packages
add_subdirectory(tests)           # contains subordinate CMakeLists to build test executables


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
endif()
