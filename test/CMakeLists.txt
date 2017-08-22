cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c-tests"
	LANGUAGES C
)

find_package(PkgConfig)

pkg_search_module(JSONC REQUIRED json-c)
find_library(LIBEV ev)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

#### tests

enable_testing()

## parser
add_executable(
	webcom-test-parser 
	test-parser.c
)

target_include_directories(
	webcom-test-parser
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-parser
	webcom-c
)

add_test(
	NAME parser
	COMMAND webcom-test-parser
)

## message builder
add_executable(
	webcom-test-msg
	test-msg.c
)

target_include_directories(
	webcom-test-msg
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-msg
	webcom-c
)

add_test(
	NAME msg
	COMMAND webcom-test-msg
)

## connection

add_executable(
	webcom-test-cnx
	test-cnx.c
)

target_include_directories(
	webcom-test-cnx
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
	${JSONC_INCLUDE_DIRS}
)

target_link_libraries(
	webcom-test-cnx
	webcom-c
	${JSONC_LIBRARIES}
	${LIBEV}
)

add_test(
	NAME cnx
	COMMAND webcom-test-cnx
)

## on data handlers hashtable

add_executable(
	webcom-test-data-handlers
	test-on-data.c
)

target_include_directories(
	webcom-test-data-handlers
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
	${JSONC_INCLUDE_DIRS}
)

add_test(
	NAME "data-handlers"
	COMMAND webcom-test-data-handlers
)

## pending request handlers hashtable

add_executable(
	webcom-test-req
	test-req.c
)

target_include_directories(
	webcom-test-req
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
	${JSONC_INCLUDE_DIRS}
)

add_test(
	NAME "request-handlers"
	COMMAND webcom-test-req
)

## misc webcom internals tests

add_executable(
	webcom-test-misc
	test-misc.c
)

target_include_directories(
	webcom-test-misc
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
	${JSONC_INCLUDE_DIRS}
)

add_test(
	NAME "misc"
	COMMAND webcom-test-misc
)