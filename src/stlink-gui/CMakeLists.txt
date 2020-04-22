if (NOT GTK3_FOUND)
    message(STATUS "GTK3 not found!")
    return()                          # no GTK3 present => no GUI build
else (GTK3_FOUND)
    message(STATUS "Found GTK3: -I${GTK3_INCLUDE_DIRS}, ${GTK3_LIBRARIES}")
endif ()

set(GUI_SOURCES gui.c gui.h)
set(INSTALLED_UI_DIR share/stlink)                                                  ### TODO: Check Path

include_directories(SYSTEM ${GTK3_INCLUDE_DIRS})

# stlink-gui-local
add_executable(stlink-gui-local ${GUI_SOURCES})
set_target_properties(
    stlink-gui-local PROPERTIES
    COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}"                 ### TODO: Check Path
    )
target_link_libraries(stlink-gui-local ${STLINK_LIB_STATIC} ${GTK3_LDFLAGS} ${SSP_LIB})

# stlink-gui
add_executable(stlink-gui ${GUI_SOURCES})
set_target_properties(
    stlink-gui PROPERTIES
    COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_INSTALL_PREFIX}/${INSTALLED_UI_DIR}" ### TODO: Check Path
    )
target_link_libraries(stlink-gui ${STLINK_LIB_STATIC} ${GTK3_LDFLAGS} ${SSP_LIB})

install(TARGETS stlink-gui RUNTIME DESTINATION bin)                                 ### TODO: Check Path
install(FILES gui.ui DESTINATION ${INSTALLED_UI_DIR})                               ### TODO: Check Path

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES stlink-gui.desktop DESTINATION share/applications)                  # Install desktop entry
    install(FILES icons/stlink-gui.svg DESTINATION share/icons/hicolor/scalable/apps) # Install icon
endif ()
