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

#include "webcom-c/webcom-libuv.h"
#include "webcom-c/webcom-utils.h"
#include "webcom-c/webcom-log.h"

#if UV_VERSION_HEX < 0x010900
/* UV_DISCONNECT was introduced in libuv 1.9.0, fallback to UV_READABLE */
# define UV_DISCONNECT UV_READABLE
#endif

struct wc_libuv_integration_data {
	uv_loop_t *loop;
	struct wc_eli_callbacks callbacks;
	uv_poll_t con_watcher;
	uv_timer_t ka_timer;
	uv_timer_t recon_timer;
	unsigned next_try;
};

static inline void _wc_on_fd_event_libuv_cb (
		uv_poll_t* handle,
		UNUSED_PARAM(int status),
		int events)
{
	wc_context_t *ctx = handle->data;
	int fd;

	if (uv_fileno((uv_handle_t*)handle, &fd) == 0) {
		wc_dispatch_fd_event(ctx, fd,
				((events&UV_READABLE) ? POLLIN : 0)
				| ((events&UV_WRITABLE) ? POLLOUT : 0)
				| ((events&UV_DISCONNECT) ? POLLIN : 0)
		);
	}
}

static inline void _wc_on_ka_timer_libuv_cb (uv_timer_t *handle) {
	wc_context_t *ctx = handle->data;
	wc_datasync_keepalive(ctx);
}

static inline void _wc_on_recon_timer_libuv_cb (uv_timer_t *handle) {
	wc_context_t *ctx = handle->data;
	W_DBG(WC_LOG_CONNECTION, "reconnect callback triggered for context %p", ctx);
	uv_timer_stop(handle);
	wc_datasync_connect(ctx);
}

static void _wc_libuv_cb (wc_event_t event, wc_context_t *ctx, void *data, UNUSED_PARAM(size_t len)) {
	struct wc_libuv_integration_data *lid = wc_context_get_user_data(ctx);
	struct wc_pollargs *pollargs;
	int reconnect = 0;

	switch(event) {
	case WC_EVENT_ADD_FD:
		pollargs = data;
		uv_poll_init(lid->loop, &lid->con_watcher, pollargs->fd);
		lid->con_watcher.data = ctx;
		uv_poll_start(&lid->con_watcher,
				((pollargs->events & WC_POLLIN) ? UV_READABLE : 0)
					| ((pollargs->events & WC_POLLOUT) ? UV_WRITABLE : 0)
					| ((pollargs->events & WC_POLLHUP) ? UV_DISCONNECT : 0),
				_wc_on_fd_event_libuv_cb);
		break;

	case WC_EVENT_DEL_FD:
		uv_poll_stop(&lid->con_watcher);
		break;

	case WC_EVENT_MODIFY_FD:
		pollargs = data;
		uv_poll_start(&lid->con_watcher,
				((pollargs->events & WC_POLLIN) ? UV_READABLE : 0)
					| ((pollargs->events & WC_POLLOUT) ? UV_WRITABLE : 0)
					| ((pollargs->events & WC_POLLHUP) ? UV_DISCONNECT : 0),
				_wc_on_fd_event_libuv_cb);
		break;

	case WC_EVENT_ON_SERVER_HANDSHAKE:
		uv_timer_init(lid->loop, &lid->ka_timer);
		lid->ka_timer.data = ctx;
		uv_timer_start(&lid->ka_timer, _wc_on_ka_timer_libuv_cb, 50000, 50000);
		if (lid->next_try) {
			uv_timer_stop(&lid->recon_timer);
			lid->next_try = 0;
		}
		lid->callbacks.on_connected(ctx);
		break;

	case WC_EVENT_ON_CNX_CLOSED:
		uv_timer_stop(&lid->ka_timer);
		reconnect = lid->callbacks.on_disconnected(ctx);
		break;
	case WC_EVENT_ON_CNX_ERROR:
		reconnect = lid->callbacks.on_error(ctx, lid->next_try, data, len);
		break;
	default:
		break;
	}

	if (reconnect) {
		uint64_t in = 1000 * lid->next_try;

		if (lid->next_try == 0) {
			lid->next_try = 1;
		} else if (lid->next_try < 128) {
			lid->next_try <<= 1;
		}

		uv_timer_init(lid->loop, &lid->recon_timer);
		lid->recon_timer.data = ctx;
		uv_timer_start(&lid->recon_timer, _wc_on_recon_timer_libuv_cb, in, 0);
		W_DBG(WC_LOG_CONNECTION, "automatic reconnection attempt in %lu millisec", (unsigned long)in);
	} else {
		if (lid->next_try != 0) {
			lid->next_try = 0;
		}
	}
}

wc_context_t *wc_context_new_with_libuv(char *host, uint16_t port, char *application, uv_loop_t *loop, struct wc_eli_callbacks *callbacks) {
	struct wc_libuv_integration_data *integration_data;
	wc_context_t *ret = NULL;

	integration_data = malloc(sizeof *integration_data);
	if (integration_data == NULL) {
		return NULL;
	}

	integration_data->callbacks = *callbacks;
	integration_data->loop = loop;
	integration_data->next_try = 0;

	ret = wc_datasync_init(host, port, application, _wc_libuv_cb, integration_data);
	ret->event_loop = LWS_SERVER_OPTION_LIBUV;

	if (ret == NULL) {
		free(integration_data);
	}

	return ret;
}
