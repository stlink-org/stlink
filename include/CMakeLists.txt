configure_file(
	"${PROJECT_SOURCE_DIR}/include/stlink/version.h.in"
	"${CMAKE_BINARY_DIR}/include/stlink/version.h"
)
file(GLOB STLINK_HEADERS
	"stlink/*.h"
	"${CMAKE_BINARY_DIR}/include/stlink/*.h"
)
install(FILES ${CMAKE_SOURCE_DIR}/include/stlink.h
    DESTINATION ${STLINK_INCLUDE_PATH}
)
install(FILES ${STLINK_HEADERS}
    DESTINATION ${STLINK_INCLUDE_PATH}/stlink
)
