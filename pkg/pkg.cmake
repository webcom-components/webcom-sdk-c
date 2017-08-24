#### webcom-c-sdk packaging

set(CPACK_PACKAGE_NAME "webcom-sdk-c")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
if(PROJECT_VERSION_EXTRA)
	set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}-${PROJECT_VERSION_EXTRA}")
endif(PROJECT_VERSION_EXTRA)
set(CPACK_PACKAGE_CONTACT "Camille Oudot <camille.oudot@orange.com>")
set(CPACK_PACKAGE_VENDOR "Orange S.A.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Webcom C SDK - dynamic library and development headers")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${webcom-sdk-c_SOURCE_DIR}/pkg/description.txt")

if(TARGET_PACKAGE_FLAVOUR)
	if(TARGET_PACKAGE_FLAVOUR STREQUAL "debian9")
		set(CPACK_GENERATOR "DEB")
		set(CPACK_DEBIAN_PACKAGE_DEPENDS  "libwebsockets8, libjson-c3, libssl1.1, libev4, libncurses5, libc6, ca-certificates")
		set(CPACK_SYSTEM_NAME "deb9")

	elseif(TARGET_PACKAGE_FLAVOUR STREQUAL "ubuntu16.04")
		set(CPACK_GENERATOR "DEB")
		set(CPACK_DEBIAN_PACKAGE_DEPENDS  "libwebsockets7, libjson-c2, libssl1.0.0, libev4, libncurses5, libc6, ca-certificates")
		set(CPACK_SYSTEM_NAME "xenial")

	elseif(TARGET_PACKAGE_FLAVOUR STREQUAL "ubuntu16.10")
		set(CPACK_GENERATOR "DEB")
		set(CPACK_DEBIAN_PACKAGE_DEPENDS  "libwebsockets7, libjson-c3, libssl1.0.0, libev4, libncurses5, libc6, ca-certificates")
		set(CPACK_SYSTEM_NAME "yakkety")

	elseif(TARGET_PACKAGE_FLAVOUR STREQUAL "ubuntu17.04")
		set(CPACK_GENERATOR "DEB")
		set(CPACK_DEBIAN_PACKAGE_DEPENDS  "libwebsockets8, libjson-c3, libssl1.0.0, libev4, libncurses5, libc6, ca-certificates")
		set(CPACK_SYSTEM_NAME "zesty")

	elseif(TARGET_PACKAGE_FLAVOUR STREQUAL "centos7")
		set(CPACK_GENERATOR "RPM")
		set(CPACK_RPM_PACKAGE_REQUIRES "libwebsockets, json-c, openssl-libs, libev, ncurses-libs")
		set(CPACK_SYSTEM_NAME "el7")

	elseif(TARGET_PACKAGE_FLAVOUR STREQUAL "fedora24")
		set(CPACK_GENERATOR "RPM")
		set(CPACK_RPM_PACKAGE_REQUIRES "libwebsockets, json-c, openssl-libs, libev, ncurses-libs")
		set(CPACK_SYSTEM_NAME "fc24")

	elseif(TARGET_PACKAGE_FLAVOUR STREQUAL "fedora25")
		set(CPACK_GENERATOR "RPM")
		set(CPACK_RPM_PACKAGE_REQUIRES "libwebsockets, json-c, openssl-libs, libev, ncurses-libs")
		set(CPACK_SYSTEM_NAME "fc25")

	elseif(TARGET_PACKAGE_FLAVOUR STREQUAL "fedora26")
		set(CPACK_GENERATOR "RPM")
		set(CPACK_RPM_PACKAGE_REQUIRES "libwebsockets, json-c, openssl-libs, libev, ncurses-libs")
		set(CPACK_SYSTEM_NAME "fc26")

	else(TARGET_PACKAGE_FLAVOUR STREQUAL "debian9")
		message(FATAL_ERROR "unknown flavour ${TARGET_PACKAGE_FLAVOUR}")
	endif(TARGET_PACKAGE_FLAVOUR STREQUAL "debian9")
else(TARGET_PACKAGE_FLAVOUR)
	set(CPACK_GENERATOR "TGZ")
endif(TARGET_PACKAGE_FLAVOUR)

if(CPACK_GENERATOR STREQUAL "DEB")
### .deb specificities
	# generate teh well-formatted description
	execute_process(
		COMMAND sh -c "fmt -w 78 | sed -e 's/^$/./' -e 's/^/ /'"
		INPUT_FILE "${webcom-sdk-c_SOURCE_DIR}/pkg/description.txt"
		OUTPUT_VARIABLE PACKAGE_DESCRIPTION)
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
"${CPACK_PACKAGE_DESCRIPTION_SUMMARY}
${PACKAGE_DESCRIPTION}")

	# add the ABI version in the package name
	set(CPACK_PACKAGE_NAME "${CPACK_PACKAGE_NAME}${PROJECT_VERSION_MAJOR}")
elseif(CPACK_GENERATOR STREQUAL "RPM")
### RPM specificities
	# run ldconfig after package installation
	set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${webcom-sdk-c_SOURCE_DIR}/pkg/postinst")
endif(CPACK_GENERATOR STREQUAL "DEB")

include(CPack)