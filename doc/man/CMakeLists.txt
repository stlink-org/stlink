###
# Generate manpages
###

set(MANPAGES st-util st-flash st-info)

# Only generate manpages with pandoc in Debug builds
if (${STLINK_GENERATE_MANPAGES})
    include(pandocology)
    foreach (manpage ${MANPAGES})
        add_document(
            ${manpage}.1
            SOURCES ${manpage}.md
            PANDOC_DIRECTIVES -s -t man
            PRODUCT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
    endforeach ()
else ()
    message(STATUS "Manpage generation disabled")
endif ()

# Install from output folder or this folder
foreach (manpage ${MANPAGES})
    if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${manpage}.1)
        set(f "${CMAKE_CURRENT_BINARY_DIR}/${manpage}.1")
    elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${manpage}.1")
        set(f "${CMAKE_CURRENT_SOURCE_DIR}/${manpage}.1")
    else()
        message(AUTHOR_WARNING "Manpage ${manpage} not generated")
    endif ()

    if (f AND NOT WIN32)
        install(FILES ${f} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/man/man1)
        unset(f)
    endif ()
endforeach ()
