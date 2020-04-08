# Determine project version
# * Using Git
# * Local .version file

set(__detect_version 0)

find_package(Git)

if (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Working off a git repo, using git versioning
    # Check if HEAD is pointing to a tag
    execute_process (
        COMMAND             "${GIT_EXECUTABLE}" describe --always --tag
        WORKING_DIRECTORY   "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE     PROJECT_VERSION
        RESULT_VARIABLE     GIT_DESCRIBE_RESULT
        ERROR_VARIABLE      GIT_DESCRIBE_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    if (GIT_DESCRIBE_RESULT EQUAL 0)
        # If the sources have been changed locally, add -dirty to the version.
        execute_process (
            COMMAND             "${GIT_EXECUTABLE}" diff --quiet
            WORKING_DIRECTORY   "${PROJECT_SOURCE_DIR}"
            RESULT_VARIABLE     res
            )

        if (res EQUAL 1)
            set (PROJECT_VERSION "${PROJECT_VERSION}-dirty")
        endif ()

        # strip a leading v off of the version as proceeding code expectes just the version numbering.
        string(REGEX REPLACE "^v" "" PROJECT_VERSION ${PROJECT_VERSION})

        string(REGEX REPLACE "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)(-[.0-9A-Za-z-]+)?([+][.0-9A-Za-z-]+)?$"
                             "\\1;\\2;\\3" PROJECT_VERSION_LIST ${PROJECT_VERSION})
        list(LENGTH PROJECT_VERSION_LIST len)
        if (len EQUAL 3)
            list(GET PROJECT_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
            list(GET PROJECT_VERSION_LIST 1 PROJECT_VERSION_MINOR)
            list(GET PROJECT_VERSION_LIST 2 PROJECT_VERSION_PATCH)
            set(__detect_version 1)
            set(__version_str "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
            if (EXISTS "${PROJECT_SOURCE_DIR}/.version")
                file(READ  "${PROJECT_SOURCE_DIR}/.version" __version_file)
                if (NOT __version_str STREQUAL __version_file)
                    message(STATUS "Rewrite ${PROJECT_SOURCE_DIR}/.version with ${__version_str}")
                endif ()
            else ()
                file(WRITE "${PROJECT_SOURCE_DIR}/.version" ${__version_str})
            endif ()
        else ()
            message(STATUS "Fail to extract version parts from \"${PROJECT_VERSION}\"")
        endif ()

    else(GIT_DESCRIBE_RESULT EQUAL 0)
        message(WARNING "git describe failed.")
        message(WARNING "${GIT_DESCRIBE_ERROR}")
    endif(GIT_DESCRIBE_RESULT EQUAL 0)
else ()
    message(STATUS "Git or repo not found.")
endif ()

if (NOT __detect_version)
    message(STATUS "Try to detect version from \"${PROJECT_SOURCE_DIR}/.version\" file.")
    if (EXISTS ${PROJECT_SOURCE_DIR}/.version)
        #  If git is not available (e.g. when building from source package)
        #  we can extract the package version from .version file.
        file(STRINGS .version PROJECT_VERSION)

        # TODO create function to extract semver from file or string and check if it is correct instead of copy-pasting
        string(REGEX REPLACE "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)(-[.0-9A-Za-z-]+)?([+][.0-9A-Za-z-]+)?$"
                             "\\1;\\2;\\3" PROJECT_VERSION_LIST ${PROJECT_VERSION})
        list(LENGTH PROJECT_VERSION_LIST len)
        if (len EQUAL 3)
            list(GET PROJECT_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
            list(GET PROJECT_VERSION_LIST 1 PROJECT_VERSION_MINOR)
            list(GET PROJECT_VERSION_LIST 2 PROJECT_VERSION_PATCH)
            set(__detect_version 1)
        else ()
            message(STATUS "Fail to extract version parts from \"${PROJECT_VERSION}\"")
        endif ()
    else ()
        message(STATUS "File \"${PROJECT_SOURCE_DIR}/.version\" did not exists.")
    endif ()
    message(FATAL_ERROR "Unable to determine project version")
endif ()

message(STATUS "stlink version: ${PROJECT_VERSION}")
message(STATUS "Major ${PROJECT_VERSION_MAJOR} Minor ${PROJECT_VERSION_MINOR} Patch ${PROJECT_VERSION_PATCH}")
