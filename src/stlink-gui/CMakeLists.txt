###
# Build GUI
###

if (NOT WIN32)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        # Check for GTK3
        pkg_check_modules(GTK3 QUIET gtk+-3.0)
    endif()

    ## GUI-Building requires the presence of a GTK3 toolset
    if (NOT GTK3_FOUND)
        message(STATUS "GTK3 not found! GUI build will be skipped.")
        return() # No GTK3 present => no GUI build
    else ()
        message(STATUS "Found GTK3 version: ${GTK3_VERSION}")
        
        # Use target-based include directories if possible, or fall back to variable
        if (TARGET PkgConfig::GTK3)
            include_directories(SYSTEM PkgConfig::GTK3)
        else ()
            include_directories(SYSTEM ${GTK3_INCLUDE_DIRS})
        endif()

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
        set_target_properties(stlink-gui PROPERTIES
                COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")
        
        # Link using the modern PkgConfig GTK3 target, falling back to variables if necessary
        if (TARGET PkgConfig::GTK3)
            target_link_libraries(stlink-gui ${STLINK_LIB_SHARED} ${SSP_LIB} PkgConfig::GTK3)
        else ()
            target_link_libraries(stlink-gui ${STLINK_LIB_SHARED} ${SSP_LIB} ${GTK3_LIBRARIES})
        endif()
        
        install(TARGETS stlink-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()
