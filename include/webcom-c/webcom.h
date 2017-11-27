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
 * @mainpage Webcom C SDK Documentation Page
 *
 * This is the documentation for the Webcom C SDK, a SDK that allows
 * connecting to, and interacting with a Webcom server.
 *
 * # What is Webcom?
 *
 * Webcom is a Backend As A Service platform offering
 *
 * - a data sync service,
 * - an authentication service.
 *
 * Orange operates a Webcom-based service named [Orange Flexible Datasync]
 * (https://io.datasync.orange.com). Create an account on
 * https://io.datasync.orange.com/u/signup to start using webcom.
 *
 * # Get the C SDK
 *
 * The SDK is published on GitHub: https://github.com/webcom-components/webcom-sdk-c
 *
 * You'll find informations on the supported platforms, building or getting
 * binary versions in the README.md document.
 *
 * # Use the SDK
 *
 * Browse the [modules](modules.html) page to get informations on the API.
 *
 * - the straightforward: @ref hilev
 * - the internals: @ref lolev
 *
 * # Show me the code!
 *
 * OK.
 *
 * 	@code{c}
 * // the generic include
 * #include <webcom-c/webcom.h>
 *
 * // must be included explicitly if you plan to use an event lib integration
 * #include <webcom-c/webcom-libuv.h>
 *
 * int main(void) {
 * 	wc_context_t *ctx;
 * 	uv_loop_t loop;
 *
 *	// get a new libuv loop instance
 * 	uv_loop_init(&loop);
 *
 *	// set the connected/disconnected callbacks
 * 	struct wc_eli_callbacks cb = {
 * 		.on_connected = on_connected,
 * 		.on_disconnected = on_disconnected,
 * 		.on_error = on_error,
 * 	};
 *
 *	// open a webcom context using libuv
 * 	ctx = wc_context_new_with_libuv(
 * 		"io.datasync.orange.com",
 * 		443,
 * 		"myapp",
 * 		&eli);
 *
 *	// finally, run the loop
 * 	uv_run(&loop, UV_RUN_DEFAULT);
 * 	return 0;
 * }
 *
 * static void on_connected(wc_context_t *ctx) {
 *	// register some data routes
 * 	wc_on_data(ctx, "/foo/bar", my_bar_callback, NULL);
 * 	wc_on_data(ctx, "/foo/baz", my_baz_callback, NULL);
 *
 *	// subscribe to the "/foo" path
 * 	wc_req_listen(ctx, NULL, "/foo");
 * }
 *
 * static int on_disconnected(wc_context_t *ctx) {
 * 	return 0; // = don't try to reconnect
 * }
 * static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len) {
 * 	return 1; // = try to reconnect
 * }
 *
 * void my_bar_callback(...) { ... }
 * void my_baz_callback(...) { ... }
 * 	@endcode
 */

/**
 * @defgroup hilev 	Easy interface
 * @{
 * 	This section describes the easiest way to build an application using the webcom
 * 	data-sync service. First you pass a reference to a event lib loop object to the
 * 	SDK to open a context, then you can use the "high level" functions to send
 * 	requests and get notified of data events.
 *
 * 	@defgroup webcom-evloop			Connect with an existing event loop
 * 	@{
 * 		This SDK is designed using an asynchronous, event-based approach, and is
 * 		perfectly suited to be used in a mono-threaded application.
 *
 * 		To greatly relieve the burden of registering to, and handling all the I/O and
 * 		timer events the SDK must be aware of, and if you don't mind making your
 * 		application rely on either **libev**, **libuv**, or **libevent**, we've already
 * 		taken care of everything.
 *
 * 		The key idea is to call the corresponding `wc_context_new_with_libXXX()`
 * 		function, and pass :
 *
 * 		- the reference to your event loop
 * 		- a couple of callbacks to be notified when the connection is
 * 		established/closed
 *
 * 		@defgroup webcom-libev			Integrate with libev
 * 		@defgroup webcom-libuv			Integrate with libuv
 * 		@defgroup webcom-libevent		Integrate with libevent
 * 	@}
 * 	@defgroup webcom-requests		Send requests: subscribe to data change, send data, ...
 *	@defgroup webcom-event			Route data events to callbacks
 *	@defgroup webcom-log			Logging functions
 *
 * @}
 * @defgroup lolev		Lower level interface
 * @{
 * 	This section contains details about functions and data structures used by the "Easy"
 * 	interface. You mays not use most of them directly.
 *
 * 	@defgroup webcom-connection 	Lower-level context and connection management functions
 * 	@defgroup webcom-messages		Webcom protocol data types, messages manipulation
 * 	@defgroup webcom-parser 		Webcom protocol parsing functions
 * 	@defgroup webcom-general		General Webcom SDK functions
 * 	@defgroup webcom-utils 			Some utility macros, functions, ...
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

#include "webcom-log.h"
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
int64_t wc_get_server_time(wc_context_t *cnx);

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
void wc_get_push_id(wc_context_t *cnx, char *result);

/**
 * get the webcom SDK version string
 * @return a string representing the version
 */
const char* wc_version();

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_H_ */
