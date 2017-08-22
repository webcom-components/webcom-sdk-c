cmake_minimum_required(VERSION 3.0)

project("webcom-sdk-c-lib"
	LANGUAGES C
	VERSION 0.0.1
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
			WORKING_DIRECTORY "${webcom-sdk-c-lib_SOURCE_DIR}"
			OUTPUT_VARIABLE PROJECT_VERSION_EXTRA
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	endif(PROJECT_VERSION_EXTRA STREQUAL "git")
endif(PROJECT_VERSION_EXTRA)

find_package(PkgConfig)

pkg_search_module(WEBSOCKETS REQUIRED libwebsockets)
pkg_search_module(JSONC REQUIRED json-c)

configure_file(
	"${webcom-sdk-c-lib_SOURCE_DIR}/../include/webcom-c/webcom-config.h.in"
	"${webcom-sdk-c-lib_SOURCE_DIR}/../include/webcom-c/webcom-config.h")

#### libwebcom-c.so

add_library(
	webcom-c
	SHARED
	parser.c
	message.c
	connection.c
	webcom.c
	request.c
	event.c
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
	${webcom-sdk-c-lib_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-c
	${WEBSOCKETS_LIBRARIES}
	${JSONC_LIBRARIES}
)

#### install

install(TARGETS
	webcom-c
	EXPORT webcom-c-targets
	LIBRARY DESTINATION lib
)

install(EXPORT webcom-c-targets
	FILE webcom-c-targets.cmake
	NAMESPACE webcom::
	DESTINATION lib/cmake/webcom
	)

file(GLOB INSTALL_HEADERS "${webcom-sdk-c-lib_SOURCE_DIR}/../include/webcom-c/*.h")

install(FILES
	${INSTALL_HEADERS}
	DESTINATION include/webcom-c
)

#### code checker

add_custom_target(
	cppcheck-lib
	COMMAND
	cppcheck --enable=warning,performance,portability,style
	--std=posix --verbose --quiet --template=gcc
	"-I${JSONC_INCLUDE_DIRS}" "-I${webcom-sdk-c-lib_SOURCE_DIR}/../include"
	lib/
	WORKING_DIRECTORY ${webcom-sdk-c_SOURCE_DIR}
)
