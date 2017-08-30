cmake_minimum_required(VERSION 3.0)

project("webcom-sdk-c-lib"
	LANGUAGES C
)

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
	"${webcom-sdk-c-lib_SOURCE_DIR}/compat.h.in"
	"${webcom-sdk-c-lib_SOURCE_DIR}/compat.h")

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

install(
	DIRECTORY
		"${webcom-sdk-c-lib_SOURCE_DIR}/../include/webcom-c/"
	DESTINATION
		include/webcom-c
	FILES_MATCHING PATTERN
		"*.h"
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
