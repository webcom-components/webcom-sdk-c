cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c-tests"
	LANGUAGES C
)

find_package(PkgConfig)

pkg_search_module(JSONC REQUIRED json-c)
pkg_search_module(WEBSOCKETS REQUIRED libwebsockets)

link_directories(
	${JSONC_LIBRARY_DIRS}
	${WEBSOCKETS_LIBRARY_DIRS}
	)

find_library(LIBEV ev)

find_path(
	LIBEV_INCLUDE_DIR
	ev.h
	)

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
	${LIBEV_INCLUDE_DIR}
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
	${WEBSOCKETS_INCLUDE_DIRS}
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
	${WEBSOCKETS_INCLUDE_DIRS}
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
	${WEBSOCKETS_INCLUDE_DIRS}
)

add_test(
	NAME "misc"
	COMMAND webcom-test-misc
)

## auth

add_executable(
	webcom-test-auth
	test-auth.c
)

target_include_directories(
	webcom-test-auth
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
	${LIBEV_INCLUDE_DIR}
)

target_link_libraries(
	webcom-test-auth
	webcom-c
	${LIBEV}
)

add_test(
	NAME auth
	COMMAND webcom-test-auth
)

## path parser
add_executable(
	webcom-test-path
	test-path.c
)

target_include_directories(
	webcom-test-path
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-path
	webcom-c
)

## tests on the treenode objects
add_executable(
	webcom-test-treenode
	test-treenode.c
)

target_include_directories(
	webcom-test-treenode
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-treenode
	webcom-c
)

## tests on the treenode cache
add_executable(
	webcom-test-cache
	test-cache.c
)

target_include_directories(
	webcom-test-cache
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-cache
	webcom-c
)

## tests on the AVL container
add_executable(
	webcom-test-avl
	test-avl.c
)

target_include_directories(
	webcom-test-avl
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-avl
	webcom-c
)

## tests on data-cache callback triggering
add_executable(
	webcom-test-on
	test-on.c
)

target_include_directories(
	webcom-test-on
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-on
	webcom-c
)

## tests for on_value events
add_executable(
	webcom-test-on-value
	test-on-value.c
)

target_include_directories(
	webcom-test-on-value
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-on-value
	webcom-c
)

## tests for on_child events
add_executable(
	webcom-test-on-child
	test-on-child.c
)

target_include_directories(
	webcom-test-on-child
	PRIVATE
	${webcom-sdk-c-tests_SOURCE_DIR}/../include
)

target_link_libraries(
	webcom-test-on-child
	webcom-c
)

