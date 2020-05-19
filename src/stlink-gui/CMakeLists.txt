###
# Build GUI
###

## GUI-Building requires the presence of a GTK3 toolset
if (NOT GTK3_FOUND)
    message(STATUS "GTK3 not found!")
    return() # no GTK3 present => no GUI build
else (GTK3_FOUND)
    message(STATUS "Found GTK3: -I${GTK3_INCLUDE_DIRS}, ${GTK3_LIBRARIES}")
endif ()

include_directories(SYSTEM ${GTK3_INCLUDE_DIRS})

set(GUI_SOURCES gui.c gui.h)

## stlink-gui-local
add_executable(stlink-gui-local ${GUI_SOURCES})
set_target_properties(stlink-gui-local PROPERTIES
        # Note: ${CMAKE_CURRENT_SOURCE_DIR} is src/stlink-gui
        COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(stlink-gui-local ${STLINK_LIB_SHARED} ${SSP_LIB} ${GTK3_LDFLAGS})

## stlink-gui
add_executable(stlink-gui ${GUI_SOURCES})
set_target_properties(stlink-gui PROPERTIES
        COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_INSTALL_BINDIR}")
target_link_libraries(stlink-gui ${STLINK_LIB_SHARED} ${SSP_LIB} ${GTK3_LDFLAGS})

install(TARGETS stlink-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES stlink-gui.ui DESTINATION ${CMAKE_INSTALL_BINDIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Install desktop application entry
    install(FILES stlink-gui.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/applications)
    # Install icons
    install(FILES icons/stlink-gui.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/icons/hicolor/scalable/apps)
endif ()
