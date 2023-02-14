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
 * connecting to, and interacting with the Webcom platform.
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
 * # Show me the code!
 *
 * OK.
 *
 * 	@code{c}
 * #include <webcom-c/webcom.h>
 * #include <webcom-c/webcom-libev.h>
 *
 * int main(int argc, char *argv[]) {
 * 	wc_context_t *ctx;
 *
 * 	// get a libev event loop
 * 	struct ev_loop *loop = EV_DEFAULT;
 *
 * 	// set a bunch of general-purpose datasync callbacks
 * 	struct wc_eli_callbacks cb = {
 * 			.on_connected = on_connected,
 * 			.on_disconnected = on_disconnected,
 * 			.on_error = on_error,
 * 	};
 *     // set the connection options
 * 	struct wc_context_options options = {
 * 			.host = "io.datasync.orange.com",
 * 			.port = 443,
 * 			.app_name = "my_namespace",
 * 	};
 *
 * 	// establish the connection to the webcom server, and let it integrate in
 * 	// our libev event loop
 * 	ctx = wc_context_create_with_libev(
 * 			&options,
 * 			loop,
 * 			&cb);
 *
 *     // we are going to use the datasync service
 * 	wc_datasync_init(ctx, loop);
 *
 * 	// ask the SDK to establish the connection to the server
 * 	wc_datasync_connect(ctx);
 *
 * 	// enter the event loop
 * 	ev_run(loop, 0);
 *
 * 	// destroy the context when the loop ends
 * 	wc_context_destroy(ctx);
 *
 * 	return 0;
 * }
 *
 * // called when the connection to the server is established
 * static void on_connected(wc_context_t *ctx) {
 * 	wc_datasync_on_child_added(ctx, "/foo/bar", on_foo_bar_added);
 * }
 *
 * // called whenever a new child node is appended to /foo/bar
 * int on_foo_bar_added(wc_context_t *ctx, on_handle_t handle, char *data, char *cur, char *prev) {
 * 	printf("a new child named [%s] with value [%s] was added on path /foo/bar\n", cur, data);
 * 	return 1;
 * }
 *
 * (...)
 * 	@endcode
 */

/**
 * @defgroup hilev 	Webcom SDK end-user API
 * @{
 * 	This section describes the easiest way to build an application using the webcom
 * 	data-sync service. First you pass a reference to a libev loop object to the
 * 	SDK to open a context, then you can use the "high level" functions to send
 * 	requests and get notified of data events.
 *
 * 	@defgroup webcom-base			Create a base context without libev (the hard way)
 * 	@defgroup webcom-libev			Create a base context with libev (the easy way)
 *	@defgroup webcom-log			Logging functions
 * 	@defgroup datasync				Interact with a Webcom datasync server
 * 	@{
 * 		@defgroup webcom-datasync-cnx	Control the websocket connection to the datasync server
 * 		@defgroup webcom-on				Access to the Webcom database and be notified of data changes
 * 		@defgroup webcom-requests		Send requests: send data, subscribe to data change, ...
 * 	@}
 * 	@defgroup auth					Interact with a Webcom auth service
 * 	@{
 * 		@defgroup webcom-auth			Retrieve authentication tokens from the Webcom server
 * 	@}
 *
 * @}
 * @defgroup lolev		Some Webcom SDK internals
 * @{
 * 	This section contains details about functions and data structures used by the above
 * 	interface. You should not need to use most of them directly.
 *
 * 	@defgroup webcom-messages		Webcom datasync protocol data types, messages manipulation
 * 	@defgroup webcom-parser 		Webcom datasync protocol parsing functions
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

#include "webcom-base.h"
#include "webcom-log.h"
#include "webcom-utils.h"

/* Datasync */
#include "webcom-msg.h"
#include "webcom-parser.h"
#include "webcom-datasync.h"
#include "webcom-req.h"
#include "webcom-on.h"

/* Authentication */
#include "webcom-auth.h"

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
int64_t wc_datasync_server_time(wc_context_t *cnx);

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
void wc_datasync_gen_push_id(wc_context_t *cnx, char *result);

/**
 * get the webcom SDK version string
 * @return a string representing the version
 */
const char* wc_version();

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_H_ */
