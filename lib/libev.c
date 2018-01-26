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
#include "webcom-c/webcom-cnx.h"
#include "webcom-c/webcom-utils.h"
#include "webcom-c/webcom-log.h"

#include "event_libs_priv.h"

struct wc_libev_integration_data {
	struct ev_loop *loop;
	struct wc_eli_callbacks callbacks;
	struct ev_io fd_watcher[_WC_POLL_MAX];
	struct ev_timer timer_events[_WC_TIMER_MAX];
	unsigned next_try;
};

static inline void _wc_on_fd_event_libev_cb (
		UNUSED_PARAM(struct ev_loop *loop),
		ev_io *w,
		UNUSED_PARAM(int revents))
{
	wc_context_t *ctx = w->data;
	struct wc_libev_integration_data *lid = wc_context_get_user_data(ctx);
	struct wc_pollargs pa;

	pa.fd = w->fd;
	pa.events = ((revents&EV_READ) ? POLLIN : 0)
			| ((revents&EV_WRITE) ? POLLOUT : 0);
	pa.src = w - lid->fd_watcher;
	wc_handle_fd_events(ctx, &pa);
}

static inline void _wc_on_timer_libev_cb (
		UNUSED_PARAM(struct ev_loop *loop),
		ev_timer *w,
		UNUSED_PARAM(int revents))
{
	wc_context_t *ctx = w->data;
	struct wc_libev_integration_data *lid = wc_context_get_user_data(ctx);
	enum wc_timersrc timer = w - lid->timer_events;

	wc_handle_timer(ctx, timer);
}

static int _wc_libev_cb (wc_event_t event, wc_context_t *ctx, void *data, size_t len) {
	struct wc_libev_integration_data *lid = wc_context_get_user_data(ctx);
	struct wc_pollargs *pollargs;
	struct wc_timerargs *timerargs;
	struct ev_io *watcher;
	struct ev_timer *timer;
	enum wc_timersrc wctimer;
	ev_tstamp in, repeat;
	struct wc_auth_info* ai;
	int ret = 0;

	switch(event) {
	case WC_EVENT_ADD_FD:
		pollargs = data;
		watcher = &lid->fd_watcher[pollargs->src];
/* ignoring some GCC warnings issued for libev, see
 * https://www.mail-archive.com/libev@lists.schmorp.de/msg00428.html */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
		ev_io_init(watcher, _wc_on_fd_event_libev_cb, pollargs->fd,
				((pollargs->events & (WC_POLLIN | WC_POLLHUP)) ? EV_READ : 0)
					| ((pollargs->events & WC_POLLOUT) ? EV_WRITE : 0));
#pragma GCC diagnostic pop
		watcher->data = ctx;
		ev_io_start(lid->loop, watcher);
		break;

	case WC_EVENT_DEL_FD:
		pollargs = data;
		watcher = &lid->fd_watcher[pollargs->src];
		ev_io_stop(lid->loop, watcher);
		break;

	case WC_EVENT_MODIFY_FD:
		pollargs = data;
		watcher = &lid->fd_watcher[pollargs->src];
		ev_io_stop(lid->loop, watcher);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
		ev_io_init(watcher, _wc_on_fd_event_libev_cb, pollargs->fd,
				((pollargs->events & (WC_POLLIN | WC_POLLHUP)) ? EV_READ : 0)
					| ((pollargs->events & WC_POLLOUT) ? EV_WRITE : 0));
#pragma GCC diagnostic pop
		watcher->data = ctx;
		ev_io_start(lid->loop, watcher);
		break;

	case WC_EVENT_ON_SERVER_HANDSHAKE:
		CALLBACK_SAFE_VOID(lid->callbacks.on_connected, ctx);
		break;

	case WC_EVENT_ON_CNX_CLOSED:
		ret = CALLBACK_SAFE_INT(lid->callbacks.on_disconnected, ctx);
		break;
	case WC_EVENT_ON_CNX_ERROR:
		//ret = lid->callbacks.on_error(ctx, lid->next_try, data, len);
		ret = CALLBACK_SAFE_INT(lid->callbacks.on_error, ctx, lid->next_try, data, len);
		break;
	case WC_EVENT_SET_TIMER:
		timerargs = data;
		timer = &lid->timer_events[timerargs->timer];
		in = ((ev_tstamp)timerargs->ms) / 1000.;
		repeat = timerargs->repeat ? in : 0.;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
		ev_timer_init(timer, _wc_on_timer_libev_cb, in, repeat);
#pragma GCC diagnostic pop
		timer->data = ctx;
		ev_timer_start(lid->loop, timer);
		break;
	case WC_EVENT_DEL_TIMER:
		wctimer = *((enum wc_timersrc *)data);
		timer = &lid->timer_events[wctimer];
		ev_timer_stop(lid->loop, timer);
		break;
	case WC_AUTH_ON_AUTH_REPLY:
		ai = data;
		if (ai->error) {
			CALLBACK_SAFE_VOID(lid->callbacks.on_auth_error, ctx, ai->error);
		} else {
			CALLBACK_SAFE_VOID(lid->callbacks.on_auth_success, ctx, ai);
		}
		break;
	default:
		break;
	}

	return ret;
}

wc_context_t *wc_context_new_with_libev(char *host, uint16_t port, char *application, struct ev_loop *loop, struct wc_eli_callbacks *callbacks) {
	struct wc_libev_integration_data *integration_data;
	wc_context_t *ret = NULL;

	integration_data = calloc(1, sizeof *integration_data);
	if (integration_data == NULL) {
		return NULL;
	}

	integration_data->callbacks = *callbacks;
	integration_data->loop = loop;

	ret = wc_context_new(host, port, application, _wc_libev_cb, integration_data);

	if (ret == NULL) {
		free(integration_data);
	}

	return ret;
}
