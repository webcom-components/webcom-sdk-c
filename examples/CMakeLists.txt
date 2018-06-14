cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c-examples"
	LANGUAGES C
)

find_package(PkgConfig)

pkg_search_module(JSONC REQUIRED json-c)
pkg_search_module(NCURSES REQUIRED ncurses)

option(WITH_LIBUV "Enable libuv event loop support" OFF)
option(WITH_LIBEVENT "Enable libevent event loop support" OFF)

link_directories(
	${JSONC_LIBRARY_DIRS}
	${NCURSES_LIBRARY_DIRS}
	)

find_library(LIBEV ev)

find_path(
	LIBEV_INCLUDE_DIR
	ev.h
	)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

#### examples

## legorange

add_executable(
	legorange
	legorange.c
)

target_include_directories(
	legorange
	PRIVATE
	${webcom-sdk-c-examples_SOURCE_DIR}/../include
	${JSONC_INCLUDE_DIRS}
	${LIBEV_INCLUDE_DIR}
)

target_link_libraries(
	legorange
	webcom-c
	${JSONC_LIBRARIES}
	${LIBEV}
)

## legorange-libuv
if(${WITH_LIBUV})
	pkg_search_module(LIBUV REQUIRED libuv)

	add_executable(
		legorange-libuv
		legorange-libuv.c
	)

	target_include_directories(
		legorange-libuv
		PRIVATE
		${webcom-sdk-c-examples_SOURCE_DIR}/../include
		${JSONC_INCLUDE_DIRS}
		${LIBUV_INCLUDE_DIR}
	)

	target_link_libraries(
		legorange-libuv
		webcom-c
		${JSONC_LIBRARIES}
		${LIBUV_LIBRARIES}
	)
endif()

## legorange-libevent
if(${WITH_LIBEVENT})
	pkg_search_module(LIBEVENT REQUIRED libevent)

	add_executable(
		legorange-libevent
		legorange-libevent.c
	)

	target_include_directories(
		legorange-libevent
		PRIVATE
		${webcom-sdk-c-examples_SOURCE_DIR}/../include
		${JSONC_INCLUDE_DIRS}
		${LIBEVENT_INCLUDE_DIR}
	)

	target_link_libraries(
		legorange-libevent
		webcom-c
		${JSONC_LIBRARIES}
		${LIBEVENT_LIBRARIES}
	)
endif()

## wcchat

add_executable(
	wcchat
	wcchat.c
)

target_include_directories(
	wcchat
	PRIVATE
	${webcom-sdk-c-examples_SOURCE_DIR}/../include
	${JSONC_INCLUDE_DIRS}
	${NCURSES_INCLUDE_DIRS}
)

target_link_libraries(
	wcchat
	webcom-c
	${JSONC_LIBRARIES}
	${LIBEV}
	${NCURSES_LIBRARIES}
)

#### install

install(TARGETS
	legorange
	wcchat
	RUNTIME DESTINATION bin
)

