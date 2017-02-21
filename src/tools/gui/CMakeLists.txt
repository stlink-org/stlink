if (NOT gtk_FOUND)
	return()
endif()

set(GUI_SOURCES stlink-gui.c stlink-gui.h)
set(INSTALLED_UI_DIR share/stlink)

include_directories(SYSTEM ${gtk_INCLUDE_DIRS})

add_executable(stlink-gui-local ${GUI_SOURCES})
set_target_properties(stlink-gui-local PROPERTIES
                      COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/gui")
target_link_libraries(stlink-gui-local ${STLINK_LIB_STATIC} ${gtk_LDFLAGS})


add_executable(stlink-gui ${GUI_SOURCES})
set_target_properties(stlink-gui PROPERTIES
                      COMPILE_DEFINITIONS STLINK_UI_DIR="${CMAKE_INSTALL_PREFIX}/${INSTALLED_UI_DIR}")
target_link_libraries(stlink-gui ${STLINK_LIB_STATIC} ${gtk_LDFLAGS})

install(TARGETS stlink-gui
        RUNTIME DESTINATION bin)
install(FILES stlink-gui.ui
        DESTINATION ${INSTALLED_UI_DIR})
