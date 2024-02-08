###
# Build GUI
###

if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(GTK3 gtk+-3.0)

    ## GUI-Building requires the presence of a GTK3 toolset
    if (NOT GTK3_FOUND)
        message(STATUS "GTK3 not found!")
        return() # no GTK3 present => no GUI build
    else (GTK3_FOUND)
        message(STATUS "Found GTK3: -I${GTK3_INCLUDE_DIRS}, ${GTK3_LIBRARIES}")
        include_directories(SYSTEM ${GTK3_INCLUDE_DIRS})

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
        target_link_libraries(stlink-gui ${STLINK_LIB_SHARED} ${SSP_LIB} ${GTK3_LDFLAGS})
        install(TARGETS stlink-gui DESTINATION ${CMAKE_BINDIR})
    endif()
endif()
