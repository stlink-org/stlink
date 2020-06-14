configure_file(
    "${PROJECT_SOURCE_DIR}/inc/version.h.in"
    "${CMAKE_BINARY_DIR}/inc/version.h"
    )

file(GLOB STLINK_HEADERS "src/st-link-lib/*.h" "${CMAKE_BINARY_DIR}/inc/version.h")
install(FILES ${STLINK_HEADERS} DESTINATION ${STLINK_INCLUDE_PATH})
install(FILES ${CMAKE_SOURCE_DIR}/inc/stlink.h DESTINATION ${STLINK_INCLUDE_PATH})
install(FILES ${CMAKE_SOURCE_DIR}/inc/stm32.h DESTINATION ${STLINK_INCLUDE_PATH})
