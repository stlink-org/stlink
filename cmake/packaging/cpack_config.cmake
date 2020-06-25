# cpack_config.cmake
# Configure and generate packages for distribution

###
# Configure package
###

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "Open source STM32 MCU programming toolset")
set(CPACK_PACKAGE_VENDOR "stlink-org")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/stlink-org/stlink")

set(CPACK_SET_DESTDIR "ON")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/dist")

if (APPLE)                                                                      # macOS
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-macosx-amd64")
    set(CPACK_INSTALL_PREFIX "")

elseif (WIN32 AND (NOT EXISTS "/etc/debian_version"))                           # Windows
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-win32")
    set(CPACK_INSTALL_PREFIX "")

elseif (WIN32)                                                                  # Windows cross-build on Debian/Ubuntu
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${TOOLCHAIN_PREFIX}")
    set(CPACK_INSTALL_PREFIX "")

elseif (EXISTS "/etc/debian_version" AND NOT EXISTS WIN32) # Package-build is available on Debian/Ubuntu only
    message(STATUS "Debian-based Linux OS detected")

    set(CPACK_GENERATOR "DEB;RPM") # RPM requires package `rpm`

    ###
    # Debian (DEB)
    ###

    # CPACK_DEB_PACKAGE_NAME             --> Default: CPACK_PACKAGE_NAME

    ## DEB package file name
    # CPack DEB generator generates package file name in deb format:
    # <PackageName>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # CPACK_DEBIAN_PACKAGE_VERSION       --> Default: CPACK_PACKAGE_VERSION

    ## Set debian_revision number
    # Convention: Restart the debian_revision at 1 each time the upstream_version is increased.
    set(CPACK_DEBIAN_PACKAGE_RELEASE "1")

    # CPACK_DEBIAN_PACKAGE_ARCHITECTURE  --> Default: Output of dpkg --print-architecture
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "pkg-config, build-essential, debhelper (>=9), cmake (>= 3.4.2), libusb-1.0-0-dev (>= 1.0.20)")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Luca Boccassi <bluca@debian.org>")
    # CPACK_DEBIAN_PACKAGE_DESCRIPTION   --> Default: CPACK_DEBIAN_PACKAGE_DESCRIPTION (as it is set)
    # CPACK_DEBIAN_PACKAGE_SECTION       --> Default: “devel”
    # CPACK_DEBIAN_ARCHIVE_TYPE          --> Default: “gnutar”
    # CPACK_DEBIAN_COMPRESSION_TYPE      --> Default: “gzip”
    # CPACK_DEBIAN_PACKAGE_PRIORITY      --> Default: “optional”
    # CPACK_DEBIAN_PACKAGE_HOMEPAGE      --> Default: CMAKE_PROJECT_HOMEPAGE_URL
    set(CPACK_DEBIAN_PACKAGE_SUGGESTS "libgtk-3-dev, pandoc")

    ## Additional package files in Debian-specific format:
    # * changelog (package changelog)
    # * copyright (license file)
    # * rules
    # * postinst-script
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/cmake/packaging/deb/changelog"
        "${CMAKE_SOURCE_DIR}/cmake/packaging/deb/copyright"
        "${CMAKE_SOURCE_DIR}/cmake/packaging/deb/rules"
        "${CMAKE_SOURCE_DIR}/cmake/packaging/deb/postinst"
        )

    ###
    # Slackware & Redhat (RPM)
    ###

    set(CPACK_SET_DESTDIR "OFF") # Required for relocatable package

    # CPACK_RPM_PACKAGE_SUMMARY          --> Default: CPACK_PACKAGE_DESCRIPTION_SUMMARY
    # CPACK_RPM_PACKAGE_NAME             --> Default: CPACK_PACKAGE_NAME

    ## RPM package file name
    # Allow rpmbuild to generate package file name
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

    # CPACK_RPM_PACKAGE_VERSION          --> Default: CPACK_PACKAGE_VERSION
    # CPACK_RPM_PACKAGE_ARCHITECTURE     --> Default: Native architecture output by uname -m

    ## Set rpm revision number
    # Convention: Restart the debian_revision at 1 each time the upstream_version is increased.
    set(CPACK_RPM_PACKAGE_RELEASE "1")

    set(CPACK_RPM_PACKAGE_LICENSE "BSD-3")
    # CPACK_RPM_PACKAGE_GROUP            --> Default: “unknown” (RPM Groups are deprecated on Fedora)
    # CPACK_RPM_PACKAGE_VENDOR           --> Default: CPACK_PACKAGE_VENDOR (as it is set)
    # CPACK_RPM_PACKAGE_URL              --> Default: CMAKE_PROJECT_HOMEPAGE_URL
    set(CPACK_RPM_PACKAGE_DESCRIPTION CPACK_DEBIAN_PACKAGE_DESCRIPTION)

    ## Add package changelog in rpm-specific format
    set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_SOURCE_DIR}/cmake/packaging/rpm/changelog")

else ()
    # No package configuration on other platforms ...
endif ()


###
# Build packages
###

include(CPack)
