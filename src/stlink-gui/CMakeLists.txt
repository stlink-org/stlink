###
# Build GUI
###

if (NOT WIN32 AND NOT CMAKE_CROSSCOMPILING)
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
                DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

        # Install icons
        install(FILES icons/stlink-gui.svg
                DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)

        set(GUI_SOURCES gui.c gui.h)

        ## stlink-gui-local
        add_executable(stlink-gui-local ${GUI_SOURCES})
        file(COPY stlink-gui.ui DESTINATION ${CMAKE_BINARY_DIR}/bin)
        set_target_properties(stlink-gui-local PROPERTIES
                COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_BINARY_DIR}/bin")
        target_link_libraries(stlink-gui-local ${STLINK_LIB_SHARED} ${SSP_LIB} ${GTK3_LDFLAGS})

        ## stlink-gui
        add_executable(stlink-gui ${GUI_SOURCES})
        install(FILES stlink-gui.ui DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME})
        set_target_properties(stlink-gui PROPERTIES
                COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME}")
        target_link_libraries(stlink-gui ${STLINK_LIB_SHARED} ${SSP_LIB} ${GTK3_LDFLAGS})
        install(TARGETS stlink-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif ()
endif ()
