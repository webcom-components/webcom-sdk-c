/*
 * webcom-sdk-c
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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_LIBEV_H_
#define INCLUDE_WEBCOM_C_WEBCOM_LIBEV_H_

#include "webcom-config.h"

#ifdef WITH_LIBEV

#include "webcom-cnx.h"

#include <ev.h>

/**
 * @ingroup webcom-libev
 * @{
 */

struct wc_libev_integration {
	/** a reference to the libev event loop */
	struct ev_loop *loop;

	/** callback when the connection to the server is up */
	void (*on_connected)(wc_context_t *ctx);

	/** callback when the connection to the server is down */
	void (*on_disconnected)(wc_context_t *ctx);
};

/**
 * @}
 * @ingroup webcom-libev
 * @{
 * Create a webcom context using libev.
 *
 * This function creates a new Webcom context, an initiates the connection
 * towards the server. All the underlying I/O and timer events will be taken
 * care of by **libev**.
 *
 * If the **http_proxy** environment variable is set, the connection will be
 * established through this HTTP proxy.
 *
 * @note this function is partly synchronous and returns once the DNS lookup
 * and TCP handshake have succeeded or failed.
 *
 * @param host the webcom server host name
 * @param port the webcom server port
 * @param application the name of the webcom application to tie to (e.g.
 * "legorange", "chat", ...)
 * @param eli a pointer to a struct wc_libev_integration object
 * @return a pointer to the newly created connection on success, NULL on
 * failure to create the context
 */
wc_context_t *wc_context_new_with_libev(char *host, uint16_t port, char *application, struct wc_libev_integration *eli);

/**
 * @}
 */

#endif /* WITH_LIBEV */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_LIBEV_H_ */
