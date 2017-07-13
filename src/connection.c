#define _GNU_SOURCE
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

#include "webcom_priv.h"
#include "webcom-c/webcom-parser.h"
#include "webcom-c/webcom-req.h"

#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#define WEBCOM_PROTOCOL_VERSION "5"
#define WEBCOM_WS_PATH "/_wss/.ws"


int wc_process_incoming_message(wc_cnx_t *cnx, wc_msg_t *msg) {
	if (msg->type == WC_MSG_CTRL && msg->u.ctrl.type == WC_CTRL_MSG_HANDSHAKE) {
		int64_t now = wc_now();
		srand48_r((long int)now, &cnx->pids.rand_buffer);
		cnx->time_offset = now - msg->u.ctrl.u.handshake.ts;
		cnx->callback(WC_EVENT_ON_SERVER_HANDSHAKE, cnx, &msg, sizeof(wc_msg_t), cnx->user);
		cnx->time_offset = wc_now() - msg->u.ctrl.u.handshake.ts;
	} else if (msg->type == WC_MSG_DATA
			&& msg->u.data.type == WC_DATA_MSG_PUSH
			&& (msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_PUT
					|| msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_MERGE))
	{
		wc_on_data_dispatch(cnx, &msg->u.data.u.push);
	} else if (msg->type == WC_MSG_DATA && msg->u.data.type == WC_DATA_MSG_RESPONSE) {
		wc_req_response_dispatch(cnx, &msg->u.data.u.response);
	}
	cnx->callback(WC_EVENT_ON_MSG_RECEIVED, cnx, msg, sizeof(wc_msg_t), cnx->user);
	return 1;
}

void wc_cnx_on_readable(wc_cnx_t *cnx) {
	lws_service(cnx->lws_context, 100);
}

void _wc_process_incoming_data(wc_cnx_t *cnx, char *buf, size_t len) {
	wc_msg_t msg;

	if (cnx->parser == NULL) {
		if (*buf == '{') {
			cnx->parser = wc_parser_new();
		} else {
			return;
		}
	}
	switch (wc_parse_msg_ex(cnx->parser, buf, (size_t)len, &msg)) {
	case WC_PARSER_OK:
		wc_process_incoming_message(cnx, &msg);
		wc_parser_free(cnx->parser);
		cnx->parser = NULL;
		wc_msg_free(&msg);
		break;
	case WC_PARSER_CONTINUE:
		break;
	case WC_PARSER_ERROR:
		wc_parser_free(cnx->parser);
		cnx->parser = NULL;
		break;
	}
}

static int _wc_lws_callback(UNUSED_PARAM(struct lws *wsi), enum lws_callback_reasons reason, void *user, void *in, size_t len) {
	wc_cnx_t *cnx = (wc_cnx_t *) user;

	lwsl_debug("EVENT %d, user %p, in %p, %zu\n", reason, user, in, len);

	switch (reason) {
	struct lws_pollargs *pa;
	case LWS_CALLBACK_ADD_POLL_FD:
		pa = (struct lws_pollargs *)in;
		cnx->fd = pa->fd;
		break;
	case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
		/* TODO: fix hostname validation */
		break;
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		cnx->state = WC_CNX_STATE_READY;
		cnx->callback(WC_EVENT_ON_CNX_ESTABLISHED, cnx, NULL, 0, cnx->user);
		break;
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		cnx->state = WC_CNX_STATE_CLOSED;
		cnx->callback(WC_EVENT_ON_CNX_ERROR, cnx, in, len, cnx->user);
		lwsl_debug("ERROR: %.*s\n", (int)len, (char*)in);
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
		_wc_process_incoming_data(cnx, (char*)in, len);
		break;
	case LWS_CALLBACK_CLOSED:
		cnx->state = WC_CNX_STATE_CLOSED;
		break;
	default:
		break;
	}
	return 0;
}

int wc_cnx_send_msg(wc_cnx_t *cnx, wc_msg_t *msg) {
	char *jsonstr;
	unsigned char *buf;
	long len;
	int sent;

	if (cnx->state != WC_CNX_STATE_READY) {
		printf("status: %d != %d\n",cnx->state, WC_CNX_STATE_READY);
		return -1;
	}

	jsonstr = wc_msg_to_json_str(msg);
	len = strlen(jsonstr);

	buf = malloc (LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING);

	memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, jsonstr, len);

	sent = lws_write(cnx->lws_conn, buf + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);
	lwsl_debug("*** [%d] sent %s\n", sent, jsonstr);
	free(jsonstr);
	free(buf);
	return sent;
}

void wc_cnx_close(wc_cnx_t *cnx) {
	cnx->state = WC_CNX_STATE_CLOSED;
	lws_context_destroy(cnx->lws_context);
	cnx->lws_context = NULL;
	cnx->callback(WC_EVENT_ON_CNX_CLOSED, cnx, NULL, 0, cnx->user);
}

int wc_cnx_get_fd(wc_cnx_t *cnx) {
	return cnx->fd;
}

struct lws_protocols protocols[] = {
	{
		.name = "webcom-protocol",
		.callback =_wc_lws_callback,
		.rx_buffer_size = WC_RX_BUF_LEN
	},
	{.name=NULL}
};

static wc_cnx_t *wc_cnx_new_ex(char *proxy_host, uint16_t proxy_port, char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user) {
	wc_cnx_t *res;

	struct lws_client_connect_info lws_client_cnx_nfo;
	struct lws_context_creation_info lws_ctx_creation_nfo;

	memset(&lws_client_cnx_nfo, 0, sizeof(lws_client_cnx_nfo));
	memset(&lws_ctx_creation_nfo, 0, sizeof(lws_ctx_creation_nfo));

	res = calloc(1, sizeof(wc_cnx_t));

	if (res == NULL) {
		goto error1;
	}

	/*
	 * XXX: logging configuration
	lws_set_log_level(0xffffffff, NULL);
	 */
	lws_set_log_level(0, NULL);
	lws_ctx_creation_nfo.port = CONTEXT_PORT_NO_LISTEN;
	lws_ctx_creation_nfo.protocols = protocols;
	lws_ctx_creation_nfo.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	lws_ctx_creation_nfo.gid = -1;
	lws_ctx_creation_nfo.uid = -1;
	if (proxy_host != NULL) {
		lws_ctx_creation_nfo.http_proxy_address = proxy_host;
		lws_ctx_creation_nfo.http_proxy_port = (unsigned int)proxy_port;
	}
	res->lws_context = lws_create_context(&lws_ctx_creation_nfo);

	lws_client_cnx_nfo.context = res->lws_context;
	lws_client_cnx_nfo.address = host;
	lws_client_cnx_nfo.host = host;
	lws_client_cnx_nfo.port = (int)port;
	lws_client_cnx_nfo.ssl_connection = 1;
	asprintf((char **)&lws_client_cnx_nfo.path, "%s?v=%s&ns=%s", WEBCOM_WS_PATH, WEBCOM_PROTOCOL_VERSION, application);
	lws_client_cnx_nfo.protocol = protocols[0].name;
	lws_client_cnx_nfo.ietf_version_or_minus_one = -1;
	lws_client_cnx_nfo.userdata = (void *)res;
	res->user = user;
	res->callback = callback;

	res->lws_conn = lws_client_connect_via_info(&lws_client_cnx_nfo);
	res->state = WC_CNX_STATE_INIT;
	lws_service(res->lws_context, 100);
	free((char *)lws_client_cnx_nfo.path);

	if(res->fd <= 0) {
		goto error2;
	}

	return res;
error2:
	free(res);
error1:
	return NULL;
}

wc_cnx_t *wc_cnx_new_with_proxy(char *proxy_host, uint16_t proxy_port, char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user) {
	return wc_cnx_new_ex(proxy_host, proxy_port, host, port, application, callback, user);
}

wc_cnx_t *wc_cnx_new(char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user) {
	return wc_cnx_new_ex(NULL, 0, host, port, application, callback, user);
}

void wc_cnx_free(wc_cnx_t *cnx) {
	if (cnx->lws_context != NULL) lws_context_destroy(cnx->lws_context);
	if (cnx->parser != NULL) wc_parser_free(cnx->parser);

	wc_free_on_data_handlers(cnx->handlers);
	wc_free_pending_trans(cnx->pending_req_table);

	free(cnx);
}

int wc_cnx_keepalive(wc_cnx_t *cnx) {
	int sent = 0;
	unsigned char keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING + 1 + LWS_SEND_BUFFER_POST_PADDING];

	keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING] = '0';
	sent = lws_write(cnx->lws_conn, &(keepalive_msg[LWS_SEND_BUFFER_PRE_PADDING]), 1, LWS_WRITE_TEXT);
	lwsl_debug("*** [%d] keepalive sent\n", sent);

	return sent;
}
