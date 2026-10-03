###
# Build GUI
###

if (NOT WIN32)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        # Check for GTK4 (minimum version 4.6)
        pkg_check_modules(GTK4 QUIET IMPORTED_TARGET gtk4>=4.6)
    endif()

    ## GUI-Building requires the presence of a GTK4 toolset
    if (NOT GTK4_FOUND)
        message(STATUS "GTK4 (>= 4.6) not found! GUI build will be skipped.")
        return() # No GTK4 present => no GUI build
    else ()
        message(STATUS "Found GTK4 version: ${GTK4_VERSION}")

        # Install desktop application entry
        install(FILES stlink-gui.desktop
                DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/applications)

        # Install icons
        install(FILES icons/stlink-gui.svg
                DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps)

        set(GUI_SOURCES gui.c gui.h)

        ## stlink-gui
        add_executable(stlink-gui ${GUI_SOURCES})
        install(FILES stlink-gui.ui DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME})

        target_compile_definitions(stlink-gui PRIVATE
                STLINK_UI_DIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}"
                # Restrict the API to GTK 4.6 / GLib 2.66 (minimum requirements): Using newer API
                # raises a warning and newer GTK versions do not warn about API deprecated after 4.6.
                GDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_6
                GDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_6
                GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_66
                GLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_66)

        target_link_libraries(stlink-gui ${STLINK_LIB_SHARED} ${SSP_LIB} PkgConfig::GTK4)

        install(TARGETS stlink-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()
