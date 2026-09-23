# Findlibusb.cmake
# Modern CMake 3.20+ Find module for libusb-1.0
#
# Defines:
#   - Imported Target: libusb::libusb
#   - Variables: LIBUSB_FOUND, LIBUSB_INCLUDE_DIR, LIBUSB_LIBRARY
#

include(FetchContent)
include(FindPackageHandleStandardArgs)

set(LIBUSB_FOUND FALSE)  # Default: libusb not found

# --- Platform-specific configurations ---
# vcpkg installs Release and Debug libraries with the same name in separate directories.
# A single find_library() can select the Debug library for every VS configuration.
if(WIN32 AND VCPKG_TARGET_TRIPLET)
    set(_LIBUSB_VCPKG_PREFIX "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
    find_path(LIBUSB_INCLUDE_DIR
        NAMES libusb.h
        PATHS "${_LIBUSB_VCPKG_PREFIX}/include"
        PATH_SUFFIXES libusb-1.0
        NO_DEFAULT_PATH
    )
    find_library(LIBUSB_LIBRARY_RELEASE
        NAMES usb-1.0 libusb-1.0
        PATHS "${_LIBUSB_VCPKG_PREFIX}/lib"
        NO_DEFAULT_PATH
    )
    find_library(LIBUSB_LIBRARY_DEBUG
        NAMES usb-1.0 libusb-1.0
        PATHS "${_LIBUSB_VCPKG_PREFIX}/debug/lib"
        NO_DEFAULT_PATH
    )
    include(SelectLibraryConfigurations)
    select_library_configurations(LIBUSB)

# FreeBSD: libusb is part of the base system
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h HINTS /usr/include)
    find_library(LIBUSB_LIBRARY NAMES usb HINTS /usr /usr/local /opt)

# OpenBSD: libusb available via ports/packages
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h HINTS /usr/local/include PATH_SUFFIXES libusb-1.0)
    find_library(LIBUSB_LIBRARY NAMES usb-1.0 HINTS /usr/local)

# MSVC without the mandatory vcpkg toolchain: unsupported per project policy.
# Fail early with a clear pointer instead of falling through to a stray
# system libusb install, which can predate 1.0.30 and reintroduce the
# winsock.h/winsock2.h conflict this project no longer supports.
elseif(MSVC)
    message(FATAL_ERROR
        "libusb-1.0 was not found via vcpkg (VCPKG_TARGET_TRIPLET is unset).\n"
        "MSVC builds require the vcpkg toolchain file - see doc/compiling.md."
    )

# Windows (native MinGW build on Windows itself, not cross-compiling)
elseif(WIN32 AND NOT EXISTS "/etc/debian_version")
    # Try to locate an existing Windows installation of libusb
    find_path(LIBUSB_INCLUDE_DIR
        NAMES libusb.h
        HINTS "C:/Program Files/libusb-1.0/include" "C:/Program Files (x86)/libusb-1.0/include"
        PATH_SUFFIXES libusb-1.0
    )

    find_library(LIBUSB_LIBRARY
        NAMES usb-1.0 libusb-1.0
        HINTS "C:/Program Files/libusb-1.0" "C:/Program Files (x86)/libusb-1.0"
    )

# Windows-Build with MinGW via cross-compiling on Debian-Linux
elseif(MINGW AND EXISTS "/etc/debian_version")
    message(STATUS "=== Building for Windows (${TOOLCHAIN_PREFIX}) ===")

    # Download and build libusb via FetchContent
    if(NOT LIBUSB_FOUND)
        message(STATUS "libusb-1.0 not found locally. Downloading and building from source via FetchContent...")

        FetchContent_Declare(
            libusb
            GIT_REPOSITORY "https://github.com/libusb/libusb.git"
            GIT_TAG "v1.0.30"
        )
        FetchContent_MakeAvailable(libusb)

        # Run bootstrap.sh (if available)
        if(EXISTS "${libusb_SOURCE_DIR}/bootstrap.sh")
            execute_process(
                COMMAND ./bootstrap.sh
                WORKING_DIRECTORY ${libusb_SOURCE_DIR}
                RESULT_VARIABLE BOOTSTRAP_RESULT
            )
            if(NOT BOOTSTRAP_RESULT EQUAL 0)
                message(FATAL_ERROR "libusb bootstrap.sh failed with code ${BOOTSTRAP_RESULT}")
            endif()
        endif()

        # Configure
        execute_process(
            COMMAND ./configure --host=${TOOLCHAIN_PREFIX} --prefix=${libusb_BINARY_DIR}/install
                                --enable-static --disable-shared --disable-udev
            WORKING_DIRECTORY ${libusb_SOURCE_DIR}
            RESULT_VARIABLE CONFIGURE_RESULT
        )
        if(NOT CONFIGURE_RESULT EQUAL 0)
            message(FATAL_ERROR "libusb configure failed with code ${CONFIGURE_RESULT}")
        endif()

        # Build library
        execute_process(
            COMMAND make
            WORKING_DIRECTORY ${libusb_SOURCE_DIR}
            RESULT_VARIABLE MAKE_RESULT
        )
        if(NOT MAKE_RESULT EQUAL 0)
            message(FATAL_ERROR "libusb make failed with code ${MAKE_RESULT}")
        endif()

        # Install library
        execute_process(
            COMMAND make install
            WORKING_DIRECTORY ${libusb_SOURCE_DIR}
            RESULT_VARIABLE INSTALL_RESULT
        )
        if(NOT INSTALL_RESULT EQUAL 0)
            message(FATAL_ERROR "libusb make install failed with code ${INSTALL_RESULT}")
        endif()

        # Get include dir and library path from the target
        set(LIBUSB_INCLUDE_DIR "${libusb_SOURCE_DIR}/libusb")
        set(LIBUSB_LIBRARY "${libusb_SOURCE_DIR}/../libusb-build/install/lib/libusb-1.0.a")

        # Create a CMake target
        if(NOT TARGET libusb::libusb)
            add_library(libusb::libusb UNKNOWN IMPORTED GLOBAL)
            set_target_properties(libusb::libusb PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}"
                IMPORTED_LOCATION "${LIBUSB_LIBRARY}"
            )
        endif()
    endif()

# All other Unix-based systems (Linux, macOS, etc.)
else()
    # Use pkg-config to find libusb (if available)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_search_module(PC_LIBUSB QUIET libusb-1.0)
    endif()

    # Locate include directory and library using pkg-config hints or defaults
    find_path(LIBUSB_INCLUDE_DIR
        NAMES libusb.h
        HINTS ${PC_LIBUSB_INCLUDE_DIRS}
        PATH_SUFFIXES libusb-1.0
    )

    find_library(LIBUSB_LIBRARY
        NAMES usb-1.0
        HINTS ${PC_LIBUSB_LIBRARY_DIRS}
    )
endif()

# Finalize the imported target
# Only proceed if both include dir and library path were found
if(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARY)
    set(LIBUSB_FOUND TRUE)  # Update found status

    # Create the imported target if it doesn't exist yet
    if(NOT TARGET libusb::libusb)
        # Create an imported target (type UNKNOWN = static or shared library)
        add_library(libusb::libusb UNKNOWN IMPORTED GLOBAL)

        # Set the target's properties:
        # - INTERFACE_INCLUDE_DIRECTORIES: Where to find libusb.h
        # - IMPORTED_LOCATION: Path to the compiled library (e.g., libusb-1.0.a/.so/.lib)
        set_target_properties(libusb::libusb PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}"
        )
        if(LIBUSB_LIBRARY_RELEASE OR LIBUSB_LIBRARY_DEBUG)
            foreach(_LIBUSB_CONFIG RELEASE DEBUG)
                if(LIBUSB_LIBRARY_${_LIBUSB_CONFIG})
                    set_property(TARGET libusb::libusb APPEND PROPERTY
                        IMPORTED_CONFIGURATIONS ${_LIBUSB_CONFIG})
                    set_target_properties(libusb::libusb PROPERTIES
                        IMPORTED_LOCATION_${_LIBUSB_CONFIG} "${LIBUSB_LIBRARY_${_LIBUSB_CONFIG}}")
                endif()
            endforeach()
        else()
            set_target_properties(libusb::libusb PROPERTIES IMPORTED_LOCATION "${LIBUSB_LIBRARY}")
        endif()
    endif()
endif()

# Handle standard REQUIRED/QUIET arguments for find_package()
find_package_handle_standard_args(libusb REQUIRED_VARS LIBUSB_FOUND)
