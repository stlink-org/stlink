###
# General cmake settings
###

cmake_minimum_required(VERSION 3.21)

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
# pthreads on Unix; on Windows the Win32 API is used directly, so that neither
# PThreads4W (MSVC) nor winpthread (MinGW) has to be found, linked or shipped.
if (WIN32)
    # Nothing to link here: _beginthreadex comes from the CRT, and
    # WaitForSingleObject and CloseHandle from kernel32, both of which MSVC and
    # MinGW link by default. Deliberately no find_package(Threads): with
    # THREADS_PREFER_PTHREAD_FLAG it would pick winpthread on MinGW and add
    # -pthread, restoring the libwinpthread-1.dll dependency.
    set(STLINK_THREAD_LIBRARY "")
else ()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(STLINK_THREAD_LIBRARY Threads::Threads)
endif()

# Set C build flags
if (NOT MSVC)
    include(${CMAKE_MODULE_PATH}/c_flags.cmake)
endif()
# MSVC runtime selection is left to CMAKE_MSVC_RUNTIME_LIBRARY/the toolchain.


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
include_directories(${STLINK_INCLUDE_DIRS})
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
    "${CMAKE_SOURCE_DIR}/src/stlink-env/*.h"
    "${CMAKE_SOURCE_DIR}/src/stlink-lib/*.h"
    "${CMAKE_BINARY_DIR}/inc/version.h")
list(APPEND STLINK_HEADERS ${STLINK_LIB_HEADERS})

file(GLOB STLINK_LIB_SOURCES
    "${CMAKE_SOURCE_DIR}/src/stlink-lib/*.c")
list(APPEND STLINK_SOURCES ${STLINK_LIB_SOURCES})

# Install library header files
install(FILES ${STLINK_HEADERS} DESTINATION ${STLINK_INCLUDE_PATH})

if (MSVC)
    # libusb_settings.h and unistd.h (both installed above) include these,
    # so downstream consumers of stlink-static.lib need them too - v1.8.0
    # shipped them, the vcpkg switch stopped bundling them (issue #2).
    install(FILES "${LIBUSB_INCLUDE_DIR}/libusb.h" DESTINATION "${STLINK_INCLUDE_PATH}/libusb-1.0")
    install(FILES src/stlink-env/getopt/getopt.h DESTINATION ${STLINK_INCLUDE_PATH})
endif()


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
        ${STLINK_THREAD_LIBRARY}
        ${SSP_LIB}
        wsock32
        ws2_32
    )
else ()
    target_link_libraries(${STLINK_LIB_SHARED}
        libusb::libusb
        ${STLINK_THREAD_LIBRARY}
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
        ${STLINK_THREAD_LIBRARY}
        ${SSP_LIB}
        wsock32
        ws2_32
    )
else ()
    target_link_libraries(${STLINK_LIB_STATIC}
        libusb::libusb
        ${STLINK_THREAD_LIBRARY}
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
install(TARGETS st-info       RUNTIME_DEPENDENCY_SET stlink_deps DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-trace      RUNTIME_DEPENDENCY_SET stlink_deps DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-server     RUNTIME_DEPENDENCY_SET stlink_deps DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-flash      RUNTIME_DEPENDENCY_SET stlink_deps DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS st-util       RUNTIME_DEPENDENCY_SET stlink_deps DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS stlink-shared RUNTIME_DEPENDENCY_SET stlink_deps DESTINATION ${CMAKE_INSTALL_BINDIR})


if (MSVC)
    # vcpkg links libusb as a DLL on Windows; without this, an installed exe imports libusb-1.0.dll
    # that was never installed and fails to start.
    #
    # Resolution does not rely solely on VCPKG_APPLOCAL_DEPS having already copied the DLL next
    # to the exe at build time (that in turn relies on applocal.ps1 finding dumpbin/objdump
    # on PATH, which is not guaranteed outside a VS developer shell, e.g. plain Ninja builds);
    # the vcpkg triplet's own bin directory is searched directly as a second source.
    # Anything still unresolved is reported instead of aborting the install.

    install(RUNTIME_DEPENDENCY_SET stlink_deps
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
        POST_EXCLUDE_REGEXES ".*[Ss][Yy][Ss][Tt][Ee][Mm]32.*"
        DIRECTORIES "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin"
        UNRESOLVED_DEPENDENCIES_VAR stlink_unresolved_deps
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(CODE "
        if(stlink_unresolved_deps)
            message(WARNING \"cmake --install: unresolved runtime dependencies, not installed: \${stlink_unresolved_deps}\")
        endif()
    ")
endif()


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
