cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c-doc"
	LANGUAGES NONE
	VERSION 0.0.1
)

include(FindDoxygen)

#### documentation

configure_file(${webcom-sdk-c-doc_SOURCE_DIR}/doxygen.conf.in doxygen.conf)

add_custom_target(
	doc
	COMMAND doxygen doxygen.conf
)

#### install

#[=[
install(TARGETS
	webcom-c
	EXPORT webcom-c-targets
	LIBRARY DESTINATION lib
)
#]=]