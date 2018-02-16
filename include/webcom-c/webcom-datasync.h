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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_DATASYNC_H_
#define INCLUDE_WEBCOM_C_WEBCOM_DATASYNC_H_

#include <stddef.h>
#include <poll.h>

#include "webcom-msg.h"
#include "webcom-base.h"
#include "webcom-callback.h"

/**
 * Create a webcom context.
 *
 * TODO update help
 *
 * This function creates a new Webcom context, an initiates the connection
 * towards the server.
 * If the **http_proxy** environment variable is set, the connection will be
 * established through this HTTP proxy.
 *
 * @note this function is partly synchronous and returns once the DNS lookup
 * and TCP handshake have succeeded or failed. The user `callback` with event
 * WC_EVENT_ON_SERVER_HANDSHAKE or WC_EVENT_ON_CNX_ERROR will the be called
 * asynchronously to indicate if the connection  to the Webcom service
 * succeeded or not.
 *
 * @param host the webcom server host name
 * @param port the webcom server port
 * @param application the name of the webcom application to tie to (e.g.
 * "legorange", "chat", ...)
 * @param callback the user-defined callback that will be called by the SDK
 * when one of the events listed in the wc_event_t enum occurs
 * @param user some optional user-data that will be passed to the callback for
 * every event occurring on this connection
 * @return a pointer to the newly created connection on success, NULL on
 * failure to create the context
 */
wc_datasync_context_t *wc_datasync_init(wc_context_t *ctx);

/**
 * Frees the resources used by a wc_context_t object.
 *
 * This function is to be called once a wc_context_t object becomes useless.
 *
 * @param ctx the wc_cnx_t object to free.
 */
void wc_datasync_context_cleanup(struct wc_datasync_context *ds_ctx);

/**
 * Sends a message to the webcom server.
 *
 * @param ctx the context
 * @param msg the webcom message to send
 * @return the number of bytes written, otherwise < 0 is returned in case of
 * failure.
 */
int wc_datasync_send_msg(wc_context_t *ctx, wc_msg_t *msg);

/**
 * Gracefully closes a connection to a webcom server.
 *
 * @note the user-defined wc_on_event_cb_t callback will be triggered with a
 * WC_EVENT_ON_CNX_CLOSED event once the connection has been closed
 *
 * @param ctx the context
 */
void wc_datasync_close_cnx(wc_context_t *ctx);

/**
 * Function that keeps the connection to the webcom server alive.
 *
 * The server will singlehandledly close the connection if there is no activity
 * from the client in the last 60 seconds. Call this function periodically to
 * avoid this.
 *
 * @note If you use one of the libev/libuv/libevent integration, you don't need
 * to call this function.
 *
 * @param ctx the context
 */
int wc_datasync_keepalive(wc_context_t *ctx);

/**
 * Reconnects a disconnected Webcom context
 *
 * This function tries to re-establish a connection to a Webcom server for a
 * context whose connection is disconnected. This is the case when the
 * following Webcom events have been dispatched to the user callback:
 *
 * - **WC_EVENT_ON_CNX_CLOSED**
 * - **WC_EVENT_ON_CNX_ERROR**
 *
 * @param ctx the context
 */
void wc_datasync_connect(wc_context_t *ctx);

/**
 * TODO
 * @param ctx
 * @param pa
 */
void wc_datasync_service_socket(wc_context_t *ctx, struct wc_pollargs *pa);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_DATASYNC_H_ */
