cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c-examples"
	LANGUAGES C
)

find_package(PkgConfig)

pkg_search_module(JSONC REQUIRED json-c)
pkg_search_module(NCURSES REQUIRED ncurses)

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

