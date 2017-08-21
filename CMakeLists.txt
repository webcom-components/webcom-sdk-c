cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c"
	LANGUAGES C
)

#### sub-projects
add_subdirectory(lib)
add_subdirectory(examples)
add_subdirectory(doc)
enable_testing()
add_subdirectory(test)

#### code checker

add_custom_target(
	cppcheck-lib
	COMMAND
	cppcheck --enable=warning,performance,portability,style
	--std=posix --verbose --quiet --template=gcc
	"-I${JSONC_INCLUDE_DIRS}" "-I${webcom-sdk-c_SOURCE_DIR}/include"
	lib/
	WORKING_DIRECTORY ${webcom-sdk-c_SOURCE_DIR}
)

add_custom_target(
	cppcheck-tests
	COMMAND
	cppcheck --enable=warning,performance,portability,style
	--std=posix --library=qt.cfg --verbose --quiet --template=gcc
	"-I${JSONC_INCLUDE_DIRS}" "-I${webcom-sdk-c_SOURCE_DIR}/include"
	test/
	WORKING_DIRECTORY ${webcom-sdk-c_SOURCE_DIR}
)

#### packaging
include("${webcom-sdk-c_SOURCE_DIR}/pkg/pkg.cmake")