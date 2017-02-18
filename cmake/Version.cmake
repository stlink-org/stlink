# Determine project version
# * Using Git
# * Local .version file
find_package (Git QUIET)
if (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
	# Working off a git repo, using git versioning
	# Check if HEAD is pointing to a tag
	execute_process (
		COMMAND             "${GIT_EXECUTABLE}" describe --always
		WORKING_DIRECTORY   "${PROJECT_SOURCE_DIR}"
		OUTPUT_VARIABLE     PROJECT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	# If the sources have been changed locally, add -dirty to the version.
	execute_process (
	COMMAND             "${GIT_EXECUTABLE}" diff --quiet
	WORKING_DIRECTORY   "${PROJECT_SOURCE_DIR}"
	RESULT_VARIABLE     res)

	if (res EQUAL 1)
		set (PROJECT_VERSION "${PROJECT_VERSION}-dirty")
	endif()

	string(REGEX REPLACE "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)(-[.0-9A-Za-z-]+)?([+][.0-9A-Za-z-]+)?$"
		"\\1;\\2;\\3" PROJECT_VERSION_LIST ${PROJECT_VERSION})
	list(LENGTH PROJECT_VERSION_LIST len)
	if(len EQUAL 3)
		list(GET PROJECT_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
		list(GET PROJECT_VERSION_LIST 1 PROJECT_VERSION_MINOR)
		list(GET PROJECT_VERSION_LIST 2 PROJECT_VERSION_PATCH)
	endif()
elseif(EXISTS ${PROJECT_SOURCE_DIR}/.version)
	#  If git is not available (e.g. when building from source package)
	#  we can extract the package version from .version file.
	file (STRINGS .version PROJECT_VERSION)

	# TODO create function to extract semver from file or string and check if it is correct instead of copy-pasting
	string(REGEX REPLACE "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)(-[.0-9A-Za-z-]+)?([+][.0-9A-Za-z-]+)?$"
                     "\\1;\\2;\\3" PROJECT_VERSION_LIST ${PROJECT_VERSION})
	list(GET PROJECT_VERSION_LIST 0 PROJECT_VERSION_MAJOR)
	list(GET PROJECT_VERSION_LIST 1 PROJECT_VERSION_MINOR)
	list(GET PROJECT_VERSION_LIST 2 PROJECT_VERSION_PATCH)
else()
	message(FATAL_ERROR "Unable to determine project version")
endif()

message(STATUS "stlink version: ${PROJECT_VERSION}")
message(STATUS "          Major ${PROJECT_VERSION_MAJOR} Minor ${PROJECT_VERSION_MINOR} Patch ${PROJECT_VERSION_PATCH}")
