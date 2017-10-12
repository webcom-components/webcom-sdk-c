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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_CNX_H_
#define INCLUDE_WEBCOM_C_WEBCOM_CNX_H_

#include <stddef.h>
#include <poll.h>

#include "webcom-msg.h"

typedef struct wc_context wc_context_t;

/**
 * @ingroup webcom-connection
 * @{
 */

#define WC_POLLHUP (POLLHUP|POLLERR)
#define WC_POLLIN (POLLIN)
#define WC_POLLOUT (POLLOUT)

struct wc_pollargs {
	int fd;
	int events;
	void *event_struct;
};

typedef enum {
	/**
	 * This event indicates that the connection passed as cnx to the callback
	 * was closed, either because the server has closed it, or because
	 * wc_cnx_close() was called.
	 */
	WC_EVENT_ON_CNX_CLOSED,
	/**
	 * This event indicates that the websocket connection to the webcom server,
	 * passed as cnx to the callback, was successfully established.
	 *
	 * Note: this does not mean the server handshake was received yet.
	 */
	WC_EVENT_ON_CNX_ESTABLISHED,
	/**
	 * This event indicates that the webcom server has sent its handshake
	 * message. From this point, it's safe to start sending requests to the
	 * webcom server through this connection.
	 */
	WC_EVENT_ON_SERVER_HANDSHAKE,
	/**
	 * This event indicates that a valid webcom message was received on the
	 * given connection. The parsed message pointer (wc_msg_t *) is passed as
	 * (void *)data to the callback.
	 */
	WC_EVENT_ON_MSG_RECEIVED,
	/**
	 * This event indicates that an error occurred on the given connection. An
	 * additional error string of size len is given in data.
	 */
	WC_EVENT_ON_CNX_ERROR,
	/**
	 *
	 */
	WC_EVENT_ADD_FD,
	WC_EVENT_DEL_FD,
	WC_EVENT_MODIFY_FD,
} wc_event_t;

/**
 * This defines the callback type that must be passed to wc_cnx_new(). It will
 * be called for the events listed in the wc_event_t enum. When triggered, the
 * callback will receive these parameters:
 *
 * @param event (mandatory) the event type that was triggered
 * @param ctx (mandatory) the connection on which the event occurred
 * @param data (optional) some additional data about the event
 * @param len (optional) the size of the additional data
 */
typedef void (*wc_on_event_cb_t) (wc_event_t event, wc_context_t *ctx, void *data, size_t len);

/**
 * Establishes a direct connection to a webcom server.
 *
 * This function establishes a new direct connection to a webcom server host.
 * If the **http_proxy** environment variable is set, the connection will be
 * established through this HTTP proxy.
 *
 * Note: this function is synchronous, it will return once the connection has
 * been established or once it failed to be established.
 *
 * @param host the webcom server host name
 *
 * @param port the webcom server port
 * @param application the name of the webcom application to tie to (e.g.
 * "legorange", "chat", ...)
 * @param callback the user-defined callback that will be called by the SDK
 * when one of the events listed in the wc_event_t enum occurs
 * @param user some optional user-data that will be passed to the callback for
 * every event occurring on this connection
 * @return a pointer to the newly created connection on success, NULL on
 * failure
 */
wc_context_t *wc_context_new(char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user);

/**
 * Frees the resources used by a wc_cnx_t object.
 *
 * This function is to be called once a wc_cnx_t object becomes useless.
 *
 * @param ctx the wc_cnx_t object to free.
 */
void wc_context_free(wc_context_t *ctx);

/**
 * Sends a message to the webcom server.
 *
 * @param cnx the connection to the server
 * @param msg the webcom message to send
 * @return the number of bytes written, otherwise < 0 is returned in case of
 * failure.
 */
int wc_context_send_msg(wc_context_t *cnx, wc_msg_t *msg);

/**
 * TODO
 * This is useful to integrate the SDK in some pre-existing or user-defined
 * event-loop (epoll, poll, ..., or abstractions such as libevent, libev, ...).
 *
 * @param ctx the connection
 * @return the file descriptor
 */
void *wc_context_get_event(wc_context_t *ctx);

/**
 * This function allows the webcom SDK to process the incoming data on a webcom
 * connection.
 *
 * This function must be called either frequently and periodically (not
 * ideal), or systematically when there is data to read on the file descriptor
 * obtained with wc_cnx_get_fd().
 *
 * If there is a webcom-level event related to the read and processed data,
 * calling this function will automatically trigger call(s) to the user-defined
 * wc_on_event_cb_t callback.
 *
 * @param ctx the connection whose incoming data should be processed
 */
void wc_cnx_on_readable(wc_context_t *ctx);

/**
 * Gracefully closes a connection to a webcom server.
 *
 * Note: the user-defined wc_on_event_cb_t callback will be triggered with a
 * WC_EVENT_ON_CNX_CLOSED event.
 *
 * @param ctx tke connection
 */
void wc_context_close_cnx(wc_context_t *ctx);

/**
 * Function that keeps the connection to the webcom server alive.
 *
 * The server will singlehandledly close the connection if there is no activity
 * from the client in the last 60 seconds. Call this function periodically to
 * avoid this.
 *
 * @param ctx the connection
 */
int wc_cnx_keepalive(wc_context_t *ctx);

/**
 * gets the user data associated to this context
 *
 * @return some user-defined data, set in wc_context_new()
 */
void *wc_context_get_user_data(wc_context_t *ctx);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_CNX_H_ */
