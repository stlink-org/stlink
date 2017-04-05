set(PKG_CONFIG_LIBDIR     "\${prefix}/lib/\${deb_host_multiarch}")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/\${deb_host_multiarch}/${PROJECT_NAME}-${PROJECT_VERSION}")
set(PKG_CONFIG_LIBS       "-L\${libdir} -l:libstlink.so.${PROJECT_VERSION_MAJOR}")
set(PKG_CONFIG_CFLAGS     "-I\${includedir}")
set(PKG_CONFIG_REQUIRES   "libusb-1.0")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
	DESTINATION ${STLINK_LIBRARY_PATH}/pkgconfig/
)
