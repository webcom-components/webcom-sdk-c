#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
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
			if (*cnx->rxbuf == '{') {
				cnx->parser = wc_parser_new();
			} else {
				return 1;
			}
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
	} else if (n == -1) {
		if (nopoll_conn_is_ok(cnx->np_conn) == nopoll_false) {
			cnx->state = WC_CNX_STATE_CLOSED;
			cnx->callback(WC_EVENT_ON_CNX_CLOSED, cnx, NULL, 0, cnx->user);
		}
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
	cnx->state = WC_CNX_STATE_CLOSED;
	nopoll_conn_close(cnx->np_conn);
	cnx->callback(WC_EVENT_ON_CNX_CLOSED, cnx, NULL, 0, cnx->user);
}

int wc_cnx_get_fd(wc_cnx_t *cnx) {
	return cnx->fd;
}

wc_cnx_t *wc_cnx_new_with_proxy(char *proxy_host, uint16_t proxy_port, char *endpoint, uint16_t port, char *path, wc_on_event_cb_t callback, void *user) {
	wc_cnx_t *res;
	char sport[6];
	int proxy_socket;
	struct hostent *proxy_hostent;
	struct sockaddr_in proxy_serveraddr;
	int nread;
	noPollConnOpts *npopts;

	res = calloc(1, sizeof(wc_cnx_t));

	if (res == NULL) {
		goto error1;
	}

	res->np_ctx = nopoll_ctx_new();

	snprintf(sport, 6, "%hu", port);

	sport[5] = '\0';

	if((proxy_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		goto error2;
	}

	proxy_hostent = gethostbyname(proxy_host);
	if (proxy_hostent == NULL) {
		goto error2;
	}

	/* build the server's Internet address */
	bzero((char *) &proxy_serveraddr, sizeof(proxy_serveraddr));
	proxy_serveraddr.sin_family = AF_INET;
	bcopy((char *)proxy_hostent->h_addr,
			(char *)&proxy_serveraddr.sin_addr.s_addr, proxy_hostent->h_length);
	proxy_serveraddr.sin_port = htons(proxy_port);

	if (connect(proxy_socket, (struct sockaddr*)&proxy_serveraddr, sizeof(proxy_serveraddr)) < 0) {
		goto error2;
	}

	dprintf(proxy_socket,
			"CONNECT %s:%hu HTTP/1.1\r\n"
			"Host: %s:%hu\r\n"
			"Proxy-Connection: keep-alive\r\n"
			"Connection: keep-alive\r\n\r\n",
			endpoint, port, endpoint, port);

	nread = read(proxy_socket, res->rxbuf, 13);

	/* expect "HTTP/1.? 200 *" or die */
	if (nread != 13 || strncmp(res->rxbuf + 8, " 200 ", 5) != 0) {
		close(proxy_socket);
		goto error2;
	}

	/* empty the socket read buffer */
	while (recv(proxy_socket, res->rxbuf, WC_RX_BUF_LEN, MSG_DONTWAIT) > 0);

	npopts = nopoll_conn_opts_new();
	res->np_conn = nopoll_conn_tls_new_with_socket(res->np_ctx, npopts, proxy_socket, endpoint, sport, NULL, path, NULL, NULL);
	nopoll_conn_opts_free(npopts);

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

wc_cnx_t *wc_cnx_new(char *host, uint16_t port, char *path, wc_on_event_cb_t callback, void *user) {
	wc_cnx_t *res;
	char sport[6];

	res = calloc(1, sizeof(wc_cnx_t));

	if (res == NULL) {
		goto error1;
	}

	res->np_ctx = nopoll_ctx_new();

	snprintf(sport, 6, "%hu", port);

	sport[5] = '\0';
	res->np_conn = nopoll_conn_tls_new(res->np_ctx, 0, host, sport, NULL, path, NULL, NULL);
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
