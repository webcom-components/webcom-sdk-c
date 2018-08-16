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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_BASE_H_
#define INCLUDE_WEBCOM_C_WEBCOM_BASE_H_


typedef struct wc_context wc_context_t;
typedef struct wc_datasync_context wc_datasync_context_t;
typedef struct wc_auth_context wc_auth_context_t;

#include <poll.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @addtogroup webcom-base
 * @{
 * This SDK is fundamentally asynchronous, and is meant to be easily used in a
 * mono-threaded application. The logic to achieve this is quite complex, and
 * if you don't mind to depend on libev, there is no need to manually deal with
 * any of the operations and concepts described below, go directly to the @link
 * webcom-libev "connect with libev" @endlink section.
 *
 * If you don't plan to rely on libev, here are the steps and logic to implement :
 *
 * ## Populate a `wc_context_options` structure :
 *
 * The `wc_context_options::callback` field must contain a pointer to a function
 * you have defined (of type `wc_on_event_cb_t`) in your code, and that will
 * be triggered by the SDK when various events happen (see `wc_event_t`).
 *
 * ## Keep track of the requested file descriptors and timers
 *
 * Whenever the SDK needs a file descriptor to be monitored for certain events,
 * or a timer to be set, it will trigger the callback with the appropriate event
 * and informations. For example, if the datasync part of the SDK needs the file
 * descriptor 42 to be monitored for read and write availability, the callback
 * will be called with the **WC_EVENT_ADD_FD** event, and the **data** parameter
 * will point to a `wc_pollargs` structure containing the following informations:
 *
 * @code{c}
 * {
 * 	.fd = 42,
 * 	.events = WC_POLLIN | WC_POLLOUT,
 * 	.src = WC_POLL_DATASYNC,
 * }
 * @endcode
 *
 * To properly keep track of these events, an event-loop based on a `poll`-like
 * mechanism needs to be implemented. This can be achieved by manually using
 * `poll`, `epoll`, `kqueue`, ...
 *
 * ## Inform the SDK whenever an event (I/O, timer) occurs
 *
 * When a timer expires or a requested operation is available on a file
 * descriptor, the appropriate methods must be called right away:
 * - wc_dispatch_timer_event() if a timer expired
 * - wc_dispatch_fd_event() if an I/O event occurred
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

#define WC_BASE_EVENT_OFFSET		0x00000001
#define WC_DATASYNC_EVENT_OFFSET	0x00000100
#define WC_AUTH_EVENT_OFFSET		0x00010000

typedef enum {
	/* Base */

	/**
	 * This event indicates that a new file descriptor should be polled for
	 * some given events. `data` is a pointer to a `struct wc_pollargs` object
	 * that contains the details on the descriptor and events.
	 */
	WC_EVENT_ADD_FD = WC_BASE_EVENT_OFFSET,           //!< WC_EVENT_ADD_FD
	/**
	 * This event indicates that a file descriptor should be removed from the
	 * poll set. `data` is a pointer to a `struct wc_pollargs` object that
	 * contains the details on the descriptor.
	 */
	WC_EVENT_DEL_FD,                                  //!< WC_EVENT_DEL_FD
	/**
	 * This event indicates that the events to poll for an already polled file
	 * file descriptor must be changed. `data` is a pointer to a
	 * `struct wc_pollargs` object that contains the details on the descriptor
	 * and events.
	 */
	WC_EVENT_MODIFY_FD,                               //!< WC_EVENT_MODIFY_FD
	/**
	 * This event instructs that a timer whose properties are passed in data as
	 * a pointer to a `struct wc_timerargs` object must be set and armed. When
	 * the timer fires, you must call the wc_handle_timer() function, with the
	 * appropriate wc_timersrc argument set.
	 */
	WC_EVENT_SET_TIMER,                               //!< WC_EVENT_SET_TIMER
	/**
	 * This event instructs that the timer whose identity is passed in data as
	 * a pointer to a `enum wc_timersrc` must be cancelled.
	 */
	WC_EVENT_DEL_TIMER,                               //!< WC_EVENT_DEL_TIMER

	/* Datasync */

	/**
	 * This event indicates that the connection passed as cnx to the callback
	 * was closed, either because the server has closed it, or because
	 * wc_cnx_close() was called.
	 */
	WC_EVENT_ON_CNX_CLOSED = WC_DATASYNC_EVENT_OFFSET,//!< WC_EVENT_ON_CNX_CLOSED
	/**
	 * This event indicates that the webcom server has sent its handshake
	 * message. From this point, it's safe to start sending requests to the
	 * webcom server through this connection.
	 *
	 * Return 1 to trigger automatic reconnection.
	 */
	WC_EVENT_ON_SERVER_HANDSHAKE,                     //!< WC_EVENT_ON_SERVER_HANDSHAKE
	/**
	 * This event indicates that a valid webcom message was received on the
	 * given connection. The parsed message pointer (wc_msg_t *) is passed as
	 * (void *)data to the callback.
	 */
	WC_EVENT_ON_MSG_RECEIVED,                         //!< WC_EVENT_ON_MSG_RECEIVED
	/**
	 * This event indicates that an error occurred on the given connection. An
	 * additional error string of size len is given in data.
	 *
	 * Return 1 to trigger automatic reconnection.
	 */
	WC_EVENT_ON_CNX_ERROR,                            //!< WC_EVENT_ON_CNX_ERROR

	/* Authentication */

	/**
	 * This event indicates that a reply to an authentication request is
	 * available. The details about the result is passed in data as a pointer
	 * to a `struct wc_auth_info`.
	 */
	WC_AUTH_ON_AUTH_REPLY = WC_AUTH_EVENT_OFFSET,     //!< WC_AUTH_ON_AUTH_REPLY
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

struct wc_context_options {
	char *app_name;
	char *host;
	uint16_t port;
	wc_on_event_cb_t callback;
	int no_tls;
	void *user_data;
};

/**
 * informs the SDK of some event happening on one of its file descriptors
 *
 * This function is to be called by the event loop logic to make the Webcom SDK
 * handle any pending event on one of its file descriptor.
 *
 * @param ctx the context
 * @param pa a pointer to a structure describing the event (file descriptor,
 * events [WC_POLLHUP, WC_POLLIN, WC_POLLOUT], and source [WC_POLL_DATASYNC,
 * WC_POLL_AUTH])
 */
void wc_dispatch_fd_event(wc_context_t *ctx, struct wc_pollargs *pa);

/**
 * informs the SDK that some timer has fired
 *
 * You need to call this function once a timer armed because of a
 * **WC_EVENT_SET_TIMER** event has fired.
 * @param ctx the context
 * @param timer the timer identifier
 */
void wc_dispatch_timer_event(wc_context_t *ctx, enum wc_timersrc timer);

/**
 * gets the user data associated to this context
 *
 * @param ctx the context
 *
 * @return some user-defined data, set in wc_context_new()
 */
void *wc_context_get_user_data(wc_context_t *ctx);


/**
 * creates a Webcom context
 *
 * This function creates and initializes a new Webcom context using the
 * given options.
 * @param options a pointer to a wc_context_options structure with each
 * field set to the desired value
 * @return a pointer to the context (it's an opaque structure)
 */
wc_context_t *wc_context_create(struct wc_context_options *options);

/**
 * destoys a Webcom context
 *
 * This function destroys a wc_context_t object that was previously
 * allocated by the wc_context_create() function.
 *
 * @param ctx the context to destroy
 */
void wc_context_destroy(wc_context_t * ctx);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_BASE_H_ */
