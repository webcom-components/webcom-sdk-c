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

#include "stfu.h"

ev_timer ev_handshake, ev_listen, ev_on_update, ev_disc;

static void on_update_fail_cb(EV_P_ ev_timer *w, UNUSED_PARAM(int revents)) {
	STFU_TRUE("We got a data update of the path we're listening to", 0);
	ev_break(EV_A_ EVBREAK_ALL);
}

int got_update = 0;

void on_listen_result(wc_context_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data) {
	ev_timer_stop(EV_DEFAULT, &ev_listen);
	if (status == WC_REQ_OK) {
		printf("\tthe listen request succeeded with status '%s', opt data: %s\n", reason, data);

		if(!got_update) {
			ev_timer_init(&ev_on_update, on_update_fail_cb, 5, 0);
			ev_on_update.data = (void *)cnx;
			ev_timer_start(EV_DEFAULT, &ev_on_update);
		}

		wc_datasync_put(cnx, "/brick/0-0", "{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":0,\"y\":0}", NULL);
		wc_datasync_put(cnx, "/brick/0-0", "{\"color\":\"green\",\"uid\":\"anonymous\",\"x\":0,\"y\":0}", NULL);

	} else {
		printf("\nthe listen request failed with status '%s'\n", reason);
		ev_break(EV_DEFAULT, EVBREAK_ALL);
	}
	STFU_TRUE("We got a response to the 'listen' request", 1);
}

static void disc_cb(EV_P_ ev_timer *w, UNUSED_PARAM(int revents)) {
	wc_context_t *cnx = (wc_context_t *)w->data;

	puts("\tnow closing the connection");
	fflush(stdout);

	wc_datasync_close_cnx(cnx);
}

void on_brick_1_1_data(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	if(strcmp(path, "/brick/0-0") == 0) {
		printf("\t%s => %s\n", path, json_data);

		if (!got_update) {
			STFU_TRUE("We got a data update of the path we're listening to", 1);
			ev_timer_stop(EV_DEFAULT, &ev_on_update);

			got_update = 1;

			ev_timer_init(&ev_disc, disc_cb, 5, 0);
			ev_disc.data = (void *)cnx;
			ev_timer_start(EV_DEFAULT, &ev_disc);
		}
	}
}

static void listen_fail_cb(EV_P_ ev_timer *w, UNUSED_PARAM(int revents)) {
	STFU_TRUE("We got a response to the 'listen' request", 0);
	ev_break(EV_A_ EVBREAK_ALL);
}

static void on_connected(wc_context_t *ctx) {
	ev_timer_stop(EV_DEFAULT, &ev_handshake);
	STFU_TRUE("The sever sent the handshake", 1);

	wc_datasync_route_data(ctx, "/brick/0-0", on_brick_1_1_data, EV_DEFAULT);
	wc_datasync_listen(ctx, "/brick/0-0", on_listen_result);

	ev_timer_init(&ev_listen, listen_fail_cb, 5, 0);
	ev_timer_start(EV_DEFAULT, &ev_listen);
}
static int on_disconnected(wc_context_t *ctx) {
	STFU_TRUE("The connection was closed", 1);
	wc_context_destroy(ctx);
	ev_break(EV_DEFAULT, EVBREAK_ALL);
	return 0;
}

static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len) {
	STFU_TRUE("Connection error", 0);
	printf("\terror: %.*s", error_len, error);
	return 0;
}

static void handshake_fail_cb(EV_P_ ev_timer *w, UNUSED_PARAM(int revents)) {
	STFU_TRUE("The sever sent the handshake", 0);
	ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
	struct ev_loop *loop = EV_DEFAULT;
	wc_context_t *cnx1;
	ev_io ev_wc_readable;

	struct wc_eli_callbacks cb = {
			.on_connected = on_connected,
			.on_disconnected = on_disconnected,
			.on_error = on_error,
	};

	struct wc_context_options options = {
			.host = "io.datasync.orange.com",
			.port = 443,
			.app_name = "legorange"
	};

	STFU_TRUE	("Establish a new Connection",
			cnx1 = wc_context_create_with_libev(&options, loop, &cb)
			);
	if (cnx1 == NULL) goto end;

	wc_datasync_init(cnx1);
	wc_datasync_connect(cnx1);

	puts("\tprocessing the event loop...");

	ev_timer_init(&ev_handshake, handshake_fail_cb, 5, 0);
	ev_timer_start(loop, &ev_handshake);


	ev_run(loop, 0);

end:
	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
