cmake_minimum_required(VERSION 3.0)
project("webcom-sdk-c-doc"
	LANGUAGES NONE
	VERSION 0.0.1
)

include(FindDoxygen)

option(BUILD_HTML_DOCUMENTATION "Create the HTML based API documentation (requires Doxygen)" OFF)
option(BUILD_MAN_PAGES "Create and install the API man pages (requires Doxygen)" ON)

if(BUILD_HTML_DOCUMENTATION OR BUILD_MAN_PAGES)
	if(BUILD_HTML_DOCUMENTATION)
		set(DOXYGEN_BUILD_HTML_DOCUMENTATION YES)
	else()
		set(DOXYGEN_BUILD_HTML_DOCUMENTATION NO)
	endif()

	if(BUILD_MAN_PAGES)
		set(DOXYGEN_BUILD_MAN_PAGES YES)
	else()
		set(DOXYGEN_BUILD_MAN_PAGES NO)
	endif()

	#### documentation

	configure_file(${webcom-sdk-c-doc_SOURCE_DIR}/doxygen.conf.in doxygen.conf)

	add_custom_command(
		OUTPUT
			doxygen.stamp
		DEPENDS
			"${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf"
			"${webcom-sdk-c-doc_SOURCE_DIR}/../include/webcom-c/webcom-libev.h"
		IMPLICIT_DEPENDS C
			"${webcom-sdk-c-doc_SOURCE_DIR}/../include/webcom-c/webcom.h"
		COMMAND
			${DOXYGEN_EXECUTABLE} doxygen.conf
		COMMAND
			${CMAKE_COMMAND} -E touch doxygen.stamp
		COMMENT
			"Generating API documentation with Doxygen"
		VERBATIM
	)

	add_custom_target(
		doc ALL
		DEPENDS doxygen.stamp
	)

	#### install
	if(BUILD_MAN_PAGES)
		install(
			FILES
				"${CMAKE_CURRENT_BINARY_DIR}/man/man3/webcom-connection.3"
				"${CMAKE_CURRENT_BINARY_DIR}/man/man3/webcom-general.3"
				"${CMAKE_CURRENT_BINARY_DIR}/man/man3/webcom-messages.3"
				"${CMAKE_CURRENT_BINARY_DIR}/man/man3/webcom-parser.3"
				"${CMAKE_CURRENT_BINARY_DIR}/man/man3/webcom-requests.3"
				"${CMAKE_CURRENT_BINARY_DIR}/man/man3/webcom-utils.3"
			DESTINATION
				man/man3
		)
	endif()

endif(BUILD_HTML_DOCUMENTATION OR BUILD_MAN_PAGES)
