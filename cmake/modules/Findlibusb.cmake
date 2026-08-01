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
# FreeBSD: libusb is part of the base system
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h HINTS /usr/include)
    find_library(LIBUSB_LIBRARY NAMES usb HINTS /usr /usr/local /opt)

# OpenBSD: libusb available via ports/packages
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h HINTS /usr/local/include PATH_SUFFIXES libusb-1.0)
    find_library(LIBUSB_LIBRARY NAMES usb-1.0 HINTS /usr/local)

# Windows (native MSVC or MinGW without cross-compiling)
elseif(MSVC OR (WIN32 AND NOT EXISTS "/etc/debian_version"))
    # Fix for missing ssize_t on Windows (required by libusb)
    add_compile_definitions(_SSIZE_T_DEFINED ssize_t=int64_t)

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
    # Fix for ssize_t on Windows
    add_compile_definitions(_SSIZE_T_DEFINED ssize_t=int64_t)

    # Architecture: 64-bit or 32-bit?
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "=== Building for Windows (x86-64) ===")
        set(ARCH 64)
    else ()
        message(STATUS "=== Building for Windows (i686) ===")
        set(ARCH 32)
    endif ()

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
            COMMAND ./configure --host=i686-w64-mingw${ARCH} --prefix=${libusb_BINARY_DIR}/install
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
            IMPORTED_LOCATION "${LIBUSB_LIBRARY}"
        )
    endif()
endif()

# Handle standard REQUIRED/QUIET arguments for find_package()
find_package_handle_standard_args(libusb REQUIRED_VARS LIBUSB_FOUND)
