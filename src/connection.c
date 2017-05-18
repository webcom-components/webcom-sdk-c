#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nopoll.h>

#include "webcom-c/webcom-cnx.h"
#include "webcom-c/webcom-parser.h"

typedef enum {
	WC_CNX_STATE_INIT = 0,
	WC_CNX_STATE_CREATED,
	WC_CNX_STATE_READY,
	WC_CNX_STATE_CLOSED,
} wc_cnx_state_t;

#define WC_RX_BUF_LEN	(1 << 12)

typedef struct wc_cnx {
	noPollCtx *np_ctx;
	noPollConn *np_conn;
	wc_on_event_cb_t callback;
	void *user;
	int fd;
	wc_cnx_state_t state;
	wc_parser_t *parser;
	char rxbuf[WC_RX_BUF_LEN];
} wc_cnx_t;

int wc_cnx_on_readable(wc_cnx_t *cnx) {
	int n, ret = 0;
	wc_msg_t msg;

	n = nopoll_conn_read(cnx->np_conn, cnx->rxbuf, WC_RX_BUF_LEN, nopoll_false, 0);

	if (n > 0) {
		if (cnx->parser == NULL) {
			cnx->parser = wc_parser_new();
		}
		switch (wc_parse_msg_ex(cnx->parser, cnx->rxbuf, (size_t)n, &msg)) {
		case WC_PARSER_OK:
			cnx->callback(WC_EVENT_ON_MSG_RECEIVED, cnx, &msg, sizeof(wc_msg_t), cnx->user);
			ret = 1;
			wc_parser_free(cnx->parser);
			cnx->parser = NULL;
			break;
		case WC_PARSER_CONTINUE:
			ret = 2;
			break;
		case WC_PARSER_ERROR:
			wc_parser_free(cnx->parser);
			cnx->parser = NULL;
			break;
		}
		}
	} else if (n == 0 && cnx->state == WC_CNX_STATE_CREATED) {

	}
	return ret;
}

int wc_cnx_send_msg(wc_cnx_t *cnx, wc_msg_t *msg) {
	char *jsonstr;
	long len;
	int sent;

	if (cnx->state != WC_CNX_STATE_READY) {
		printf("status: %d != %d\n",cnx->state, WC_CNX_STATE_READY);
		return -1;
	}

	jsonstr = wc_msg_to_json_str(msg);
	len = strlen(jsonstr);

	sent = nopoll_conn_send_text(cnx->np_conn, jsonstr, len);
	nopoll_conn_flush_writes(cnx->np_conn, 2000, 0);

	return sent;
}

void wc_cnx_close(wc_cnx_t *cnx) {
	nopoll_conn_close(cnx->np_conn);
	cnx->callback(WC_EVENT_ON_CNX_CLOSED, cnx, NULL, 0, cnx->user);
}

int wc_cnx_get_fd(wc_cnx_t *cnx) {
	return cnx->fd;
}

wc_cnx_t *wc_cnx_new(char *endpoint, int port, char *path, wc_on_event_cb_t callback, void *user) {
	wc_cnx_t *res;
	char sport[6];

	res = calloc(1, sizeof(wc_cnx_t));

	if (res == NULL) {
		goto error1;
	}

	res->np_ctx = nopoll_ctx_new();

	snprintf(sport, 6, "%hu", port);

	sport[5] = '\0';
	res->np_conn = nopoll_conn_tls_new(res->np_ctx, 0, endpoint, sport, NULL, path, NULL, NULL);
	if (nopoll_conn_is_ok(res->np_conn)) {
		res->state = WC_CNX_STATE_CREATED;
		res->user = user;
		res->callback = callback;
		res->fd = nopoll_conn_socket(res->np_conn);
		nopoll_conn_wait_until_connection_ready(res->np_conn, 2);
		res->state = WC_CNX_STATE_READY;
		callback(WC_EVENT_ON_CNX_ESTABLISHED, res, NULL, 0, user);
	} else {
		goto error2;
	}

	return res;

error2:
	free(res);
error1:
	return NULL;
}

void wc_cnx_free(wc_cnx_t *cnx) {
	nopoll_ctx_unref(cnx->np_ctx);
	free(cnx);
}

int wc_cnx_keepalive(wc_cnx_t *cnx) {
	int sent = 0;
	static char keepalive = '0';

	sent = nopoll_conn_send_text(cnx->np_conn, &keepalive, 1);
	nopoll_conn_flush_writes(cnx->np_conn, 2000, 0);

	return sent;
}
