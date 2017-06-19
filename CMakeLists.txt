cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c")

find_package(PkgConfig)

pkg_search_module(NOPOLL REQUIRED nopoll)
pkg_search_module(JSONC REQUIRED json-c)
pkg_search_module(NCURSES REQUIRED ncurses)

find_library(EV ev)

if(NOT ${JSONC_FOUND})
	message(FATAL_ERROR "could not find prerequisite: libjson-c")
endif(NOT ${JSONC_FOUND})

if(NOT ${NOPOLL_FOUND})
	message(FATAL_ERROR "could not find prerequisite: libnopoll")
endif(NOT ${NOPOLL_FOUND})

if(${EV-NOTFOUND})
	message(FATAL_ERROR "could not find prerequisite: libev")
endif(${EV-NOTFOUND})

if(NOT ${NCURSES_FOUND})
	message(FATAL_ERROR "could not find prerequisite: libncurses")
endif(NOT ${NCURSES_FOUND})

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

link_directories(
	${NOPOLL_LIBRARY_DIRS}
	${JSONC_LIBRARY_DIRS}
)

#### libwebcom-c.so

add_library(
	webcom-c
	SHARED
	src/parser.c
	src/message.c
	src/connection.c
	src/webcom.c
)

target_include_directories(
	webcom-c
	INTERFACE
	${JSONC_INCLUDE_DIRS}
	PUBLIC
	${NOPOLL_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(
	webcom-c
	${NOPOLL_LIBRARIES}
	${JSONC_LIBRARIES}
)

#### documentation

add_custom_target(
	doc
	COMMAND doxygen doxygen.conf
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

#### examples

## legorange

add_executable(
	legorange
	examples/legorange.c
)

target_include_directories(
	legorange
	PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${JSONC_INCLUDE_DIRS}
)

target_link_libraries(
	legorange
	webcom-c
	${JSONC_LIBRARIES}
	${EV}
)

## wcchat

add_executable(
	wcchat
	examples/wcchat.c
)

target_include_directories(
	wcchat
	PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${JSONC_INCLUDE_DIRS}
	${NCURSES_INCLUDE_DIRS}
)

target_link_libraries(
	wcchat
	webcom-c
	${JSONC_LIBRARIES}
	${EV}
	${NCURSES_LIBRARIES}
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
	webcom-c
)

add_test(
	NAME parser
	COMMAND webcom-test-parser
)

## message builder
add_executable(
	webcom-test-msg
	test/test-msg.c
)

target_include_directories(
	webcom-test-msg
	PUBLIC
	${CMAKE_SOURCE_DIR}/include
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
	test/test-cnx.c
)

target_include_directories(
	webcom-test-cnx
	PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${JSONC_INCLUDE_DIRS}
)

target_link_libraries(
	webcom-test-cnx
	webcom-c
	${JSONC_LIBRARIES}
	${EV}
)

add_test(
	NAME cnx
	COMMAND webcom-test-cnx
)

#### code checker

add_custom_target(
	cppcheck-lib
	COMMAND
	cppcheck --enable=warning,performance,portability,style
	--std=posix --library=qt.cfg --verbose --quiet --template=gcc
	"-I${JSONC_INCLUDE_DIRS}" "-I${NOPOLL_INCLUDE_DIRS}" "-I${CMAKE_SOURCE_DIR}/include"
	src/
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(
	cppcheck-tests
	COMMAND
	cppcheck --enable=warning,performance,portability,style
	--std=posix --library=qt.cfg --verbose --quiet --template=gcc
	"-I${JSONC_INCLUDE_DIRS}" "-I${NOPOLL_INCLUDE_DIRS}" "-I${CMAKE_SOURCE_DIR}/include"
	test/
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
