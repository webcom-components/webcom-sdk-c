cmake_minimum_required(VERSION 3.0)

# XXX change to the appropriate name
project("boilerplate")

find_library(EV ev)
find_package(webcom-c REQUIRED)

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

	PRIVATE
		${WEBCOM_C_INCLUDE_DIRS}
)

target_link_libraries(
	boilerplate

	${WEBCOM_C_LIBRARIES}
	${EV}
)