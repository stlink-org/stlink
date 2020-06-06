###
# General cmake settings
###

cmake_minimum_required(VERSION 3.4.2)
cmake_policy(SET CMP0042 NEW)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/modules")
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

## Set installation directories for header files                                ### TODO: Clean this up...
if (IS_DIRECTORY ${INCLUDE_INSTALL_DIR})
    set(INCLUDE_INSTALL_DIR ${INCLUDE_INSTALL_DIR} CACHE PATH "Main include directory")
    set(STLINK_INCLUDE_PATH "${INCLUDE_INSTALL_DIR}")
else ()
    set(INCLUDE_INSTALL_DIR "include" CACHE PATH "Main include directory")
    set(STLINK_INCLUDE_PATH "${CMAKE_INSTALL_PREFIX}/${INCLUDE_INSTALL_DIR}")
endif ()


###
# Dependencies
###

find_package(libusb REQUIRED)

## Package configuration (pkg-config) on unix-based systems
if (NOT WIN32 AND NOT CMAKE_CROSSCOMPILING)
    #add_subdirectory(cmake/pkgconfig)
    find_package(PkgConfig)
    pkg_check_modules(GTK3 gtk+-3.0)
endif ()

## Check for system-specific additional header files and libraries
include(CheckIncludeFile)

CHECK_INCLUDE_FILE(sys/mman.h STLINK_HAVE_SYS_MMAN_H)
if (STLINK_HAVE_SYS_MMAN_H)
    add_definitions(-DSTLINK_HAVE_SYS_MMAN_H)
endif ()

CHECK_INCLUDE_FILE(unistd.h STLINK_HAVE_UNISTD_H)
if (STLINK_HAVE_UNISTD_H)
    add_definitions(-DSTLINK_HAVE_UNISTD_H)
endif ()

include(CheckLibraryExists)

CHECK_LIBRARY_EXISTS(ssp __stack_chk_fail "" _stack_chk_fail_exists)
if (_stack_chk_fail_exists)
    set(SSP_LIB -static ssp)
else ()
    set(SSP_LIB "")
endif ()


###
# Main build process
###

## Define include directories to avoid absolute paths for header defines
include_directories(${LIBUSB_INCLUDE_DIR})

# ====
include_directories(${PROJECT_SOURCE_DIR}/include)                              ### TODO: Clean this up...
include_directories(${PROJECT_BINARY_DIR}/include/stlink)
include_directories(${PROJECT_SOURCE_DIR}/include/stlink)
include_directories(${PROJECT_SOURCE_DIR}/include/stlink/tools)
# ====

include_directories(src)
include_directories(src/tools)                                                  ### TODO: Clean this up...

set(STLINK_HEADERS
        include/stlink.h
        include/stlink/backend.h
        include/stlink/chipid.h
        include/stlink/commands.h
        include/stlink/flash_loader.h
        include/stlink/reg.h
        src/logging.h
        src/md5.h
        src/sg.h
        src/usb.h
        )

set(STLINK_SOURCE
        src/common.c
        src/chipid.c
        src/flash_loader.c
        src/logging.c
        src/md5.c
        src/sg.c
        src/usb.c
        )

if (WIN32 OR MINGW OR MSYS)
    include_directories(src/mingw)
    set(STLINK_SOURCE "${STLINK_SOURCE};src/mmap.c;src/mingw/mingw.c")
    set(STLINK_HEADERS "${STLINK_HEADERS};src/mmap.h;src/mingw/mingw.h")
endif ()

if (MSVC)
    include_directories(src/win32)
    include_directories(src/getopt)
    # Use string.h rather than strings.h and disable annoying warnings
    add_definitions(-DHAVE_STRING_H -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS /wd4710)
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
execute_process (COMMAND bash -c "export LD_LIBRARY_PATH="${CMAKE_INSTALL_LIBDIR}" ")


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

set(ST-UTIL_SOURCES src/st-util/gdb-remote.c src/st-util/gdb-server.c src/st-util/semihosting.c)

if (MSVC)
    # Add getopt to sources
    set(ST-UTIL_SOURCES "${ST-UTIL_SOURCES};src/getopt/getopt.c")
endif ()

add_executable(st-flash src/tools/flash.c src/tools/flash_opts.c)
add_executable(st-info src/tools/info.c)
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

    ## Install udev rules files to /etc/udev/rules.d/ (explicitly hardcoded)
    set(STLINK_UDEV_RULES_DIR "/etc/udev/rules.d" CACHE PATH "udev rules directory")
    file(GLOB RULES_FILES ${CMAKE_SOURCE_DIR}/config/udev/rules.d/*.rules)
    install(FILES ${RULES_FILES} DESTINATION ${STLINK_UDEV_RULES_DIR})
endif ()


###
# Additional build tasks
###

# ====
add_subdirectory(include) # contains subordinate CMakeLists for version config and old header includes
                                                                                ### TODO: Clean this up ...
# ====

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
