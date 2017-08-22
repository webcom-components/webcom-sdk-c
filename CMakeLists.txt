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

#### packaging
include("${webcom-sdk-c_SOURCE_DIR}/pkg/pkg.cmake")