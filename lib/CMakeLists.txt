cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c-lib"
	LANGUAGES C
	VERSION 0.0.1
)

find_package(PkgConfig)

pkg_search_module(WEBSOCKETS REQUIRED libwebsockets)
pkg_search_module(JSONC REQUIRED json-c)

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
