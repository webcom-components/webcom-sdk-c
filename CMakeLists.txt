cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c"
	LANGUAGES C
	VERSION 0.4.99
)

# Extra version information appearing after "Major.minor.patch" in the library
# version string.
# set to "false" to disable
# set to "git" to use the current git revision abbbreviated hash
# set to anything else to specify anything else
set(PROJECT_VERSION_EXTRA git)


if(PROJECT_VERSION_EXTRA)
	if(PROJECT_VERSION_EXTRA STREQUAL "git")
		execute_process(
			COMMAND git rev-parse --short HEAD
			WORKING_DIRECTORY "${webcom-sdk-c_SOURCE_DIR}"
			OUTPUT_VARIABLE PROJECT_VERSION_EXTRA
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	endif(PROJECT_VERSION_EXTRA STREQUAL "git")
endif(PROJECT_VERSION_EXTRA)

configure_file(
	"${webcom-sdk-c_SOURCE_DIR}/include/webcom-c/webcom-config.h.in"
	"${webcom-sdk-c_SOURCE_DIR}/include/webcom-c/webcom-config.h")

#### sub-projects
add_subdirectory(lib)
add_subdirectory(examples)
add_subdirectory(doc)
enable_testing()
add_subdirectory(test)

#### packaging
include("${webcom-sdk-c_SOURCE_DIR}/pkg/pkg.cmake")