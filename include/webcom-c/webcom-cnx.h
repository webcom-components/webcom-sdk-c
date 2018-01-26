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
 * @addtogroup webcom-connection
 * @{
 */

#define WC_POLLHUP (POLLHUP|POLLERR)
#define WC_POLLIN (POLLIN)
#define WC_POLLOUT (POLLOUT)

/**
 * the origin of a poll event
 */
enum wc_pollsrc {
	WC_POLL_DATASYNC,           //!< the event relates to the datasync socket
	WC_POLL_AUTH,               //!< the event relates to the authentication socket

	_WC_POLL_MAX /* keep last *///!< special value, holds the actual number of possible socket event sources
};

/**
 * the origin of a timer event
 */
enum wc_timersrc {
	WC_TIMER_DATASYNC_KEEPALIVE, //!< the event relates to the datasync socket keepalive
	WC_TIMER_DATASYNC_RECONNECT, //!< the event relates to the datasync reconnection timer
	WC_TIMER_AUTH,               //!< the event relates to the authentication HTTP request handling

	_WC_TIMER_MAX /* keep last *///!< special value, holds the actual number of possible timers
};

/**
 * arguments to poll events
 */
struct wc_pollargs {
	int fd;              //!< file descriptor
	short events;        //!< a mask of events (WC_POLL* macros)
	enum wc_pollsrc src; //!< the origin of this event
};

/**
 * arguments to timer events
 */
struct wc_timerargs {
	long ms;                //!< timer delay in milliseconds
	enum wc_timersrc timer; //!< the origin of this event
	int repeat;             //!< 1 if this timer should be recurring, 0 otherwise (only relevant for the WC_EVENT_SET_TIMER event)
};

typedef enum {
	/**
	 * This event indicates that the connection passed as cnx to the callback
	 * was closed, either because the server has closed it, or because
	 * wc_cnx_close() was called.
	 */
	WC_EVENT_ON_CNX_CLOSED,
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
	/**
	 * This event indicates that a new file descriptor should be polled for
	 * some given events. `data` is a pointer to a `struct wc_pollargs` object
	 * that contains the details on the descriptor and events.
	 */
	WC_EVENT_ADD_FD,
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
	/**
	 * This event indicates that a reply to an authentication request is
	 * available. The details about the result is passed in data as a pointer
	 * to a `struct wc_auth_info`.
	 */
	WC_AUTH_ON_AUTH_REPLY,
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

/**
 * Create a webcom context.
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
wc_context_t *wc_context_new(char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user);

/**
 * Frees the resources used by a wc_context_t object.
 *
 * This function is to be called once a wc_context_t object becomes useless.
 *
 * @param ctx the wc_cnx_t object to free.
 */
void wc_context_free(wc_context_t *ctx);

/**
 * Sends a message to the webcom server.
 *
 * @param ctx the context
 * @param msg the webcom message to send
 * @return the number of bytes written, otherwise < 0 is returned in case of
 * failure.
 */
int wc_context_send_msg(wc_context_t *ctx, wc_msg_t *msg);

/**
 * Gracefully closes a connection to a webcom server.
 *
 * @note the user-defined wc_on_event_cb_t callback will be triggered with a
 * WC_EVENT_ON_CNX_CLOSED event once the connection has been closed
 *
 * @param ctx the context
 */
void wc_context_close_cnx(wc_context_t *ctx);

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
int wc_cnx_keepalive(wc_context_t *ctx);

/**
 * gets the user data associated to this context
 *
 * @return some user-defined data, set in wc_context_new()
 */
void *wc_context_get_user_data(wc_context_t *ctx);

/**
 * informs the SDK of some event happening on one of its file descriptors
 *
 * This function is to be called by the event loop logic to make the Webcom SDK
 * handle any pending event on one of its file descriptor.
 *
 * @note If you use one of the libev/libuv/libevent integration, you don't need
 * to call this function.
 *
 * @param ctx the context
 * @param pa a pointer to a structure describing the event (file descriptor,
 * events [WC_POLLHUP, WC_POLLIN, WC_POLLOUT], and source [WC_POLL_DATASYNC,
 * WC_POLL_AUTH])
 */
void wc_handle_fd_events(wc_context_t *ctx, struct wc_pollargs *pa);

/**
 * informs the SDK that some timer has fired
 *
 * You need to call this function once a timer armed because of a
 * **WC_EVENT_SET_TIMER** event has fired.
 * @param ctx the context
 * @param timer the timer identifier
 */
void wc_handle_timer(wc_context_t *ctx, enum wc_timersrc timer);

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
void wc_context_reconnect(wc_context_t *ctx);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_CNX_H_ */
