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

#include "webcom-eli.h"

#include <ev.h>

/**
 * @ingroup webcom-libev
 * @{
 * This SDK is designed using an asynchronous, event-based approach, and is
 * perfectly suited to be used in a mono-threaded application.
 *
 * To greatly relieve the burden of registering to, and handling all the I/O and
 * timer events the SDK must be aware of, and if you don't mind making your
 * application rely on **libev**, we've already taken care of everything.
 *
 * The key idea is to call the corresponding `wc_context_new_with_libev()`
 * function, and pass :
 *
 * - the reference to your event loop
 * - a couple of callbacks to be notified when the connection is
 * established/closed
 */

/**
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
 * @param loop a pointer to a libev loop object
 * @param callbacks a structure containing the callbacks to trigger for various
 * events
 * @return a pointer to the newly created connection on success, NULL on
 * failure to create the context
 */
wc_context_t *wc_context_create_with_libev(struct wc_context_options *options, struct ev_loop *loop, struct wc_eli_callbacks *callbacks);

/**
 * @}
 */

#endif /* WITH_LIBEV */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_LIBEV_H_ */
