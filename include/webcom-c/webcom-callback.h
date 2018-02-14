/*
 * webcom-sdk-c
 *
 * Copyright 2018 Orange
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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_CALLBACK_H_
#define INCLUDE_WEBCOM_C_WEBCOM_CALLBACK_H_

#define WC_BASE_EVENT_OFFSET		0x00000001
#define WC_DATASYNC_EVENT_OFFSET	0x00000100
#define WC_AUTH_EVENT_OFFSET		0x00010000

#include "webcom-base.h"

typedef enum {
	/* Base */

	/**
	 * This event indicates that a new file descriptor should be polled for
	 * some given events. `data` is a pointer to a `struct wc_pollargs` object
	 * that contains the details on the descriptor and events.
	 */
	WC_EVENT_ADD_FD = WC_BASE_EVENT_OFFSET,
	/**
	 * This event indicates that a file descriptor should be removed from the
	 * poll set. `data` is a pointer to a `struct wc_pollargs` object that
	 * contains the details on the descriptor.
	 */
	WC_EVENT_DEL_FD,
	/**
	 * This event indicates that the events to poll for an already polled file
	 * file descriptor must be changed. `data` is a pointer to a
	 * `struct wc_pollargs` object that contains the details on the descriptor
	 * and events.
	 */
	WC_EVENT_MODIFY_FD,
	/**
	 * This event instructs that a timer whose properties are passed in data as
	 * a pointer to a `struct wc_timerargs` object must be set and armed. When
	 * the timer fires, you must call the wc_handle_timer() function, with the
	 * appropriate wc_timersrc argument set.
	 */
	WC_EVENT_SET_TIMER,
	/**
	 * This event instructs that the timer whose identity is passed in data as
	 * a pointer to a `enum wc_timersrc` must be cancelled.
	 */
	WC_EVENT_DEL_TIMER,

	/* Datasync */

	/**
	 * This event indicates that the connection passed as cnx to the callback
	 * was closed, either because the server has closed it, or because
	 * wc_cnx_close() was called.
	 */
	WC_EVENT_ON_CNX_CLOSED = WC_DATASYNC_EVENT_OFFSET,
	/**
	 * This event indicates that the webcom server has sent its handshake
	 * message. From this point, it's safe to start sending requests to the
	 * webcom server through this connection.
	 *
	 * Return 1 to trigger automatic reconnection.
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
	 *
	 * Return 1 to trigger automatic reconnection.
	 */
	WC_EVENT_ON_CNX_ERROR,

	/* Authentication */

	/**
	 * This event indicates that a reply to an authentication request is
	 * available. The details about the result is passed in data as a pointer
	 * to a `struct wc_auth_info`.
	 */
	WC_AUTH_ON_AUTH_REPLY = WC_AUTH_EVENT_OFFSET,
} wc_event_t;

/**
 * This defines the callback type that must be passed to wc_cnx_new(). It will
 * be called for the events listed in the wc_event_t enum. When triggered, the
 * callback will receive these parameters:
 *
 * @param event (mandatory) the event type that was triggered
 * @param ctx (mandatory) the context on which the event occurred
 * @param data (optional) some additional data about the event
 * @param len (optional) the size of the additional data
 * @return a value to return to the C SDK
 */
typedef int (*wc_on_event_cb_t) (wc_event_t event, wc_context_t *ctx, void *data, size_t len);

#endif /* INCLUDE_WEBCOM_C_WEBCOM_CALLBACK_H_ */
