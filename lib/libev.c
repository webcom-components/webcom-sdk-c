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
#include <string.h>

#include "webcom-c/webcom-libev.h"
#include "webcom-c/webcom-utils.h"
#include "webcom-c/webcom-log.h"

struct wc_libev_integration_data {
	struct ev_loop *loop;
	struct wc_eli_callbacks callbacks;
	struct ev_io con_watcher;
	struct ev_timer ka_timer;
	struct ev_timer recon_timer;
	unsigned next_try;
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

static inline void _wc_on_recon_timer_libev_cb (
		struct ev_loop *loop,
		ev_timer *w,
		UNUSED_PARAM(int revents))
{
	wc_context_t *ctx = w->data;

	W_DBG(WC_LOG_CONNECTION, "reconnect callback triggered for context %p", ctx);
	ev_timer_stop(loop, w);
	wc_context_reconnect(ctx);

}

static void _wc_libev_cb (wc_event_t event, wc_context_t *ctx, void *data, size_t len) {
	struct wc_libev_integration_data *lid = wc_context_get_user_data(ctx);
	struct wc_pollargs *pollargs;
	int reconnect = 0;

	switch(event) {
	case WC_EVENT_ADD_FD:
		pollargs = data;
/* ignoring some GCC warnings issued for libev, see
 * https://www.mail-archive.com/libev@lists.schmorp.de/msg00428.html */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
		ev_io_init(&lid->con_watcher, _wc_on_fd_event_libev_cb, pollargs->fd,
				((pollargs->events & (WC_POLLIN | WC_POLLHUP)) ? EV_READ : 0)
					| ((pollargs->events & WC_POLLOUT) ? EV_WRITE : 0));
#pragma GCC diagnostic pop
		lid->con_watcher.data = ctx;
		ev_io_start(lid->loop, &lid->con_watcher);
		break;

	case WC_EVENT_DEL_FD:
		ev_io_stop(lid->loop, &lid->con_watcher);
		break;

	case WC_EVENT_MODIFY_FD:
		pollargs = data;
		ev_io_stop(lid->loop, &lid->con_watcher);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
		ev_io_init(&lid->con_watcher, _wc_on_fd_event_libev_cb, pollargs->fd,
				((pollargs->events & (WC_POLLIN | WC_POLLHUP)) ? EV_READ : 0)
					| ((pollargs->events & WC_POLLOUT) ? EV_WRITE : 0));
#pragma GCC diagnostic pop
		lid->con_watcher.data = ctx;
		ev_io_start(lid->loop, &lid->con_watcher);
		break;

	case WC_EVENT_ON_SERVER_HANDSHAKE:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
		ev_timer_init(&lid->ka_timer, _wc_on_ka_timer_libev_cb, 50., 50.);
#pragma GCC diagnostic pop
		lid->ka_timer.data = ctx;
		ev_timer_start(lid->loop, &lid->ka_timer);

		if (lid->next_try) {
			ev_timer_stop(lid->loop, &lid->recon_timer);
			lid->next_try = 0;
		}
		lid->callbacks.on_connected(ctx);
		break;

	case WC_EVENT_ON_CNX_CLOSED:
		ev_timer_stop(lid->loop, &lid->ka_timer);
		reconnect = lid->callbacks.on_disconnected(ctx);
		break;
	case WC_EVENT_ON_CNX_ERROR:
		reconnect = lid->callbacks.on_error(ctx, lid->next_try, data, len);
		break;
	default:
		break;
	}

	if (reconnect) {
		ev_tstamp in = (ev_tstamp) lid->next_try;

		if (lid->next_try == 0) {
			lid->next_try = 1;
		} else if (lid->next_try < 128) {
			lid->next_try <<= 1;
		}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
		ev_timer_init(&lid->recon_timer, _wc_on_recon_timer_libev_cb, in, 0.);
#pragma GCC diagnostic pop
		lid->recon_timer.data = ctx;
		ev_timer_start(lid->loop, &lid->recon_timer);
		W_DBG(WC_LOG_CONNECTION, "automatic reconnection attempt in %f sec", in);
	} else {
		if (lid->next_try != 0) {
			lid->next_try = 0;
		}
	}
}

wc_context_t *wc_context_new_with_libev(char *host, uint16_t port, char *application, struct ev_loop *loop, struct wc_eli_callbacks *callbacks) {
	struct wc_libev_integration_data *integration_data;
	wc_context_t *ret = NULL;

	integration_data = malloc(sizeof *integration_data);
	if (integration_data == NULL) {
		return NULL;
	}

	integration_data->callbacks = *callbacks;
	integration_data->loop = loop;
	integration_data->next_try = 0;

	ret = wc_context_new(host, port, application, _wc_libev_cb, integration_data);

	if (ret == NULL) {
		free(integration_data);
	}

	return ret;
}