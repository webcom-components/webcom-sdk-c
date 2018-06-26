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

ev_timer ev_handshake;

static void on_connected(wc_context_t *ctx) {
	ev_timer_stop(EV_DEFAULT, &ev_handshake);
	STFU_TRUE("The sever sent the handshake", 1);

	wc_datasync_close_cnx(ctx);

}
static int on_disconnected(wc_context_t *ctx) {
	STFU_TRUE("The connection was closed", 1);
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
	wc_context_destroy(cnx1);

end:
	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
