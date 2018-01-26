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

#define LOCAL_LOG_FACILITY WC_LOG_CONNECTION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <libwebsockets.h>
#include <assert.h>
#include <poll.h>

#include "webcom_priv.h"
#include "webcom-c/webcom-parser.h"
#include "webcom-c/webcom-req.h"

#define WEBCOM_PROTOCOL_VERSION "5"
#define WEBCOM_WS_PATH "/_wss/.ws"

static void _wc_context_connect(wc_context_t *ctx);

static int _wc_process_incoming_message(wc_context_t *ctx, wc_msg_t *msg) {
	struct wc_timerargs ta;
	if (msg->type == WC_MSG_CTRL && msg->u.ctrl.type == WC_CTRL_MSG_HANDSHAKE) {
		int64_t now = wc_now();
		srand48_r((long int)now, &ctx->pids.rand_buffer);
		ctx->time_offset = now - msg->u.ctrl.u.handshake.ts;
		ctx->state = WC_CNX_STATE_CONNECTED;
		ctx->time_offset = wc_now() - msg->u.ctrl.u.handshake.ts;
		ctx->callback(WC_EVENT_ON_SERVER_HANDSHAKE, ctx, msg, sizeof(wc_msg_t));
		ta.ms = 50000;
		ta.repeat = 1;
		ta.timer = WC_TIMER_DATASYNC_KEEPALIVE;
		ctx->ws_next_reconnect_timer = 0;
		ctx->callback(WC_EVENT_SET_TIMER, ctx, &ta, 0);
	} else if (msg->type == WC_MSG_DATA
			&& msg->u.data.type == WC_DATA_MSG_PUSH
			&& (msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_PUT
					|| msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_MERGE))
	{
		wc_on_data_dispatch(ctx, &msg->u.data.u.push);
	} else if (msg->type == WC_MSG_DATA && msg->u.data.type == WC_DATA_MSG_RESPONSE) {
		wc_req_response_dispatch(ctx, &msg->u.data.u.response);
	}
	ctx->callback(WC_EVENT_ON_MSG_RECEIVED, ctx, msg, sizeof(wc_msg_t));
	return 1;
}

void wc_handle_fd_events(wc_context_t *ctx, struct wc_pollargs *pa) {
	struct lws_pollfd pfd;

	pfd.fd = pa->fd;
	pfd.events = pa->events;
	pfd.revents = pa->events;
	if (pa->src == WC_POLL_DATASYNC) {
		lws_service_fd(ctx->lws_cci.context, &pfd);
	} else if (pa->src == WC_POLL_AUTH) {
		wc_auth_event_action(ctx, pa->fd);
	}
}

void wc_handle_timer(wc_context_t *ctx, enum wc_timersrc timer) {
	switch (timer) {
	case WC_TIMER_DATASYNC_KEEPALIVE:
		wc_cnx_keepalive(ctx);
		break;
	case WC_TIMER_DATASYNC_RECONNECT:
		wc_context_reconnect(ctx);
		break;
	case WC_TIMER_AUTH:
		wc_auth_event_action(ctx, -1);
		break;
	default:
		break;
	}
}

void _wc_process_incoming_data(wc_context_t *ctx, char *buf, size_t len) {
	wc_msg_t msg;

	if (ctx->parser == NULL) {
		if (*buf == '{') {
			ctx->parser = wc_parser_new();
		} else {
			return;
		}
	}
	switch (wc_parse_msg_ex(ctx->parser, buf, (size_t)len, &msg)) {
	case WC_PARSER_OK:
		_wc_process_incoming_message(ctx, &msg);
		wc_parser_free(ctx->parser);
		ctx->parser = NULL;
		wc_msg_free(&msg);
		break;
	case WC_PARSER_CONTINUE:
		break;
	case WC_PARSER_ERROR:
		wc_parser_free(ctx->parser);
		ctx->parser = NULL;
		break;
	}
}

static void _wc_schedule_reconnect(wc_context_t *ctx) {
	struct wc_timerargs wcta;

	wcta.ms = 1000 * ctx->ws_next_reconnect_timer;
	wcta.repeat = 0;
	wcta.timer = WC_TIMER_DATASYNC_RECONNECT;
	ctx->callback(WC_EVENT_SET_TIMER, ctx, &wcta, 0);
	if (ctx->ws_next_reconnect_timer < (1 << 8)) {
		ctx->ws_next_reconnect_timer <<= 1;
	}
}

static int _wc_lws_callback(UNUSED_PARAM(struct lws *wsi), enum lws_callback_reasons reason, void *user, void *in, size_t len) {
	wc_context_t *ctx = (wc_context_t *) user;
	struct wc_pollargs wcpa;
	struct wc_timerargs wcta;
	struct lws_pollargs *pa;

	WL_EXDBG("EVENT %d, user %p, in %p, %zu", reason, user, in, len);

#if defined(LWS_LIBRARY_VERSION_NUMBER) && LWS_LIBRARY_VERSION_NUMBER < 2000000
	/* in LWS < 2, lws_client_connect_via_info() can call the callback  for
	 * the LWS_CALLBACK_CHANGE_MODE_POLL_FD event, with a bogus fd, before the
	 * user data has been saved in the wsi structure */
	if (ctx == NULL) return 0;
#endif

	switch (reason) {
	case LWS_CALLBACK_ADD_POLL_FD:
		pa = in;
		wcpa.fd = pa->fd;
		wcpa.events = pa->events;
		wcpa.src = WC_POLL_DATASYNC;
		WL_DBG("request to add fd %d to poll set for events %hd", wcpa.fd, wcpa.events);
		ctx->callback(WC_EVENT_ADD_FD, ctx, &wcpa, 0);
		break;
	case LWS_CALLBACK_DEL_POLL_FD:
		pa = in;
		wcpa.fd = pa->fd;
		wcpa.events = pa->events;
		wcpa.src = WC_POLL_DATASYNC;
		WL_DBG("request to remove fd %d from poll set", wcpa.fd);
		ctx->callback(WC_EVENT_DEL_FD, ctx, &wcpa, 0);
		break;
	case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
		pa = in;
		wcpa.fd = pa->fd;
		wcpa.events = pa->events;
		wcpa.src = WC_POLL_DATASYNC;
		WL_DBG("request to modify fd %d from events %d to %hd", wcpa.fd, pa->prev_events, wcpa.events);
		ctx->callback(WC_EVENT_MODIFY_FD, ctx, &wcpa, 0);
		break;
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		WL_DBG("client websocket established for context %p", ctx);
		break;
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		ctx->state = WC_CNX_STATE_DISCONNECTED;
		WL_ERR("connection error \"%.*s\"", (int)len, (char*)in);
		if (ctx->callback(WC_EVENT_ON_CNX_ERROR, ctx, in, len)) {
			_wc_schedule_reconnect(ctx);
		}
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
		if (ctx->state != WC_CNX_STATE_DISCONNECTING) {
			_wc_process_incoming_data(ctx, (char*)in, len);
		}
		break;
	case LWS_CALLBACK_CLOSED:
		ctx->state = WC_CNX_STATE_DISCONNECTED;
		wcta.timer = WC_TIMER_DATASYNC_KEEPALIVE;
		ctx->callback(WC_EVENT_DEL_TIMER, ctx, &wcta.timer, 0);
		if (ctx->callback(WC_EVENT_ON_CNX_CLOSED, ctx, NULL, 0)) {
			_wc_schedule_reconnect(ctx);
		}
		break;
	case LWS_CALLBACK_CLIENT_WRITEABLE:
		if (ctx->state == WC_CNX_STATE_DISCONNECTING) {
			return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}

int wc_context_send_msg(wc_context_t *ctx, wc_msg_t *msg) {
	char *jsonstr;
	unsigned char *buf;
	long len;
	int sent;

	if (ctx->state != WC_CNX_STATE_CONNECTED) {
		WL_WARN("message not sent, the context %p is not connected (state %d)", ctx, ctx->state);
		return -1;
	}

	jsonstr = wc_msg_to_json_str(msg);
	if (jsonstr == NULL) {
		return -1;
	}
	len = strlen(jsonstr);

	buf = malloc (LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING);

	memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, jsonstr, len);

	sent = lws_write(ctx->lws_conn, buf + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);
	WL_DBG("%d bytes sent:\n\t%s", sent, jsonstr);
	free(jsonstr);
	free(buf);
	return sent;
}

void wc_context_close_cnx(wc_context_t *ctx) {
	switch (ctx->state) {
	case WC_CNX_STATE_CONNECTED:
	case WC_CNX_STATE_CONNECTING:
		ctx->state = WC_CNX_STATE_DISCONNECTING;
		lws_callback_on_writable(ctx->lws_conn);
		break;
	case WC_CNX_STATE_DISCONNECTING:
	case WC_CNX_STATE_DISCONNECTED:
		WL_WARN("wc_context_close_cnx called for already closing/ed connection in context %p", ctx);
		break;
	}
}

struct lws_protocols protocols[] = {
	{
		.name = "webcom-protocol",
		.callback =_wc_lws_callback,
		.rx_buffer_size = WC_RX_BUF_LEN
	},
	{.name=NULL}
};

wc_context_t *wc_context_new(char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user) {
	wc_context_t *res;
	size_t ws_path_l;

	struct lws_context_creation_info lws_ctx_creation_nfo;
	memset(&lws_ctx_creation_nfo, 0, sizeof(lws_ctx_creation_nfo));

	res = calloc(1, sizeof(wc_context_t));

	if (res == NULL) {
		return NULL;
	}

	lws_ctx_creation_nfo.port = CONTEXT_PORT_NO_LISTEN;
	lws_ctx_creation_nfo.protocols = protocols;
#if defined(LWS_LIBRARY_VERSION_MAJOR) && LWS_LIBRARY_VERSION_MAJOR >= 2
	lws_ctx_creation_nfo.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#endif
	lws_ctx_creation_nfo.gid = -1;
	lws_ctx_creation_nfo.uid = -1;

#if defined(LWS_LIBRARY_VERSION_NUMBER) && LWS_LIBRARY_VERSION_NUMBER < 2002000
	char *proxy;
	/* hack to make LWS support the http_proxy variable beginning with "http://" */
	proxy = getenv("http_proxy");
	if (proxy != NULL && strncmp("http://", proxy, 7) == 0) {
		proxy += 7;
	}
	lws_ctx_creation_nfo.http_proxy_address = proxy;
#endif

	res->lws_cci.context = lws_create_context(&lws_ctx_creation_nfo);
	res->lws_cci.address = host;
	res->lws_cci.host = host;
	res->lws_cci.port = (int)port;
	res->lws_cci.ssl_connection = 1;
	ws_path_l = (
			sizeof(WEBCOM_WS_PATH) - 1
			+ 3
			+ sizeof(WEBCOM_PROTOCOL_VERSION) - 1
			+ 4
			+ strlen(application)
			+ 1 );
	res->lws_cci.path = malloc(ws_path_l);
	if (res->lws_cci.path == NULL) {
		free(res);
		return NULL;
	}
	snprintf((char*)res->lws_cci.path, ws_path_l, "%s?v=%s&ns=%s", WEBCOM_WS_PATH, WEBCOM_PROTOCOL_VERSION, application);
	res->lws_cci.protocol = protocols[0].name;
	res->lws_cci.ietf_version_or_minus_one = -1;
	res->lws_cci.userdata = (void *)res;
	res->user = user;
	res->callback = callback;
	res->app_name = strdup(application);
	res->host = strdup(host);
	res->port = port;

	res->state = WC_CNX_STATE_DISCONNECTED;
	_wc_context_connect(res);

	return res;
}

static void _wc_context_connect(wc_context_t *ctx) {
	ctx->state = WC_CNX_STATE_CONNECTING;
	ctx->lws_conn = lws_client_connect_via_info(&ctx->lws_cci);
	lws_service(ctx->lws_cci.context, 0);
}

void wc_context_reconnect(wc_context_t *ctx) {
	if (ctx->state == WC_CNX_STATE_DISCONNECTED) {
		_wc_context_connect(ctx);
	} else {
		WL_WARN("called wc_context_reconnect() on not disconnected connection in context %p", ctx);
	}
}

void wc_context_free(wc_context_t *ctx) {
	if (ctx->lws_cci.path != NULL) free((char*)ctx->lws_cci.path);
	if (ctx->app_name != NULL) free(ctx->app_name);
	if (ctx->host != NULL) free(ctx->host);
	if (ctx->auth_form_data != NULL) free(ctx->auth_form_data);
	if (ctx->auth_url != NULL) free(ctx->auth_url);
	if (ctx->lws_cci.context != NULL) lws_context_destroy(ctx->lws_cci.context);
	if (ctx->parser != NULL) wc_parser_free(ctx->parser);

	wc_free_on_data_handlers(ctx->handlers);
	wc_free_pending_trans(ctx->pending_req_table);

	free(ctx);
}

int wc_cnx_keepalive(wc_context_t *ctx) {
	int sent = 0;
	unsigned char keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING + 1 + LWS_SEND_BUFFER_POST_PADDING];

	keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING] = '0';
	sent = lws_write(ctx->lws_conn, &(keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING]), 1, LWS_WRITE_TEXT);
	if (sent) {
		WL_DBG("keepalive frame sent");
	} else {
		WL_ERR("error sending keepalive frame");
	}

	return sent;
}

void *wc_context_get_user_data(wc_context_t *ctx) {
	return ctx->user;
}
