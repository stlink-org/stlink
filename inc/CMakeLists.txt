configure_file(
    "${PROJECT_SOURCE_DIR}/inc/version.h.in"
    "${CMAKE_BINARY_DIR}/inc/version.h"
    )

file(GLOB STLINK_HEADERS
    "${CMAKE_SOURCE_DIR}/inc/*.h"
    "${CMAKE_SOURCE_DIR}/src/stlink-lib/*.h"
    "${CMAKE_BINARY_DIR}/inc/version.h")
install(FILES ${STLINK_HEADERS} DESTINATION ${STLINK_INCLUDE_PATH})
