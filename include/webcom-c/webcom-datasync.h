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

#include "webcom-msg.h"
#include "webcom-base.h"

/**
 * @ingroup webcom-datasync-cnx
 * @{
 */

/**
 * initializes the datasync service for a Webcom context
 *
 * This function must be have been called once before performing any datasync
 * related operation. Note that wc_datasync_connect() will call this function
 * before trying to connect to the server, if it had not been called already.
 *
 * @param ctx the Webcom context
 * @return a pointer to an opaque structure representing the datasync service
 */
wc_datasync_context_t *wc_datasync_init(wc_context_t *ctx, void *foreign_loop);

/**
 * Function that keeps the connection to the webcom server alive.
 *
 * The server will singlehandledly close the connection if there is no activity
 * from the client in the last 60 seconds. Call this function periodically to
 * avoid this.
 *
 * @note If you use one of the libev integration, you don't need to call this
 * function.
 *
 * @param ctx the context
 */
int wc_datasync_keepalive(wc_context_t *ctx);

/**
 * Connects or reconnects a disconnected Webcom context
 *
 * This function tries to (re)establish a connection to a Webcom server for a
 * context whose connection is disconnected. This is the case when the
 * following Webcom events have been dispatched to the user callback:
 *
 * - **WC_EVENT_ON_CNX_CLOSED**
 * - **WC_EVENT_ON_CNX_ERROR**
 *
 * @param ctx the context
 */
void wc_datasync_connect(wc_context_t *ctx, void *foreign_loop);

/**
 * Gracefully closes the connection to the Webcom datasync server.
 *
 * @note the user-defined wc_on_event_cb_t callback will be triggered with a
 * WC_EVENT_ON_CNX_CLOSED event once the connection has been closed. The higher
 * level callback wc_eli_callbacks::on_disconnected will be called if the
 * context is managed by libev.
 *
 * @param ctx the context
 */
void wc_datasync_close_cnx(wc_context_t *ctx);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_DATASYNC_H_ */
