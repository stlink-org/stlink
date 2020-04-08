# FindLibUSB.cmake
# Once done this will define
#
#  LIBUSB_FOUND - System has libusb
#  LIBUSB_INCLUDE_DIR - The libusb include directory
#  LIBUSB_LIBRARY - The libraries needed to use libusb
#  LIBUSB_DEFINITIONS - Compiler switches required for using libusb

if (NOT (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")) # all OS apart from FreeBSD
    FIND_PATH(
        LIBUSB_INCLUDE_DIR NAMES libusb.h
        HINTS /usr /usr/local /opt
        PATH_SUFFIXES libusb-1.0
        )
endif ()

if (APPLE)                                      # macOS
    set(LIBUSB_NAME libusb-1.0.a)
    find_library(
        LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
        HINTS /usr /usr/local /opt
        )

elseif (MINGW OR MSYS)                          # Windows with MinGW or MSYS
    set(LIBUSB_NAME usb-1.0)

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)            # 64-bit
        find_library(
            LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
            HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/MinGW64/static
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
            )
    else ()                                     # 32-bit
        find_library(
            LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
            HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/MinGW32/static
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
            )
    endif ()

elseif (MSVC)                                   # Windows with MSVC
    set(LIBUSB_NAME libusb-1.0.lib)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)            # 64-bit
        find_library(
            LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
            HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/MS64/dll
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
            )
    else ()                                     # 32-bit
        find_library(
            LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
            HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/MS32/dll
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
            )
    endif ()

elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")   # FreeBSD
    # libusb is integrated into FreeBSD
    FIND_PATH(
        LIBUSB_INCLUDE_DIR NAMES libusb.h
        HINTS /usr/include
        )

    set(LIBUSB_NAME usb)
    find_library(
        LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
        HINTS /usr /usr/local /opt
        )

else ()                                          # all other OS
    set(LIBUSB_NAME usb-1.0)
    find_library(
        LIBUSB_LIBRARY NAMES ${LIBUSB_NAME}
        HINTS /usr /usr/local /opt
        )
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libusb DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)
mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)

if (NOT LIBUSB_FOUND)
    if (WIN32 OR MINGW OR MSYS OR MSVC)         # Windows

        # Preparations for installing libusb library
        find_package(7Zip REQUIRED)
        set(LIBUSB_WIN_VERSION 1.0.23)           # set libusb version to 1.0.23 (latest as of Apr 2020)
        set(LIBUSB_WIN_ARCHIVE libusb-${LIBUSB_WIN_VERSION}.7z)
        set(LIBUSB_WIN_ARCHIVE_PATH ${CMAKE_BINARY_DIR}/${LIBUSB_WIN_ARCHIVE})
        set(LIBUSB_WIN_OUTPUT_FOLDER ${CMAKE_BINARY_DIR}/3rdparty/libusb-${LIBUSB_WIN_VERSION})

        # Get libusb package
        if (EXISTS ${LIBUSB_WIN_ARCHIVE_PATH})   # ... should the package be already there (for whatever reason)
            message(STATUS "libusb archive already in build folder")
        else ()                                  # ... download the package (1.0.23 as of Apr 2020)
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
                COMMAND ${ZIP_EXECUTABLE} x -y ${LIBUSB_WIN_ARCHIVE_PATH} -o ${LIBUSB_WIN_OUTPUT_FOLDER}
                )
        endif ()

        # Find path to libusb library
        FIND_PATH(
            LIBUSB_INCLUDE_DIR NAMES libusb.h
            HINTS ${LIBUSB_WIN_OUTPUT_FOLDER}/include
            PATH_SUFFIXES libusb-1.0
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
            )

        FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libusb DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)
        message(STATUS "installed libusb library")
    else ()                                      # all other OS
        message(FATAL_ERROR "libusb library not found on your system! Compilation terminated.")
    endif ()
else (NOT LIBUSB_FOUND)
    message(STATUS "found libusb library")
endif ()
