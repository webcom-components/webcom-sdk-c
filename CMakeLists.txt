cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c")

find_package(PkgConfig)

pkg_search_module(WEBSOCKETS REQUIRED libwebsockets)
pkg_search_module(JSONC REQUIRED json-c)

if(NOT ${WEBSOCKETS_FOUND})
	message(FATAL_ERROR "could not find prerequisite: libwebsockets")
endif(NOT ${WEBSOCKETS_FOUND})

if(NOT ${JSONC_FOUND})
	message(FATAL_ERROR "could not find prerequisite: libjson-c")
endif(NOT ${JSONC_FOUND})

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

#### libwebcom-c.so

add_library(
	webcom-c
	SHARED
	src/parser.c
)

target_include_directories(
	webcom-c
	INTERFACE
	${WEBSOCKETS_INCLUDE_DIRS}
	${JSONC_INCLUDE_DIRS}
	PUBLIC
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(
	webcom-c
	${WEBSOCKETS_LIBRARIES}
	${JSONC_LIBRARIES}
)

#### documentation

add_custom_target(
	doc
	COMMAND doxygen doxygen.conf
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

#### tests

enable_testing()

## parser
add_executable(
	webcom-test-parser 
	test/test-parser.c
)

target_include_directories(
	webcom-test-parser
	PUBLIC
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(
	webcom-test-parser
	${CMAKE_BINARY_DIR}/libwebcom-c.so
)

add_test(
	NAME parser
	COMMAND webcom-test-parser
)

