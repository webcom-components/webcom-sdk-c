/*
 * Webcom C SDK
 *
 * Copyright 2017 Orange
 * <camille.oudot@orange.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/**
 * @mainpage Webcom C SDK Index Page
 *
 * This SDK offers an easy access to the Orange Flexible Datasync service
 * to C/C++ programmer.
 */

/**
 * @defgroup hilev		Higher level functions
 * @{
 * 	@defgroup webcom-general		Webcom general functions
 * 	@defgroup webcom-connection 	Connect a client to a Webcom server
 * 	@defgroup webcom-requests		Send Webcom client to server request
 *	@defgroup webcom-event			Register callback on Webcom events
 * @}
 * @defgroup lolev		Lower level functions (you may not need them)
 * @{
 * 	@defgroup webcom-messages	Webcom protocol data types, messages manipulation
 * 	@defgroup webcom-parser 	Webcom protocol parsing functions
 * 	@defgroup webcom-utils 	Some utility macros, functions, ...
 * @}
 */

#ifndef INCLUDE_WEBCOM_C_WEBCOM_H_
#define INCLUDE_WEBCOM_C_WEBCOM_H_

#include "webcom-config.h"

#define WEBCOM_SDK_VERSION	((unsigned)((WEBCOM_SDK_MAJOR<<16)|(WEBCOM_SDK_MINOR<<8)|WEBCOM_SDK_PATCH))

#define WC_XSTR(s) WC_STR(s)
#define WC_STR(s) #s

#ifdef WEBCOM_SDK_EXTRA
#	define WEBCOM_SDK_VERSION_STR WC_XSTR(WEBCOM_SDK_MAJOR) "." WC_XSTR(WEBCOM_SDK_MINOR) "." WC_XSTR(WEBCOM_SDK_PATCH) "-" WEBCOM_SDK_EXTRA
#else
#	define WEBCOM_SDK_VERSION_STR WC_XSTR(WEBCOM_SDK_MAJOR) "." WC_XSTR(WEBCOM_SDK_MINOR) "." WC_XSTR(WEBCOM_SDK_PATCH)
#endif

#include "webcom-msg.h"
#include "webcom-parser.h"
#include "webcom-cnx.h"
#include "webcom-req.h"
#include "webcom-utils.h"
#include "webcom-event.h"

/**
 * @ingroup webcom-general
 * @{
 */

/**
 * gets the server time in milliseconds since 1970/1/1
 *
 * This function returns the (estimated) time on the server, in milliseconds.
 * It is achieved by the SDK by computing and memorizing the clock offset
 * between the local machine and the server when establishing the connection to
 * the Webcom server.
 *
 * @param cnx the webcom connection
 * @return the estimated server time in milliseconds since 1970/1/1
 */
int64_t wc_get_server_time(wc_cnx_t *cnx);

/**
 * builds a webcom push id
 *
 * This function will write a new push id in the buffer pointed by `result`.
 *
 * A push id is a 20-bytes string that guarantees several properties:
 * - all SDKs across all platform generate it using the exact same method
 * - when sorted in lexicographical order, they are also sorted by
 *   chronological order of their creation, regardless of what webcom client
 *   node has generated it (the SDKs does so by computing a local to server
 *   clock offset during the webcom protocolar handshake)
 *
 * **Example:**
 *
 *			foo(wc_cnx_t *cnx) {
 *				char buf[20];
 *				...
 *				wc_get_push_id(cnx, buf);
 *				printf("the id is: %20s\n", buf);
 *				...
 *			}
 *
 * @note the result buffer **will not** be nul-terminated
 *
 * @param cnx the webcom connection (it **MUST** have received the handshake
 * from the server, and it may be currently connected or disconnected)
 * @param result the address of a (minimum) 20-bytes buffer that will receive
 * the newly created push id
 */
void wc_get_push_id(wc_cnx_t *cnx, char *result);

/**
 * get the webcom SDK version string
 * @return a string representing the version
 */
const char* wc_version();

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_H_ */
