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
#include <inttypes.h>

#include "stfu.h"

ev_timer ev_handshake, ev_listen, ev_on_update, ev_disc;

static void on_update_fail_cb(EV_P_ ev_timer *w, UNUSED_PARAM(int revents)) {
	STFU_TRUE("We got a data update of the path we're listening to", 0);
	ev_break(EV_A_ EVBREAK_ALL);
}

int got_update = 0;

void on_listen_result(wc_cnx_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data) {
	ev_timer_stop(EV_DEFAULT, &ev_listen);
	if (status == WC_REQ_OK) {
		printf("\tthe listen request succeeded with status '%s', opt data: %s\n", reason, data);

		if(!got_update) {
			ev_timer_init(&ev_on_update, on_update_fail_cb, 5, 0);
			ev_on_update.data = (void *)cnx;
			ev_timer_start(EV_DEFAULT, &ev_on_update);
		}

		wc_req_put(cnx, NULL, "/brick/0-0", "{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":0,\"y\":0}");
		wc_req_put(cnx, NULL, "/brick/0-0", "{\"color\":\"green\",\"uid\":\"anonymous\",\"x\":0,\"y\":0}");

	} else {
		printf("\nthe listen request failed with status '%s'\n", reason);
		ev_break(EV_DEFAULT, EVBREAK_ALL);
	}
	STFU_TRUE("We got a response to the 'listen' request", 1);
}

static void disc_cb(EV_P_ ev_timer *w, UNUSED_PARAM(int revents)) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	puts("\tnow closing the connection");
	fflush(stdout);

	wc_cnx_close(cnx);
}

void on_brick_1_1_data(wc_cnx_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
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

void test_cb(wc_event_t event, wc_cnx_t *cnx, void *data, UNUSED_PARAM(size_t len), void *user) {
	struct ev_loop *loop = user;
	wc_msg_t *msg = (wc_msg_t*) data;
	switch (event) {
	case WC_EVENT_ON_SERVER_HANDSHAKE:
		ev_timer_stop(loop, &ev_handshake);
		printf("\tserver [%s], version [%s], timestamp[%"PRId64"]\n", msg->u.ctrl.u.handshake.server, msg->u.ctrl.u.handshake.version, msg->u.ctrl.u.handshake.ts);
		STFU_TRUE("The sever sent the handshake", 1);

		wc_on_data(cnx, "/brick/0-0", on_brick_1_1_data, loop);
		wc_req_listen(cnx, on_listen_result, "/brick/0-0");

		ev_timer_init(&ev_listen, listen_fail_cb, 5, 0);
		ev_listen.data = (void *)cnx;
		ev_timer_start(loop, &ev_listen);

		break;
	case WC_EVENT_ON_CNX_CLOSED:
		STFU_TRUE("The connection was closed", 1);
		wc_cnx_free(cnx);
		ev_break(EV_DEFAULT, EVBREAK_ALL);
		break;
	default:
		break;
	}
}

static void readable_cb (EV_P_ ev_io *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;
	wc_cnx_on_readable(cnx);
}

static void handshake_fail_cb(EV_P_ ev_timer *w, UNUSED_PARAM(int revents)) {
	STFU_TRUE("The sever sent the handshake", 0);
	ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
	struct ev_loop *loop = EV_DEFAULT;
	wc_cnx_t *cnx1;
	ev_io ev_wc_readable;
	int fd;


	STFU_TRUE	("Establish a new Connection",
			cnx1 = (wc_cnx_new("io.datasync.orange.com", 443, "legorange", test_cb, (void *)loop))
			);
	if (cnx1 == NULL) goto end;

	fd = wc_cnx_get_fd(cnx1);
	printf("\tfd = %d\n", fd);
	STFU_TRUE   ("Obtained the websocket file descriptor", fd >= 0);

	puts("\tprocessing the event loop...");
	ev_io_init(&ev_wc_readable, readable_cb, fd, EV_READ);
	ev_wc_readable.data = (void *)cnx1;
	ev_io_start(loop, &ev_wc_readable);

	ev_timer_init(&ev_handshake, handshake_fail_cb, 5, 0);
	ev_handshake.data = (void *)cnx1;
	ev_timer_start(loop, &ev_handshake);


	ev_run(loop, 0);

end:
	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
