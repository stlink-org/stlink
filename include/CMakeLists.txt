file(GLOB STLINK_HEADERS
	"stlink/*.h"
)

install(FILES ${CMAKE_SOURCE_DIR}/include/stlink.h
	DESTINATION include/${CMAKE_LIBRARY_PATH})
install(FILES ${STLINK_HEADERS}
	DESTINATION include/${CMAKE_LIBRARY_PATH}/stlink)
