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
#include <stdlib.h>

#include "webcom-c/webcom-libevent.h"
#include "webcom-c/webcom-utils.h"

struct wc_libevent_integration_data {
	struct wc_libevent_integration eli;
	struct event con_watcher;
	struct event ka_timer;
};

static inline void _wc_on_readable_libevent_cb (
		UNUSED_PARAM(evutil_socket_t fd),
		UNUSED_PARAM(short revents),
		void *data)
{
	wc_context_t *ctx = data;
	wc_cnx_on_readable(ctx);
}

static inline void _wc_on_ka_timer_libevent_cb (
		UNUSED_PARAM(evutil_socket_t fd),
		UNUSED_PARAM(short revents),
		void *data)
{
	wc_context_t *ctx = data;
	wc_cnx_keepalive(ctx);
}

static void _wc_libevent_cb (wc_event_t event, wc_context_t *ctx, void *data, UNUSED_PARAM(size_t len)) {
	struct wc_libevent_integration_data *lid = wc_context_get_user_data(ctx);
	struct wc_pollargs *pollargs;
	struct timeval tv;

	switch(event) {
	case WC_EVENT_ADD_FD:
		pollargs = data;
		event_assign(&lid->con_watcher, lid->eli.loop, pollargs->fd,
				(pollargs->events & (WC_POLLIN | WC_POLLHUP) ? EV_READ : 0)
					| (pollargs->events & WC_POLLOUT ? EV_WRITE : 0)
					| EV_PERSIST,
				_wc_on_readable_libevent_cb, ctx);
		event_add(&lid->con_watcher, NULL);
		break;

	case WC_EVENT_DEL_FD:
		event_del(&lid->con_watcher);
		break;

	case WC_EVENT_MODIFY_FD:
		pollargs = data;
		event_del(&lid->con_watcher);
		event_assign(&lid->con_watcher, lid->eli.loop, pollargs->fd,
				(pollargs->events & (WC_POLLIN | WC_POLLHUP) ? EV_READ : 0)
					| (pollargs->events & WC_POLLOUT ? EV_WRITE : 0)
					| EV_PERSIST,
				_wc_on_readable_libevent_cb, ctx);
		event_add(&lid->con_watcher, NULL);
		break;

	case WC_EVENT_ON_SERVER_HANDSHAKE:
		tv.tv_sec = 50;
		tv.tv_usec = 0;
		event_assign(&lid->ka_timer, lid->eli.loop, -1, EV_PERSIST, _wc_on_ka_timer_libevent_cb, ctx);
		event_add(&lid->ka_timer, &tv);
		lid->eli.on_connected(ctx, 1);
		break;

	case WC_EVENT_ON_CNX_CLOSED:
		event_del(&lid->ka_timer);
		lid->eli.on_disconnected(ctx);
		break;

	default:
		break;
	}
}

wc_context_t *wc_context_new_with_libevent(char *host, uint16_t port, char *application, struct wc_libevent_integration *eli){
	struct wc_libevent_integration_data *integration_data;
	wc_context_t *ret = NULL;

	integration_data = malloc(sizeof *integration_data);
	if (integration_data == NULL) {
		return NULL;
	}

	integration_data->eli = *eli;

	ret = wc_context_new(host, port, application, _wc_libevent_cb, integration_data);

	if (ret == NULL) {
		free(integration_data);
	}

	return ret;
}
