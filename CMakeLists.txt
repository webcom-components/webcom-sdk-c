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

#### webcom-c library



find_package(PkgConfig)

pkg_search_module(WEBSOCKETS REQUIRED libwebsockets)
pkg_search_module(JSONC REQUIRED json-c)

link_directories(
	${WEBSOCKETS_LIBRARY_DIRS}
	${JSONC_LIBRARY_DIRS}
	)

include(CheckIncludeFile)
include(CheckSymbolExists)

check_include_file("alloca.h" HAVE_ALLOCA_H)
check_symbol_exists("srand48_r" "stdlib.h" HAVE_RAND48_R)
check_include_file("endian.h" HAVE_ENDIAN_H)
check_include_file("sys/endian.h" HAVE_SYS_ENDIAN_H)

configure_file(
	"${webcom-sdk-c_SOURCE_DIR}/lib/compat.h.in"
	"${webcom-sdk-c_SOURCE_DIR}/lib/compat.h")


configure_file(
	"${webcom-sdk-c_SOURCE_DIR}/include/webcom-c/webcom-config.h.in"
	"${webcom-sdk-c_SOURCE_DIR}/include/webcom-c/webcom-config.h")


#### libwebcom-c.so

add_library(
	webcom-c
	SHARED
	lib/parser.c
	lib/message.c
	lib/connection.c
	lib/webcom.c
	lib/request.c
	lib/event.c
)

target_compile_options(
	webcom-c
	PRIVATE
	-Werror
	-Wall
	-Wextra
)

target_include_directories(
	webcom-c
	PRIVATE
	${JSONC_INCLUDE_DIRS}
	${WEBSOCKETS_INCLUDE_DIRS}
	${webcom-sdk-c_SOURCE_DIR}/include
)

target_link_libraries(
	webcom-c
	${WEBSOCKETS_LIBRARIES}
	${JSONC_LIBRARIES}
)


#### install

set(LIBRARY_INSTALL_DIR lib)
set(INCLUDE_INSTALL_DIR include)
set(CONFIG_INSTALL_DIR lib/cmake/webcom-c)

install(
	TARGETS
		webcom-c
	EXPORT
		webcom-c-targets
	LIBRARY DESTINATION
		${LIBRARY_INSTALL_DIR}
)

install(
	DIRECTORY
		"${webcom-sdk-c_SOURCE_DIR}/include/"
	DESTINATION
		${INCLUDE_INSTALL_DIR}
	FILES_MATCHING PATTERN
		"webcom-c/*.h"
)

### CMake library interface export

install(
	EXPORT
		webcom-c-targets
	FILE
		webcom-c-targets.cmake
	NAMESPACE
		webcom-c::
	DESTINATION
		${CONFIG_INSTALL_DIR}
	)

include(CMakePackageConfigHelpers)

configure_package_config_file(
	webcom-c-config.cmake.in
	webcom-c-config.cmake
	INSTALL_DESTINATION
		${CONFIG_INSTALL_DIR}
	PATH_VARS
		CONFIG_INSTALL_DIR
		INCLUDE_INSTALL_DIR
)

write_basic_package_version_file(
	webcom-c-config-version.cmake
	COMPATIBILITY
		SameMajorVersion
)

install(
	FILES
		"${CMAKE_CURRENT_BINARY_DIR}/webcom-c-config-version.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/webcom-c-config.cmake"
	DESTINATION
		${CONFIG_INSTALL_DIR}
	)

#### code checker

add_custom_target(
	cppcheck-lib
	COMMAND
		cppcheck --enable=warning,performance,portability,style
		--std=posix --verbose --quiet --template=gcc
		"-I${JSONC_INCLUDE_DIRS}" "-I${webcom-sdk-c_SOURCE_DIR}/include"
		lib/
	WORKING_DIRECTORY
		${webcom-sdk-c_SOURCE_DIR}
)

#### sub-projects
add_subdirectory(examples)
add_subdirectory(doc)
enable_testing()
add_subdirectory(test)

set_target_properties(webcom-c
	PROPERTIES
		VERSION
			${PROJECT_VERSION}
		SOVERSION
			${PROJECT_VERSION_MAJOR}
)

#### packaging
include("${webcom-sdk-c_SOURCE_DIR}/pkg/pkg.cmake")