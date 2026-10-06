###
# Generate the stlink-lib API documentation with Doxygen
###

if (NOT STLINK_GENERATE_API_DOCS)
    message(STATUS "API documentation generation disabled")
    return()
endif()

find_package(Doxygen REQUIRED)

set(DOXYGEN_PROJECT_NAME "stlink-lib")
set(DOXYGEN_PROJECT_NUMBER ${PROJECT_VERSION})
set(DOXYGEN_PROJECT_BRIEF "${PROJECT_DESCRIPTION}")
set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md)
set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
set(DOXYGEN_STRIP_FROM_INC_PATH ${CMAKE_SOURCE_DIR}/inc ${CMAKE_SOURCE_DIR}/src/stlink-lib)

set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
set(DOXYGEN_DISTRIBUTE_GROUP_DOC YES)
set(DOXYGEN_SORT_MEMBER_DOCS NO)          # keep the order of the headers
set(DOXYGEN_EXTRACT_ALL NO)
set(DOXYGEN_EXTRACT_STATIC NO)
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
set(DOXYGEN_PREDEFINED "PRINTF_ARRT=")

set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_GENERATE_LATEX NO)
set(DOXYGEN_GENERATE_TREEVIEW YES)

set(DOXYGEN_QUIET YES)
set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
set(DOXYGEN_WARN_NO_PARAMDOC YES)

# Public API: the installed headers applications use
set(STLINK_API_DOC_INPUTS
    ${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md
    ${CMAKE_CURRENT_SOURCE_DIR}/groups.dox
    ${CMAKE_SOURCE_DIR}/inc/stlink.h
    ${CMAKE_SOURCE_DIR}/inc/stlink_backend.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/chipid.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/common_flash.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/flash_loader.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/helper.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/logging.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/map_file.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/md5.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/option_bytes.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/read_write.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/remote.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/sg_legacy.h
    ${CMAKE_SOURCE_DIR}/src/stlink-lib/usb.h
    )

# Internal functions: exported, but not part of the public API
if (STLINK_API_DOCS_INTERNAL)
    set(DOXYGEN_ENABLED_SECTIONS STLINK_INTERNAL)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)   # internal functions are documented briefly
    set(DOXYGEN_WARN_NO_PARAMDOC NO)
    list(APPEND STLINK_API_DOC_INPUTS
        ${CMAKE_SOURCE_DIR}/src/stlink-lib/calculate.h
        ${CMAKE_SOURCE_DIR}/src/stlink-lib/lib_md5.h
        ${CMAKE_SOURCE_DIR}/src/stlink-lib/progress.h
        ${CMAKE_SOURCE_DIR}/src/stlink-env/fs/stlink_fs.h
        ${CMAKE_SOURCE_DIR}/src/stlink-env/pthreads/stlink_threads.h
        )
endif()

doxygen_add_docs(doc-api ${STLINK_API_DOC_INPUTS}
                 ALL
                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                 COMMENT "Generating the stlink-lib API documentation")

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/api OPTIONAL)
