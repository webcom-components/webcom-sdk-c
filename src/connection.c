#include <stdlib.h>
#include <libwebsockets.h>

#include "webcom-c/webcom-cnx.h"
#include "webcom-c/webcom-parser.h"

typedef struct wc_cnx {
	struct lws_context_creation_info ctxnfo;
	struct lws_context *context;
	struct lws_client_connect_info cnxnfo;
	wc_on_event_cb_t callback;
	struct lws *web_socket;
	void *user;
} wc_cnx_t;

#define WC_RX_BUFFER_BYTES	4096


static int _wc_lws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
	wc_cnx_t *cnx = (wc_cnx_t *) user;
	wc_msg_t *msg;
	wc_parser_t *parser;

	switch(reason)
	{
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			lws_callback_on_writable( wsi );
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			lwsl_notice("received %.*s\n", (int)len, (char*)in);

			msg = malloc(sizeof(wc_msg_t));
			wc_msg_init(msg);

			parser = wc_parser_new();

			wc_parse_msg_ex(parser, (char*)in, len, msg);

			cnx->callback(WC_EVENT_ON_MSG_RECEIVED, cnx, msg, sizeof(wc_msg_t), cnx->user);

			wc_parser_free(parser);
			wc_msg_free(msg);
			free(msg);

			break;

		case LWS_CALLBACK_CLIENT_WRITEABLE:
		{

			break;
		}

		case LWS_CALLBACK_CLOSED:
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			cnx->callback(WC_EVENT_ON_CNX_CLOSED, cnx, NULL, 0, cnx->user);
			break;

		default:
			break;
	}
	return 0;
}

void wc_cnx_connect(wc_cnx_t *cnx) {
	cnx->web_socket = lws_client_connect_via_info(&cnx->cnxnfo);
}

int wc_cnx_close(wc_cnx_t *cnx) {
	lws_context_destroy(cnx->context);
	return 1;
}
void wc_service(wc_cnx_t *cnx) {
	lws_service(cnx->context, 250);
}

static struct lws_protocols protocols[] =
{
	{
		"webcom-protocol",
		_wc_lws_callback,
		0,
		WC_RX_BUFFER_BYTES,
		0,
		0,
		0
	},
	{ NULL, NULL, 0, 0, 0, 0, 0}
};

wc_cnx_t *wc_cnx_new(char *endpoint, int port, char *path, wc_on_event_cb_t callback, void *user) {
	wc_cnx_t *res;

	res = calloc(1, sizeof(wc_cnx_t));

	if (res == NULL) goto end;

	res->ctxnfo.port = CONTEXT_PORT_NO_LISTEN;
	res->ctxnfo.protocols = protocols;
	res->ctxnfo.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	res->ctxnfo.gid = -1;
	res->ctxnfo.uid = -1;

	res->context = lws_create_context(&res->ctxnfo);

	res->cnxnfo.context = res->context;
	res->cnxnfo.address = strdup(endpoint);
	res->cnxnfo.port = port;
	/* FIXME: DO NOT ACCEPT BOGUS SSL BY DEFAULT!! */
	res->cnxnfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_EXPIRED | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
	res->cnxnfo.path = strdup(path);
	res->cnxnfo.host = lws_canonical_hostname(res->context);
	res->cnxnfo.protocol = protocols[0].name;
	res->cnxnfo.ietf_version_or_minus_one = -1;
	res->cnxnfo.userdata = (void *)res;
	res->user = user;
	res->callback = callback;

end:
	return res;
}

void wc_cnx_free(wc_cnx_t *cnx) {
	free((char*)cnx->cnxnfo.address);
	free((char*)cnx->cnxnfo.path);
	lws_context_destroy(cnx->context);
	free(cnx);
}
