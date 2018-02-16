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
#include "webcom-c/webcom-log.h"

struct wc_libevent_integration_data {
	struct event_base *loop;
	struct wc_eli_callbacks callbacks;
	struct event con_watcher;
	struct event ka_timer;
	struct event recon_timer;
	unsigned next_try;
};

static inline void _wc_on_fd_event_libevent_cb (
		UNUSED_PARAM(evutil_socket_t fd),
		UNUSED_PARAM(short revents),
		void *data)
{

	wc_context_t *ctx = data;
	wc_dispatch_fd_event(ctx, fd,
			((revents&EV_READ) ? POLLIN : 0)
					| ((revents&EV_WRITE) ? POLLOUT : 0)
					);
}

static inline void _wc_on_ka_timer_libevent_cb (
		UNUSED_PARAM(evutil_socket_t fd),
		UNUSED_PARAM(short revents),
		void *data)
{
	wc_context_t *ctx = data;
	wc_datasync_keepalive(ctx);
}

static inline void _wc_on_recon_timer_libevent_cb (
		UNUSED_PARAM(evutil_socket_t fd),
		UNUSED_PARAM(short revents),
		void *data)
{
	wc_context_t *ctx = data;

	W_DBG(WC_LOG_CONNECTION, "reconnect callback triggered for context %p", ctx);
	wc_datasync_connect(ctx);
}

static void _wc_libevent_cb (wc_event_t event, wc_context_t *ctx, void *data, UNUSED_PARAM(size_t len)) {
	struct wc_libevent_integration_data *lid = wc_context_get_user_data(ctx);
	struct wc_pollargs *pollargs;
	struct timeval tv;
	int reconnect = 0;

	switch(event) {
	case WC_EVENT_ADD_FD:
		pollargs = data;
		event_assign(&lid->con_watcher, lid->loop, pollargs->fd,
				((pollargs->events & (WC_POLLIN | WC_POLLHUP)) ? EV_READ : 0)
					| ((pollargs->events & WC_POLLOUT) ? EV_WRITE : 0)
					| EV_PERSIST,
				_wc_on_fd_event_libevent_cb, ctx);
		event_add(&lid->con_watcher, NULL);
		break;

	case WC_EVENT_DEL_FD:
		event_del(&lid->con_watcher);
		break;

	case WC_EVENT_MODIFY_FD:
		pollargs = data;
		event_del(&lid->con_watcher);
		event_assign(&lid->con_watcher, lid->loop, pollargs->fd,
				((pollargs->events & (WC_POLLIN | WC_POLLHUP)) ? EV_READ : 0)
					| ((pollargs->events & WC_POLLOUT) ? EV_WRITE : 0)
					| EV_PERSIST,
				_wc_on_fd_event_libevent_cb, ctx);
		event_add(&lid->con_watcher, NULL);
		break;

	case WC_EVENT_ON_SERVER_HANDSHAKE:
		tv.tv_sec = 50;
		tv.tv_usec = 0;
		event_assign(&lid->ka_timer, lid->loop, -1, EV_PERSIST, _wc_on_ka_timer_libevent_cb, ctx);
		event_add(&lid->ka_timer, &tv);
		if (lid->next_try) {
			event_del(&lid->recon_timer);
			lid->next_try = 0;
		}
		lid->callbacks.on_connected(ctx);
		break;
	case WC_EVENT_ON_CNX_CLOSED:
		event_del(&lid->ka_timer);
		reconnect = lid->callbacks.on_disconnected(ctx);
		break;
	case WC_EVENT_ON_CNX_ERROR:
		reconnect = lid->callbacks.on_error(ctx, lid->next_try, data, len);
		break;
	default:
		break;
	}

	if (reconnect) {
		tv.tv_sec = lid->next_try;
		tv.tv_usec = 0;

		if (lid->next_try == 0) {
			lid->next_try = 1;
		} else if (lid->next_try < 128) {
			lid->next_try <<= 1;
		}

		event_assign(&lid->recon_timer, lid->loop, -1, 0 , _wc_on_recon_timer_libevent_cb, ctx);
		event_add(&lid->recon_timer, &tv);
		W_DBG(WC_LOG_CONNECTION, "automatic reconnection attempt in %ld sec", (long)tv.tv_sec);
	} else {
		if (lid->next_try != 0) {
			lid->next_try = 0;
		}
	}
}

wc_context_t *wc_context_new_with_libevent(char *host, uint16_t port, char *application, struct event_base *loop, struct wc_eli_callbacks *callbacks) {
	struct wc_libevent_integration_data *integration_data;
	wc_context_t *ret = NULL;

	integration_data = malloc(sizeof *integration_data);
	if (integration_data == NULL) {
		return NULL;
	}

	integration_data->callbacks = *callbacks;
	integration_data->loop = loop;
	integration_data->next_try = 0;

	ret = wc_datasync_init(host, port, application, _wc_libevent_cb, integration_data);

	if (ret == NULL) {
		free(integration_data);
	}

	return ret;
}
