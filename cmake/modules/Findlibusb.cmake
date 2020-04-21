# Findlibusb.cmake
# Once done this will define
#
#  LIBUSB_FOUND         libusb present on system
#  LIBUSB_INCLUDE_DIR   the libusb include directory
#  LIBUSB_LIBRARY       the libraries needed to use libusb
#  LIBUSB_DEFINITIONS   compiler switches required for using libusb

include(FindPackageHandleStandardArgs)

if (APPLE)                                      # macOS
    FIND_PATH(
        LIBUSB_INCLUDE_DIR NAMES libusb.h
        HINTS /usr /usr/local /opt
        PATH_SUFFIXES libusb-1.0
        )
    set(LIBUSB_NAME libusb-1.0.a)
    find_library(
        LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
        HINTS /usr /usr/local /opt
        )
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(libusb DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)
    mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)
    if (NOT LIBUSB_FOUND)
        message(FATAL_ERROR "No libusb library found on your system! Install libusb-1.0 from Homebrew or MacPorts")
    endif ()

elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")   # FreeBSD; libusb is integrated into the system
    FIND_PATH(
        LIBUSB_INCLUDE_DIR NAMES libusb.h
        HINTS /usr/include
        )
    set(LIBUSB_NAME usb)
    find_library(
        LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
        HINTS /usr /usr/local /opt
        )
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(libusb DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)
    mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)
    if (NOT LIBUSB_FOUND)
        message(FATAL_ERROR "Expected libusb library not found on your system! Verify your system integrity.")
    endif ()

elseif (WIN32)                                  # Windows
    # for MinGW/MSYS/MSVC: 64-bit or 32-bit?
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH 64)
    else ()
        set(ARCH 32)
    endif ()

    if (NOT EXISTS "/etc/debian_version")
        FIND_PATH(
            LIBUSB_INCLUDE_DIR NAMES libusb.h
            HINTS /usr /usr/local /opt
            PATH_SUFFIXES libusb-1.0
            )

        if (MINGW OR MSYS)
            set(LIBUSB_NAME usb-1.0)
            find_library(
                LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
                HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/MinGW${ARCH}/static
                )
        else (MSVC)
            set(LIBUSB_NAME libusb-1.0.lib)
            find_library(
                LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
                HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/MS${ARCH}/dll
                )
        endif ()
    endif ()

    if (NOT LIBUSB_FOUND OR EXISTS "/etc/debian_version")
        # Preparations for installing libusb library
        find_package(7zip REQUIRED)
        set(LIBUSB_WIN_VERSION 1.0.23)          # set libusb version
        set(LIBUSB_WIN_ARCHIVE libusb-${LIBUSB_WIN_VERSION}.7z)
        set(LIBUSB_WIN_ARCHIVE_PATH ${CMAKE_BINARY_DIR}/${LIBUSB_WIN_ARCHIVE})
        set(LIBUSB_WIN_OUTPUT_FOLDER ${CMAKE_BINARY_DIR}/3rdparty/libusb-${LIBUSB_WIN_VERSION})

        # Get libusb package
        if (EXISTS ${LIBUSB_WIN_ARCHIVE_PATH})  # ... should the package be already there (for whatever reason)
            message(STATUS "libusb archive already in build folder")
        else ()                                 # ... download the package
            message(STATUS "downloading libusb ${LIBUSB_WIN_VERSION}")
            file(DOWNLOAD
                https://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-${LIBUSB_WIN_VERSION}/libusb-${LIBUSB_WIN_VERSION}.7z/download
                ${LIBUSB_WIN_ARCHIVE_PATH} EXPECTED_MD5 cf3d38d2ff053ef343d10c0b8b0950c2
                )
        endif ()

        file(MAKE_DIRECTORY ${LIBUSB_WIN_OUTPUT_FOLDER})

        # Extract libusb package
        if (${ZIP_EXECUTABLE} MATCHES "p7zip")
            execute_process(
                COMMAND ${ZIP_EXECUTABLE} -d ${LIBUSB_WIN_ARCHIVE_PATH}
                WORKING_DIRECTORY ${LIBUSB_WIN_OUTPUT_FOLDER}
                )
        else ()
            execute_process(
                COMMAND ${ZIP_EXECUTABLE} x -y ${LIBUSB_WIN_ARCHIVE_PATH} -o${LIBUSB_WIN_OUTPUT_FOLDER}
                ) # <-- Note the absence of a space character following the -o option!
        endif ()

        # Find path to libusb library
        FIND_PATH(
            LIBUSB_INCLUDE_DIR NAMES libusb.h
            HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/include
            PATH_SUFFIXES libusb-1.0
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
            )

        if (MINGW OR MSYS)
            find_library(
                LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
                HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/MinGW${ARCH}/static
                NO_DEFAULT_PATH
                NO_CMAKE_FIND_ROOT_PATH
                )

        else (MSVC)
            find_library(
                LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
                HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/MS${ARCH}/dll
                NO_DEFAULT_PATH
                NO_CMAKE_FIND_ROOT_PATH
                )
        endif ()
        message(STATUS "Missing libusb library has been installed")
    endif ()
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(libusb DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)
    mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)

else ()                                          # all other OS (unix-based)
    FIND_PATH(
        LIBUSB_INCLUDE_DIR NAMES libusb.h
        HINTS /usr /usr/local /opt
        PATH_SUFFIXES libusb-1.0
        )
    set(LIBUSB_NAME usb-1.0)
    find_library(
        LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
        HINTS /usr /usr/local /opt
        )
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(libusb DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)
    mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)

    if (NOT LIBUSB_FOUND)
        message(FATAL_ERROR "libusb library not found on your system! Install libusb 1.0.x from your package repository.")
    endif ()
endif ()
