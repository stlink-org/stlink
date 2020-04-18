###
# Configure package
###

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_SET_DESTDIR "ON")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/dist")

if (APPLE)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-macosx-amd64")
    set(CPACK_INSTALL_PREFIX "")
elseif (WIN32)                                                                  ### TODO: Binary build config for windows...
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-win32")
    set(CPACK_INSTALL_PREFIX "")

    # Sample toolchain file for building for Windows from a Debian/Ubuntu Linux system.
    # Typical usage:
    #    *) install cross compiler: `sudo apt-get install mingw-w64`
    #    *) cd build
    #    *) cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64.cmake ..
    #    *) cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw32.cmake ..

    #set(CMAKE_SYSTEM_NAME Windows)
    #set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)
    #set(TOOLCHAIN_PREFIX i686-w64-mingw32)

    # cross compilers to use for C and C++
    #set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    #set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
    #set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

    # target environment on the build host system
    # set 1st to dir with the cross compiler's C/C++ headers/libs
    #set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

    # modify default behavior of FIND_XXX() commands to
    # search for headers/libs in the target environment and
    # search for programs in the build host environment
    #set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    #set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    #set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND EXISTS "/etc/debian_version")
    message(STATUS "Debian-based Linux OS detected")

    ### Debian-specific
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Open source STM32 MCU programming toolset")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/stlink-org/stlink")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Luca Boccassi")
    set(CPACK_PACKAGE_CONTACT "bluca@debian.org")

    ## Set debian_revision number
    # Convention: Restart the debian_revision at 1 each time the upstream_version is increased.
    set(CPACK_DEBIAN_PACKAGE_RELEASE "0")

    ## Debian package name
    # CPack DEB generator generates package file name in deb format:
    # <PackageName>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    ## Add CHANGELOG in Debian-specific format
                                                                                ### TODO

    ## Add license file
                                                                                ### TODO

    # Include a postinst-script
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/cmake/packaging/debian/postinst")

    ### rpm-specific                                                            ### TODO: Package config for opensuse should go here...

else ()
                                                                                ### TODO: Package config for fedora should go here...
endif ()


###
# Build package
###

include(CPack)
