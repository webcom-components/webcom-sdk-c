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

#include <stdio.h>
#include <string.h>
#include <ev.h>
#include <webcom-c/webcom.h>
#include <webcom-c/webcom-libev.h>
#include <inttypes.h>
#include <getopt.h>

#include "stfu.h"

char *email = "user@test.org";
char *password = "s3cr3t";
char *read_path = NULL;

static void on_connected(wc_context_t *ctx, int initial_connection) {
	STFU_TRUE("Connected", 1);
	wc_auth_with_password(ctx, email, password);

}
static int on_disconnected(wc_context_t *ctx) {
	STFU_TRUE("The connection was closed", 1);

	return 0;
}

static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len) {
	STFU_TRUE("Connection error", 0);
	printf("\t%.*s\n", error_len, error);
	return 0;
}

void on_data(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	STFU_TRUE("Successfully received the test data", 1);
	printf("\t%s\n", json_data);
	ev_break(EV_DEFAULT, EVBREAK_ALL);
}

void wc_on_ws_listen_response(wc_context_t *ctx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data) {
	if (status == WC_REQ_OK) {
		STFU_TRUE("Successfully listened to the test path", 1);
	} else {
		STFU_TRUE("Failed to listen to the test path", 0);
		printf("\t%s\n", reason);
		ev_break(EV_DEFAULT, EVBREAK_ALL);
	}
}

void wc_on_ws_auth_response(wc_context_t *ctx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data) {
	if (status == WC_REQ_OK) {
		STFU_TRUE("Successful authentication on the datasync websocket", 1);
		if (read_path) {
			wc_on_data(ctx, read_path, on_data, NULL);
			wc_req_listen(ctx, NULL, read_path);
		} else {
			ev_break(EV_DEFAULT, EVBREAK_ALL);
		}
	} else {
		STFU_TRUE("Failed authentication on the datasync websocket", 0);
		printf("\t%s\n", reason);
		ev_break(EV_DEFAULT, EVBREAK_ALL);
	}
}

void on_auth_success(wc_context_t *ctx, struct wc_auth_info* ai) {
	STFU_TRUE("Auth token received", ai->token);
	printf("\t%.25s...\n", ai->token);
	STFU_TRUE("Auth UID received", ai->uid);
	printf("\t%s\n", ai->uid);
	STFU_TRUE("Auth provider received", ai->provider);
	printf("\t%s\n", ai->provider);
	STFU_TRUE("Auth Provider UID received", ai->provider_uid);
	printf("\t%s\n", ai->provider_uid);
	if (ai->provider_profile) {
		STFU_TRUE("Auth Provider Profile received", ai->provider_profile);
		printf("\t%s\n", ai->provider_profile);
	} else {
		STFU_TRUE("No Provider Profile", 1);
	}
	STFU_TRUE("Auth expires received", ai->expires != 0);
	printf("\t%"PRIu64"\n", ai->expires);

	wc_req_auth(ctx, wc_on_ws_auth_response, ai->token);
}

void on_auth_error(wc_context_t *ctx, char* error) {
	STFU_TRUE("Auth error received", error);
	printf("\t%s...\n", error);

	ev_break(EV_DEFAULT, EVBREAK_ALL);
}

int main(int argc, char **argv) {
	struct ev_loop *loop = EV_DEFAULT;
	wc_context_t *ctx;
	ev_io ev_wc_readable;
	char *server = "io.datasync.orange.com";
	char *app = "test";
	uint16_t port = 443;
	int c;

	while((c = getopt(argc, argv, "s:p:a:E:P:vr:")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = (uint16_t)atoi(optarg);
			break;
		case 'a':
			app = optarg;
			break;
		case 'E':
			email = optarg;
			break;
		case 'P':
			password = optarg;
			break;
		case 'v':
			wc_set_log_level(WC_LOG_ALL, WC_LOG_INFO);
			wc_set_log_level(WC_LOG_AUTH, WC_LOG_EXTRADEBUG);
			break;
		case 'r':
			read_path = optarg;
			break;
		}
	}

	struct wc_eli_callbacks cb = {
			.on_connected = on_connected,
			.on_disconnected = on_disconnected,
			.on_error = on_error,
			.on_auth_success = on_auth_success,
			.on_auth_error = on_auth_error,
	};

	STFU_TRUE	("Establish a new Connection",
			ctx = wc_context_new_with_libev(server, port, app, loop, &cb)
			);
	if (ctx == NULL) goto end;

	ev_run(loop, 0);

end:
	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
