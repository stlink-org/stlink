configure_file(
	"${PROJECT_SOURCE_DIR}/include/stlink/version.h.in"
	"${CMAKE_BINARY_DIR}/include/stlink/version.h"
)
file(GLOB STLINK_HEADERS
	"stlink/*.h"
	"${CMAKE_BINARY_DIR}/include/stlink/*.h"
)
install(FILES ${CMAKE_SOURCE_DIR}/include/stlink.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
install(FILES ${STLINK_HEADERS}
    DESTINATION include/${CMAKE_INSTALL_PREFIX}/include/stlink
)
