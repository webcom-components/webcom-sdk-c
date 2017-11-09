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
#include <poll.h>

#include "webcom-c/webcom-libev.h"
#include "webcom-c/webcom-utils.h"

struct wc_libev_integration_data {
	struct wc_libev_integration eli;
	struct ev_io con_watcher;
	struct ev_timer ka_timer;
};

static inline void _wc_on_fd_event_libev_cb (
		UNUSED_PARAM(struct ev_loop *loop),
		ev_io *w,
		UNUSED_PARAM(int revents))
{
	wc_context_t *ctx = w->data;
	wc_handle_fd_events(ctx, w->fd, ((revents&EV_READ) ? POLLIN : 0)
			| ((revents&EV_WRITE) ? POLLOUT : 0)
			);
}

static inline void _wc_on_ka_timer_libev_cb (
		UNUSED_PARAM(struct ev_loop *loop),
		ev_timer *w,
		UNUSED_PARAM(int revents))
{
	wc_context_t *ctx = w->data;
	wc_cnx_keepalive(ctx);
}

static void _wc_libev_cb (wc_event_t event, wc_context_t *ctx, void *data, UNUSED_PARAM(size_t len)) {
	struct wc_libev_integration_data *lid = wc_context_get_user_data(ctx);
	struct wc_pollargs *pollargs;

	switch(event) {
	case WC_EVENT_ADD_FD:
		pollargs = data;
		ev_io_init(&lid->con_watcher, _wc_on_fd_event_libev_cb, pollargs->fd,
				((pollargs->events & (WC_POLLIN | WC_POLLHUP)) ? EV_READ : 0)
					| ((pollargs->events & WC_POLLOUT) ? EV_WRITE : 0));
		lid->con_watcher.data = ctx;
		ev_io_start(lid->eli.loop, &lid->con_watcher);
		break;

	case WC_EVENT_DEL_FD:
		ev_io_stop(lid->eli.loop, &lid->con_watcher);
		break;

	case WC_EVENT_MODIFY_FD:
		pollargs = data;
		ev_io_stop(lid->eli.loop, &lid->con_watcher);
		ev_io_init(&lid->con_watcher, _wc_on_fd_event_libev_cb, pollargs->fd,
				((pollargs->events & (WC_POLLIN | WC_POLLHUP)) ? EV_READ : 0)
					| ((pollargs->events & WC_POLLOUT) ? EV_WRITE : 0));
		lid->con_watcher.data = ctx;
		ev_io_start(lid->eli.loop, &lid->con_watcher);
		break;

	case WC_EVENT_ON_SERVER_HANDSHAKE:
		ev_timer_init(&lid->ka_timer, _wc_on_ka_timer_libev_cb, 50, 50);
		lid->ka_timer.data = ctx;
		ev_timer_start(lid->eli.loop, &lid->ka_timer);

		lid->eli.on_connected(ctx);
		break;

	case WC_EVENT_ON_CNX_CLOSED:
		ev_timer_stop(lid->eli.loop, &lid->ka_timer);
		lid->eli.on_disconnected(ctx);
		break;

	default:
		break;
	}

}

wc_context_t *wc_context_new_with_libev(char *host, uint16_t port, char *application, struct wc_libev_integration *eli){
	struct wc_libev_integration_data *integration_data;
	wc_context_t *ret = NULL;

	integration_data = malloc(sizeof *integration_data);
	if (integration_data == NULL) {
		return NULL;
	}

	integration_data->eli = *eli;

	ret = wc_context_new(host, port, application, _wc_libev_cb, integration_data);

	if (ret == NULL) {
		free(integration_data);
	}

	return ret;
}
