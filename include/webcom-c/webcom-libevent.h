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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_LIBEVENT_H_
#define INCLUDE_WEBCOM_C_WEBCOM_LIBEVENT_H_


#include "webcom-config.h"

#ifdef WITH_LIBEVENT

#include "webcom-cnx.h"

#include <event.h>


/**
 * @ingroup webcom-libevent
 * @{
 */

struct wc_libevent_integration {
	struct event_base *loop;
	void (*on_connected)(wc_context_t *ctx);
	void (*on_disconnected)(wc_context_t *ctx);
};

/**
 * @}
 * @ingroup webcom-libevent
 * @{
 * Create a webcom context using libevent.
 *
 * This function creates a new Webcom context, an initiates the connection
 * towards the server. All the underlying I/O and timer events will be taken
 * care of by **libevent**.
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
 * @param eli a pointer to a struct wc_libevent_integration object
 * @return a pointer to the newly created connection on success, NULL on
 * failure to create the context
 */
wc_context_t *wc_context_new_with_libevent(char *host, uint16_t port, char *application, struct wc_libevent_integration *eli);

/**
 * @}
 */

#endif /* WITH_LIBEVENT */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_LIBEVENT_H_ */
