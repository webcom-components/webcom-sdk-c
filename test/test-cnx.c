#include <stdio.h>
#include <stdio_ext.h>
#include <string.h>
#include <ev.h>
#include <webcom-c/webcom.h>

#include "stfu.h"



void test_cb(wc_event_t event, wc_cnx_t *cnx, void *data, size_t len, void *user) {
	wc_msg_t *msg = (wc_msg_t*) data;
	switch (event) {
		case WC_EVENT_ON_CNX_ESTABLISHED:
			puts("ESTABLISHED");

			break;
		case WC_EVENT_ON_MSG_RECEIVED:
			STFU_TRUE	("Receive handshake", msg->type == WC_MSG_CTRL && msg->u.ctrl.type == WC_CTRL_MSG_HANDSHAKE);
			STFU_TRUE	("Handshake path exists", msg->u.ctrl.u.handshake.server != NULL);
			printf("\t%s\n", msg->u.ctrl.u.handshake.server);
			break;
		case WC_EVENT_ON_CNX_CLOSED:
			puts("\tCNX CLOSED");
			wc_cnx_free(cnx);
			break;
		default:
			break;
	}
}

static void readable_cb (EV_P_ ev_io *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;
	printf ("\tfd %d, ev %08x, erv %08x\n", w->fd, w->events, revents);
	if (wc_cnx_on_readable(cnx)) {
		ev_io_stop(loop, w);
	}
}

static void send_cb(EV_P_ ev_timer *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	wc_msg_t msg1;

	puts("\tSENDING...");

	memset(&msg1, 0, sizeof(wc_msg_t));
	msg1.type = WC_MSG_DATA;
	msg1.u.data.type = WC_DATA_MSG_ACTION;
	msg1.u.data.u.action.type = WC_ACTION_PUT;
	msg1.u.data.u.action.r = 0;
	msg1.u.data.u.action.u.put.path = strdup("/brick/12-12");
	msg1.u.data.u.action.u.put.data = strdup("{\"color\":\"green\",\"uid\":\"anonymous\",\"x\":12,\"y\":12}");

	STFU_TRUE	("Data was sent", wc_cnx_send_msg(cnx, &msg1) > 0);

	wc_msg_free(&msg1);
	ev_timer_stop(loop, w);
}

static void close_cb(EV_P_ ev_timer *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;
	puts("\tCLOSE");
	wc_cnx_close(cnx);
	ev_timer_stop(loop, w);
}

int main(void) {
	struct ev_loop *loop = EV_DEFAULT;
	wc_cnx_t *cnx1;
	ev_io ev_wc_readable;
	ev_timer ev_close, ev_send;



	STFU_TRUE	("Create new wc_cnx_t object", (cnx1 = wc_cnx_new("io.datasync.orange.com", 443, "legorange", test_cb, (void *)0xDEADBEEF)) != NULL);

	ev_io_init(&ev_wc_readable, readable_cb, wc_cnx_get_fd(cnx1), EV_READ);
	ev_wc_readable.data = (void *)cnx1;
	ev_io_start(loop, &ev_wc_readable);
	ev_timer_init(&ev_send, send_cb, 1, 0);
	ev_send.data = (void *)cnx1;
	ev_timer_start(loop, &ev_send);
	ev_timer_init(&ev_close, close_cb, 4, 0);
	ev_close.data = (void *)cnx1;
	ev_timer_start(loop, &ev_close);

	ev_run(loop, 0);


	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
