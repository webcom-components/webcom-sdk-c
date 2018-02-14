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

#include "webcom-callback.h"
#include "webcom-auth.h"

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

/**
 * @}
 */

struct wc_context_options {
	char *app_name;
	char *host;
	uint16_t port;
	wc_on_event_cb_t callback;
	int no_tls;
	void *user_data;
};

wc_context_t *wc_context_create(struct wc_context_options *options);

wc_datasync_context_t *wc_get_datasync(wc_context_t *);
wc_auth_context_t *wc_get_auth(wc_context_t *);

#endif /* INCLUDE_WEBCOM_C_WEBCOM_BASE_H_ */
