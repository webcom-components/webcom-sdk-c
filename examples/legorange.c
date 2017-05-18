#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdio_ext.h>
#include <string.h>
#include <webcom-c/webcom.h>
#include <ev.h>

/* This callback is called by the webcom SDK when events such as "connection is
 * ready", "connection was closed", or "incoming webcom message" occur on the
 * webcom connection.
 */
static void webcom_service_cb(wc_event_t event, wc_cnx_t *cnx, void *data,
		size_t len, void *user)
{
	wc_msg_t *msg = (wc_msg_t*) data;
	switch (event) {
		case WC_EVENT_ON_CNX_ESTABLISHED:
			/* when the connection is established, register to events in the
			 * "/brick" path
			 */
			wc_listen(cnx, "/brick");
			break;
		case WC_EVENT_ON_MSG_RECEIVED:
			if (msg->type == WC_MSG_CTRL
					&& msg->u.ctrl.type == WC_CTRL_MSG_HANDSHAKE)
			{
				printf("Got server handshake: server [%s], timestamp [%ld], "
						"version [%s]\n",
						msg->u.ctrl.u.handshake.server,
						msg->u.ctrl.u.handshake.ts,
						msg->u.ctrl.u.handshake.version);
			} else if (msg->type == WC_MSG_DATA
					&& msg->u.data.type == WC_DATA_MSG_PUSH
					&& msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_PUT)
			{
				printf("Got server data update push: path [%s], data\n%s\n",
						msg->u.data.u.push.u.update_put.path,
						json_object_to_json_string_ext(
								msg->u.data.u.push.u.update_put.data,
								JSON_C_TO_STRING_PRETTY));
			}
			break;
		case WC_EVENT_ON_CNX_CLOSED:
			break;
		default:
			break;
	}
}

/*
 * libev callback that captures user input on stdin and sends put messages to
 * the webcom server if the input matches "x y color"
 */
static void stdin_cb (EV_P_ ev_io *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;
	static char buf[2048];
	int x, y, col;
	char *col_str, *path, *data, nl;
	if (fgets(buf, sizeof(buf), stdin)) {
		if(sscanf(buf, "%d %d %d%c", &x, &y, &col, &nl) == 4) {
			col_str = col ? "white" : "black";

			/* format the path and the data */
			asprintf(&path, "/brick/%d-%d", x, y);
			asprintf(&data, "{\"color\":\"%s\",\"uid\":\"anonymous\",\"x\":%d,"
					"\"y\":%d}", col_str, x, y);

			/* send the put message to the webcom server */
			if (wc_put_json_data(cnx, path, data) > 0) {
				puts("OK");
			} else {
				puts("ERROR");
			}

			free(path);
			free(data);
		}
	}
	if (feof(stdin)) {
		puts("Bye");
		ev_io_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
	}
}

/* call webcom API when the webcom connection becomes readable */
static void webcom_socket_cb(EV_P_ ev_io *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	if(revents&EV_READ) {
		wc_cnx_on_readable(cnx);
	}
}

/* send keepalives to the webcom server periodically */
static void ka_cb(EV_P_ ev_timer *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	wc_cnx_keepalive(cnx);
}

int main(void) {
	wc_cnx_t *cnx;
	int wc_fd;
	struct ev_loop *loop = EV_DEFAULT;
	ev_io stdin_watcher;
	ev_io webcom_watcher;
	ev_timer ka_timer;

	cnx = wc_cnx_new(
			"io.datasync.orange.com",
			443,
			"/_wss/.ws?v=5&ns=legorange",
			webcom_service_cb,
			NULL);

	wc_fd = wc_cnx_get_fd(cnx);

	ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
	stdin_watcher.data = cnx;
	ev_io_start (loop, &stdin_watcher);

	ev_io_init(&webcom_watcher, webcom_socket_cb, wc_fd, EV_READ);
	webcom_watcher.data = cnx;
	ev_io_start (loop, &webcom_watcher);

	ev_timer_init(&ka_timer, ka_cb, 45, 45);
	ka_timer.data = cnx;
	ev_timer_start(loop, &ka_timer);

    ev_run(loop, 0);

    return 0;
}
