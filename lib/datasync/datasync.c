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

#include "../webcom_base_priv.h"
#include "webcom-c/webcom.h"

#define WEBCOM_PROTOCOL_VERSION "5"
#define WEBCOM_WS_PATH "/_wss/.ws"

static int _wc_datasync_process_message(wc_context_t *ctx, wc_msg_t *msg) {
	struct wc_timerargs ta;
	if (msg->type == WC_MSG_CTRL && msg->u.ctrl.type == WC_CTRL_MSG_HANDSHAKE) {
		int64_t now = wc_datasync_now();
		srand48_r((long int)now, &ctx->datasync.pids.rand_buffer);
		ctx->datasync.time_offset = now - msg->u.ctrl.u.handshake.ts;
		ctx->datasync.state = WC_CNX_STATE_CONNECTED;
		ctx->datasync.time_offset = wc_datasync_now() - msg->u.ctrl.u.handshake.ts;
		ctx->callback(WC_EVENT_ON_SERVER_HANDSHAKE, ctx, msg, sizeof(wc_msg_t));
		ta.ms = 50000;
		ta.repeat = 1;
		ta.timer = WC_TIMER_DATASYNC_KEEPALIVE;
		ctx->datasync.ws_next_reconnect_timer = 0;
		ctx->callback(WC_EVENT_SET_TIMER, ctx, &ta, 0);
	} else if (msg->type == WC_MSG_DATA
			&& msg->u.data.type == WC_DATA_MSG_PUSH
			&& (msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_PUT
					|| msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_MERGE))
	{
		wc_datasync_dispatch_data(ctx, &msg->u.data.u.push);
	} else if (msg->type == WC_MSG_DATA && msg->u.data.type == WC_DATA_MSG_RESPONSE) {
		wc_datasync_req_response_dispatch(ctx, &msg->u.data.u.response);
	}
	ctx->callback(WC_EVENT_ON_MSG_RECEIVED, ctx, msg, sizeof(wc_msg_t));
	return 1;
}

void _wc_datasync_process_data(wc_context_t *ctx, char *buf, size_t len) {
	wc_msg_t msg;

	if (ctx->datasync.parser == NULL) {
		if (*buf == '{') {
			ctx->datasync.parser = wc_datasync_parser_new();
		} else {
			return;
		}
	}
	switch (wc_datasync_parse_msg_ex(ctx->datasync.parser, buf, (size_t)len, &msg)) {
	case WC_PARSER_OK:
		_wc_datasync_process_message(ctx, &msg);
		wc_datasync_parser_free(ctx->datasync.parser);
		ctx->datasync.parser = NULL;
		wc_datasync_free(&msg);
		break;
	case WC_PARSER_CONTINUE:
		break;
	case WC_PARSER_ERROR:
		wc_datasync_parser_free(ctx->datasync.parser);
		ctx->datasync.parser = NULL;
		break;
	}
}

static void _wc_datasync_schedule_reconnect(wc_context_t *ctx) {
	struct wc_timerargs wcta;

	wcta.ms = 1000 * ctx->datasync.ws_next_reconnect_timer;
	wcta.repeat = 0;
	wcta.timer = WC_TIMER_DATASYNC_RECONNECT;
	ctx->callback(WC_EVENT_SET_TIMER, ctx, &wcta, 0);
	if (ctx->datasync.ws_next_reconnect_timer < (1 << 8)) {
		ctx->datasync.ws_next_reconnect_timer <<= 1;
	}
}

void wc_datasync_service_socket(wc_context_t *ctx, struct wc_pollargs *pa) {
	struct lws_pollfd pfd;

	pfd.fd = pa->fd;
	pfd.events = pa->events;
	pfd.revents = pa->events;

	lws_service_fd(ctx->datasync.lws_cci.context, &pfd);
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
		ctx->datasync.state = WC_CNX_STATE_DISCONNECTED;
		WL_ERR("connection error \"%.*s\"", (int)len, (char*)in);
		if (ctx->callback(WC_EVENT_ON_CNX_ERROR, ctx, in, len)) {
			_wc_datasync_schedule_reconnect(ctx);
		}
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
		if (ctx->datasync.state != WC_CNX_STATE_DISCONNECTING) {
			_wc_datasync_process_data(ctx, (char*)in, len);
		}
		break;
	case LWS_CALLBACK_CLOSED:
		ctx->datasync.state = WC_CNX_STATE_DISCONNECTED;
		wcta.timer = WC_TIMER_DATASYNC_KEEPALIVE;
		ctx->callback(WC_EVENT_DEL_TIMER, ctx, &wcta.timer, 0);
		if (ctx->callback(WC_EVENT_ON_CNX_CLOSED, ctx, NULL, 0)) {
			_wc_datasync_schedule_reconnect(ctx);
		}
		break;
	case LWS_CALLBACK_CLIENT_WRITEABLE:
		if (ctx->datasync.state == WC_CNX_STATE_DISCONNECTING) {
			return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}

int wc_datasync_send_msg(wc_context_t *ctx, wc_msg_t *msg) {
	char *jsonstr;
	unsigned char *buf;
	long len;
	int sent;

	if (ctx->datasync.state != WC_CNX_STATE_CONNECTED) {
		WL_WARN("message not sent, the context %p is not connected (state %d)", ctx, ctx->datasync.state);
		return -1;
	}

	jsonstr = wc_datasync_msg_to_json_str(msg);
	if (jsonstr == NULL) {
		return -1;
	}
	len = strlen(jsonstr);

	buf = malloc (LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING);

	memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, jsonstr, len);

	sent = lws_write(ctx->datasync.lws_conn, buf + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);
	WL_DBG("%d bytes sent:\n\t%s", sent, jsonstr);
	free(jsonstr);
	free(buf);
	return sent;
}

void wc_datasync_close_cnx(wc_context_t *ctx) {
	switch (ctx->datasync.state) {
	case WC_CNX_STATE_CONNECTED:
	case WC_CNX_STATE_CONNECTING:
		ctx->datasync.state = WC_CNX_STATE_DISCONNECTING;
		lws_callback_on_writable(ctx->datasync.lws_conn);
		break;
	case WC_CNX_STATE_DISCONNECTING:
	case WC_CNX_STATE_DISCONNECTED:
		WL_WARN("wc_context_close_cnx called for already closing/ed connection in context %p", ctx);
		break;
	}
}

static struct lws_protocols protocols[] = {
	{
		.name = "webcom-protocol",
		.callback =_wc_lws_callback,
		.rx_buffer_size = WC_RX_BUF_LEN
	},
	{.name=NULL}
};

wc_datasync_context_t *wc_datasync_init(wc_context_t *ctx) {
	size_t ws_path_l;

	struct lws_context_creation_info lws_ctx_creation_nfo;
	memset(&lws_ctx_creation_nfo, 0, sizeof(lws_ctx_creation_nfo));

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

	ctx->datasync.lws_cci.context = lws_create_context(&lws_ctx_creation_nfo);
	ctx->datasync.lws_cci.address = ctx->host;
	ctx->datasync.lws_cci.host = ctx->host;
	ctx->datasync.lws_cci.port = (int)ctx->port;
	ctx->datasync.lws_cci.ssl_connection = 1;
	ws_path_l = (
			sizeof(WEBCOM_WS_PATH) - 1
			+ 3
			+ sizeof(WEBCOM_PROTOCOL_VERSION) - 1
			+ 4
			+ strlen(ctx->app_name)
			+ 1 );
	ctx->datasync.lws_cci.path = malloc(ws_path_l);
	if (ctx->datasync.lws_cci.path == NULL) {
		return NULL;
	}
	snprintf((char*)ctx->datasync.lws_cci.path, ws_path_l, "%s?v=%s&ns=%s", WEBCOM_WS_PATH, WEBCOM_PROTOCOL_VERSION, ctx->app_name);
	ctx->datasync.lws_cci.protocol = protocols[0].name;
	ctx->datasync.lws_cci.ietf_version_or_minus_one = -1;
	ctx->datasync.lws_cci.userdata = (void *)ctx;

	ctx->datasync.state = WC_CNX_STATE_DISCONNECTED;

	ctx->datasync_init = 1;

	return &ctx->datasync;
}

void _wc_datasync_connect(wc_context_t *ctx) {
	if (ctx->datasync.state == WC_CNX_STATE_DISCONNECTED) {
		ctx->datasync.state = WC_CNX_STATE_CONNECTING;
		ctx->datasync.lws_conn = lws_client_connect_via_info(&ctx->datasync.lws_cci);
		lws_service(ctx->datasync.lws_cci.context, 0);
	} else {
		WL_WARN("bad state for _wc_datasync_connect() in context %p, expected %d, got %d", ctx, WC_CNX_STATE_DISCONNECTED, ctx->datasync.state);
	}
}

void wc_datasync_connect(wc_context_t *ctx) {
	_wc_datasync_schedule_reconnect(ctx);
}

void wc_datasync_context_cleanup(struct wc_datasync_context *ds_ctx) {
	if (ds_ctx->lws_cci.path != NULL) free((char*)ds_ctx->lws_cci.path);
	//TODO add destrucors in base / auth
	//if (ctx->app_name != NULL) free(ctx->app_name);
	//if (ctx->host != NULL) free(ctx->host);
	//if (ctx->auth_form_data != NULL) free(ctx->auth_form_data);
	//if (ctx->auth_url != NULL) free(ctx->auth_url);
	if (ds_ctx->lws_cci.context != NULL) lws_context_destroy(ds_ctx->lws_cci.context);
	if (ds_ctx->parser != NULL) wc_datasync_parser_free(ds_ctx->parser);

	wc_datasync_cleanup_data_routes(ds_ctx->data_routes);
	wc_datasync_free_pending_trans(ds_ctx->pending_req_table);
}

int wc_datasync_keepalive(wc_context_t *ctx) {
	int sent = 0;
	unsigned char keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING + 1 + LWS_SEND_BUFFER_POST_PADDING];

	keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING] = '0';
	sent = lws_write(ctx->datasync.lws_conn, &(keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING]), 1, LWS_WRITE_TEXT);
	if (sent) {
		WL_DBG("keepalive frame sent");
	} else {
		WL_ERR("error sending keepalive frame");
	}

	return sent;
}
