cmake_minimum_required(VERSION 3.0)

# XXX change to the appropriate name
project("boilerplate")

find_package(PkgConfig)
pkg_search_module(JSONC REQUIRED json-c)

find_library(EV ev)
find_library(WEBCOM-C webcom-c)

if(NOT ${JSONC_FOUND})
	message(FATAL_ERROR "could not find prerequisite: libjson-c. Please install the libjson-c binaries and dev headers.")
endif(NOT ${JSONC_FOUND})

if(${EV} STREQUAL "EV-NOTFOUND")
	message(FATAL_ERROR "could not find required library: ev. Please install the libev binaries and dev headers.")
endif(${EV} STREQUAL "EV-NOTFOUND")

if(${WEBCOM-C} STREQUAL "WEBCOM-C-NOTFOUND")
	message(FATAL_ERROR "could not find required library: webcom-c. Please download, build and install the Webcom C SDK.")
endif(${WEBCOM-C} STREQUAL "WEBCOM-C-NOTFOUND")

add_executable(
# XXX set the desired executable name here
	boilerplate
# XXX then give the list of source files required to build the program
	src/boilerplate.c
#	src/other-source-file1.c
#	src/other-source-file2.c
#	...
)

target_include_directories(
	boilerplate
	PUBLIC
	${JSONC_INCLUDE_DIRS}
)
target_link_libraries(
	boilerplate
	${JSONC_LIBRARIES}
	${WEBCOM-C}
	${EV}
)