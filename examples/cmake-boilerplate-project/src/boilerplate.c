#include <webcom-c/webcom.h>
#include <unistd.h>
#include <ev.h>
#include <stdio.h>
#include <json-c/json.h>

void keepalive_cb(EV_P_ ev_timer *w, int revents);
void webcom_socket_cb(EV_P_ ev_io *w, int revents);
void stdin_cb (EV_P_ ev_io *w, int revents);
void webcom_service_cb(wc_event_t event, wc_cnx_t *cnx, void *data,
		size_t len, void *user);

int main(int argc, char *argv[]) {
	wc_cnx_t *cnx;
	int wc_fd;
	struct ev_loop *loop = EV_DEFAULT;
	ev_io stdin_watcher;
	ev_io webcom_watcher;
	ev_timer ka_timer;

	cnx = wc_cnx_new(
			"io.datasync.orange.com",
			443,
			"<your app name here>",
			webcom_service_cb, /* this is the callback that gets called
			when a webcom-level event occurs */
			loop);

	if (cnx == NULL) {
		return 1;
	}

	/* get the raw file descriptor of the webcom connection: we need it for
	 * the event loop
	 */
	wc_fd = wc_cnx_get_fd(cnx);

	/* let's define 3 events to listen to:
	 *
	 * first, if the webcom file descriptor is readable, call
	 * webcom_socket_cb() */
	ev_io_init(&webcom_watcher, webcom_socket_cb, wc_fd, EV_READ);
	webcom_watcher.data = cnx;
	ev_io_start (loop, &webcom_watcher);

	/* then on a 45s recurring timer, call keepalive_cb() to keep the webcom
	 * connection alive */
	ev_timer_init(&ka_timer, keepalive_cb, 45, 45);
	ka_timer.data = cnx;
	ev_timer_start(loop, &ka_timer);

	/* finally, if stdin has data to read, call stdin_watcher() */
	ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
	stdin_watcher.data = cnx;
	ev_io_start (loop, &stdin_watcher);

	/* enter the event loop */
	ev_run(loop, 0);

	return 0;
}

/* called by libev on read event on the webcom TCP socket */
void webcom_socket_cb(EV_P_ ev_io *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	wc_cnx_on_readable(cnx);

}

/* This callback is called by the webcom SDK when events such as "connection is
 * ready", "connection was closed", or "incoming webcom message" occur on the
 * webcom connection.
 */
void webcom_service_cb(wc_event_t event, wc_cnx_t *cnx, void *data,
		size_t len, void *user)
{
	wc_msg_t *msg = (wc_msg_t*) data;
	struct ev_loop *loop = (struct ev_loop*) user;

	switch (event) {
		case WC_EVENT_ON_CNX_ESTABLISHED:
			/* called once the connection is successfully established, you
			 * could start listening to some data path(s) here, e.g.:
			 *
			 * wc_listen(cnx, "/your/path/here");
			 *
			 */
			break;
		case WC_EVENT_ON_MSG_RECEIVED:
			/* do something depending on the content on the webcom message
			 * available as ̀`msg`, e.g.:
			 *
			 * if (msg->type == WC_MSG_DATA
			 *     && msg->u.data.type == WC_DATA_MSG_PUSH
			 *     && msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_PUT)
			 * {
			 *     do_something_with_the(msg->u.data.u.push.u.update_put.data);
			 * }
			 */
			break;
		case WC_EVENT_ON_CNX_CLOSED:
			ev_break(EV_A_ EVBREAK_ALL);
			wc_cnx_free(cnx);
			break;
		default:
			break;
	}
}


/* libev timer callback, to send keepalives to the webcom server periodically
 */
void keepalive_cb(EV_P_ ev_timer *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	wc_cnx_keepalive(cnx);
}

/*
 * libev callback when data is available on stdin
 */
void stdin_cb (EV_P_ ev_io *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;
	static char buf[2048];

	if (fgets(buf, sizeof(buf), stdin) != NULL) {
		/* do stuff */
	}
	if (feof(stdin)) {
		/* quit if EOF on stdin */
		ev_io_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
		wc_cnx_close(cnx);
	}
}
