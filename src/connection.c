#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <nopoll.h>

#include "webcom_priv.h"
#include "webcom-c/webcom-parser.h"
#include "webcom-c/webcom-req.h"

#define WEBCOM_PROTOCOL_VERSION "5"
#define WEBCOM_WS_PATH "/_wss/.ws"


int wc_process_incoming_message(wc_cnx_t *cnx, wc_msg_t *msg) {
	wc_action_trans_t *trans;

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


		if ((trans = wc_req_get_pending(msg->u.data.u.response.r)) != NULL) {
			if (trans->callback != NULL) {
				trans->callback(
						trans->cnx,
						trans->id,
						trans->type,
						strcmp(msg->u.data.u.response.status, "ok") == 0 ? WC_REQ_OK : WC_REQ_ERROR,
						msg->u.data.u.response.status);
			}
			free(trans);
		}
	}
	cnx->callback(WC_EVENT_ON_MSG_RECEIVED, cnx, msg, sizeof(wc_msg_t), cnx->user);
	return 1;
}

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
			ret = wc_process_incoming_message(cnx, &msg);
			wc_parser_free(cnx->parser);
			cnx->parser = NULL;
			wc_msg_free(&msg);
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
	free(jsonstr);

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

static wc_cnx_t *wc_cnx_new_ex(char *proxy_host, uint16_t proxy_port, char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user) {
	wc_cnx_t *res;
	char sport[6];
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *ai, *pai;
	int nread, s;
	noPollConnOpts *npopts;
	char *get_path;
	char *sock_host;
	uint16_t sock_port;

	sock_host = proxy_host == NULL ? host : proxy_host;
	sock_port = proxy_host == NULL ? port : proxy_port;

	res = calloc(1, sizeof(wc_cnx_t));

	if (res == NULL) {
		goto error1;
	}

	res->np_ctx = nopoll_ctx_new();

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	snprintf(sport, 6, "%hu", sock_port);
	sport[5] = '\0';

	s = getaddrinfo(sock_host, sport, &hints, &ai);

	if (s != 0) {
		goto error2;
	}

	for (pai = ai ; pai != NULL ; pai = pai->ai_next) {
		if((sockfd = socket(pai->ai_family, pai->ai_socktype, 0)) < 0) {
			continue;
		}

		if (connect(sockfd, pai->ai_addr, pai->ai_addrlen) < 0) {
			close(sockfd);
			continue;
		} else {
			break;
		}
	}

	freeaddrinfo(ai);

	if(pai == NULL) {
		goto error2;
	}

	if (proxy_host != NULL) {
		dprintf(sockfd,
				"CONNECT %s:%hu HTTP/1.1\r\n"
				"Host: %s:%hu\r\n"
				"Proxy-Connection: keep-alive\r\n"
				"Connection: keep-alive\r\n\r\n",
				host, port, host, port);

		nread = read(sockfd, res->rxbuf, 13);

		/* expect "HTTP/1.? 200 *" or die */
		if (nread != 13 || strncmp(res->rxbuf, "HTTP/1.", 7) != 0 || strncmp(res->rxbuf + 8, " 200 ", 5) != 0) {
			close(sockfd);
			goto error2;
		}

		/* empty the socket read buffer */
		while (recv(sockfd, res->rxbuf, WC_RX_BUF_LEN, MSG_DONTWAIT) > 0);
	}

	npopts = nopoll_conn_opts_new();
	asprintf(&get_path, "%s?v=%s&ns=%s", WEBCOM_WS_PATH, WEBCOM_PROTOCOL_VERSION, application);
	snprintf(sport, 6, "%hu", port);
	sport[5] = '\0';
	res->np_conn = nopoll_conn_tls_new_with_socket(res->np_ctx, npopts, sockfd, host, sport, NULL, get_path, NULL, NULL);
	free(get_path);

	if (nopoll_conn_is_ok(res->np_conn)) {
		res->state = WC_CNX_STATE_CREATED;
		res->user = user;
		res->callback = callback;
		res->fd = nopoll_conn_socket(res->np_conn);
		nopoll_conn_wait_until_connection_ready(res->np_conn, 2);
		res->state = WC_CNX_STATE_READY;
		callback(WC_EVENT_ON_CNX_ESTABLISHED, res, NULL, 0, user);
	} else {
		goto error3;
	}

	return res;
error3:
	free(get_path);
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
	wc_on_data_handler_t *p, *q;

	nopoll_ctx_unref(cnx->np_ctx);
	if (cnx->parser != NULL) wc_parser_free(cnx->parser);

	p = cnx->handlers;
	while (p != NULL) {
		free(p->path);
		q = p;
		p = p->next;
		free(q);
	}
	free(cnx);
}

int wc_cnx_keepalive(wc_cnx_t *cnx) {
	int sent = 0;
	static char keepalive = '0';

	sent = nopoll_conn_send_text(cnx->np_conn, &keepalive, 1);
	nopoll_conn_flush_writes(cnx->np_conn, 2000, 0);

	return sent;
}
