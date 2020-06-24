# get_version.cmake
# Determine project version by using Git or a local .version file

set(__detect_version 0)

find_package(Git)
set(ERROR_FLAG "0")

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
            set(PROJECT_VERSION "${PROJECT_VERSION}-dirty")
        endif ()

        # Strip a leading v off of the version as proceeding code expects just the version numbering.
        string(REGEX REPLACE "^v" "" PROJECT_VERSION ${PROJECT_VERSION})

        # Read version string
        string(REGEX REPLACE "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)(-[.0-9A-Za-z-]+)?([+][.0-9A-Za-z-]+)?$"
                             "\\1;\\2;\\3" PROJECT_VERSION_LIST ${PROJECT_VERSION})
        list(LENGTH PROJECT_VERSION_LIST len)
        if (len EQUAL 3)
            list(GET PROJECT_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
            list(GET PROJECT_VERSION_LIST 1 PROJECT_VERSION_MINOR)
            list(GET PROJECT_VERSION_LIST 2 PROJECT_VERSION_PATCH)
            set(__detect_version 1)
            set(__version_str "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

            # Compare git-Version with version read from .version file in source folder
            if (EXISTS "${PROJECT_SOURCE_DIR}/.version")

                # Local .version file found, read version string...
                file(READ  "${PROJECT_SOURCE_DIR}/.version" __version_file)

                # ...the version does not match with git-version string
                if (NOT __version_str STREQUAL __version_file)
                    message(STATUS "Rewrite ${PROJECT_SOURCE_DIR}/.version with ${__version_str}!")
                endif ()

            else (NOT EXISTS "${PROJECT_SOURCE_DIR}/.version")

                # No local .version file found: Create a new one...
                file(WRITE "${PROJECT_SOURCE_DIR}/.version" ${__version_str})

            endif ()

            message(STATUS "stlink version: ${PROJECT_VERSION}")
            message(STATUS "Major ${PROJECT_VERSION_MAJOR} Minor ${PROJECT_VERSION_MINOR} Patch ${PROJECT_VERSION_PATCH}")

        else (len EQUAL 3)
            message(STATUS "Failed to extract version parts from \"${PROJECT_VERSION}\"")
            set(ERROR_FLAG "1")
        endif (len EQUAL 3)

    else (GIT_DESCRIBE_RESULT EQUAL 0)
        message(WARNING "git describe failed: ${GIT_DESCRIBE_ERROR}")
        set(ERROR_FLAG "1")
    endif(GIT_DESCRIBE_RESULT EQUAL 0)
endif ()

##
# Failure to read version via git
# Possible cases:
# -> git is not found or
# -> /.git does not exist or
# -> GIT_DESCRIBE failed or
# -> version string is of invalid format
##
if (NOT GIT_FOUND OR NOT EXISTS "${PROJECT_SOURCE_DIR}/.git" OR ERROR_FLAG EQUAL 1)
    message(STATUS "Git and/or repository not found.") # e.g. when building from source package
    message(STATUS "Try to detect version from \"${PROJECT_SOURCE_DIR}/.version\" file instead...")
    if (EXISTS ${PROJECT_SOURCE_DIR}/.version)
        file(STRINGS .version PROJECT_VERSION)

        # Read version string
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
    else (EXISTS ${PROJECT_SOURCE_DIR}/.version)
        message(STATUS "File \"${PROJECT_SOURCE_DIR}/.version\" does not exist.")
        message(FATAL_ERROR "Unable to determine project version")
    endif ()
endif ()
