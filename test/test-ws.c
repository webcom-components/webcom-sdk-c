#include <libwebsockets.h>
#include <json-c/json.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static struct lws *web_socket = NULL;

#define EXAMPLE_RX_BUFFER_BYTES (1024)

static int callback_example(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	switch( reason )
	{
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			lws_callback_on_writable( wsi );
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			lwsl_notice("received %.*s\n", len, (char*)in);
			json_tokener* tok = json_tokener_new();
			struct json_object_iterator it, ite;

			struct json_object *rootr = json_tokener_parse_ex(tok, (char *)in, len);
			it = json_object_iter_begin(rootr);
			ite = json_object_iter_end(rootr);

			for ( ; !json_object_iter_equal(&it, &ite) ; json_object_iter_next(&it)) {
				lwsl_notice("Parsed: %s => [%d] %p\n", json_object_iter_peek_name(&it), json_object_get_type(json_object_iter_peek_value(&it)), json_object_iter_peek_value(&it));

			}

			json_tokener_free(tok);
			break;

		case LWS_CALLBACK_CLIENT_WRITEABLE:
		{
			unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
			unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
			size_t n = sprintf( (char *)p, "%u", rand() );
			lwsl_notice("send %.*s\n", n, p);
			struct json_object *root = json_object_new_object();
			struct json_object *leaf = json_object_new_string_len(p, n);
			json_object_object_add(root, "plop", leaf);
			n = sprintf( (char *)p, "%s", json_object_to_json_string(root));
			json_object_put(root);
			lws_write( wsi, p, n, LWS_WRITE_TEXT );

			break;
		}

		case LWS_CALLBACK_CLOSED:
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			web_socket = NULL;
			break;

		default:
			break;
	}

	return 0;
}

static struct lws_protocols protocols[] =
{
	{
		"echo-protocol",
		callback_example,
		0,
		EXAMPLE_RX_BUFFER_BYTES,
	},
	{ NULL, NULL, 0, 0 }
};

int main(int argc, char *argv[])
{

	struct lws_context_creation_info ctxnfo;
	struct lws_client_connect_info cnxnfo;

	memset(&ctxnfo, 0, sizeof(ctxnfo));
	memset(&cnxnfo, 0, sizeof(cnxnfo));

	ctxnfo.port = CONTEXT_PORT_NO_LISTEN;
	ctxnfo.protocols = protocols;
	ctxnfo.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	ctxnfo.gid = -1;
	ctxnfo.uid = -1;

	struct lws_context *context = lws_create_context( &ctxnfo );

	time_t old = 0;
	while( 1 )
	{
		struct timeval tv;
		gettimeofday( &tv, NULL );

		/* Connect if we are not connected to the server. */
		if( !web_socket && tv.tv_sec != old )
		{
			cnxnfo.context = context;
			cnxnfo.address = "io.datasync.orange.com";
			cnxnfo.port = 443;
			cnxnfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_EXPIRED | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
			cnxnfo.path = "/_wss/.ws?v=5&ns=legorange";
			cnxnfo.host = lws_canonical_hostname(context);
			cnxnfo.protocol = protocols[0].name;
			cnxnfo.ietf_version_or_minus_one = -1;

			web_socket = lws_client_connect_via_info(&cnxnfo);
		}

		if( tv.tv_sec != old )
		{
			/* Send a random number to the server every second. */
			lws_callback_on_writable( web_socket );
			old = tv.tv_sec;
		}

		lws_service( context, 250 );
	}

	lws_context_destroy( context );

return 0;
}

