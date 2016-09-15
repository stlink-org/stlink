set(MANPAGES
	st-util
)

# Only generate manpages with pandoc in Debug builds
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	include(pandocology)

	foreach(manpage ${MANPAGES})
		add_document(
			${manpage}.1
			SOURCES ${manpage}.md
			PANDOC_DIRECTIVES -s -t man
			PRODUCT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)

		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${manpage}.1
			DESTINATION share/man/man1
		)
	endforeach()
else()
	# TODO(xor-gate): we should generate a install list for Release and Debug targets
	foreach(manpage ${MANPAGES})
		install(FILES ${manpage}.1
			DESTINATION share/man/man1
		)
	endforeach()
endif()
