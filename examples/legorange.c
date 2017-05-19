#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdio_ext.h>
#include <string.h>
#include <webcom-c/webcom.h>
#include <ev.h>

static inline void clear_screen() {
	printf("\033[2J");
}

void draw_brick(unsigned x, unsigned y, char color) {
	printf("\033[%u;%uf\033[%dm%s\033[0m", y + 1, 2*x + 1, (int) color, color ? " ●" : "  ");
	fflush(stdout);
}

void on_brick_update(char *key, json_object *data) {
	int x, y;
	json_object *jcolor;
	char *scolor;
	char ccolor = 37;

	if (sscanf(key, "%d-%d", &x, &y) == 2) {
		if (json_object_is_type(data, json_type_null)) {
			draw_brick(x, y, 0);
		} else {
			if (json_object_object_get_ex(data, "color", &jcolor)) {
				scolor = json_object_get_string(jcolor);
				if (strcmp("green", scolor) == 0) {
					ccolor = 32;
				} else if (strcmp("red", scolor) == 0) {
					ccolor = 31;
				} else if (strcmp("darkgrey", scolor) == 0) {

				} else if (strcmp("blue", scolor) == 0) {
					ccolor = 34;
				} else if (strcmp("yellow", scolor) == 0) {
					ccolor = 33;
				} else if (strcmp("brown", scolor) == 0) {
					ccolor = 31;
				}
				draw_brick(x, y, ccolor);
			}
		}
	}
}

void on_data_update_update_put(wc_push_data_update_put_t *event) {
	char *key;
	struct json_object *val;
	if (strcmp("/brick", event->path) == 0 && event->data != NULL) {
		json_object_object_foreach(event->data, key, val) {
			on_brick_update(key, val);
		}
	} else if (strncmp("/brick/", event->path, 7) == 0){
		on_brick_update(event->path + 7, event->data);
	}
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
			/* when the connection is established, register to events in the
			 * "/brick" path
			 */
			wc_listen(cnx, "/brick");
			clear_screen();
			break;
		case WC_EVENT_ON_MSG_RECEIVED:
			if (msg->type == WC_MSG_DATA
					&& msg->u.data.type == WC_DATA_MSG_PUSH
					&& msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_PUT)
			{
				on_data_update_update_put(&msg->u.data.u.push.u.update_put);
			}
			break;
		case WC_EVENT_ON_CNX_CLOSED:
			clear_screen();
			puts("Closed by remote, Bye");
			ev_break(EV_A_ EVBREAK_ALL);
			break;
		default:
			break;
	}
}

/*
 * libev callback that captures user input on stdin and sends put messages to
 * the webcom server if the input matches "x y color"
 */
void stdin_cb (EV_P_ ev_io *w, int revents) {
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
		clear_screen();
		puts("Bye");
		ev_io_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
	}
}

/* call webcom API when the webcom connection becomes readable */
void webcom_socket_cb(EV_P_ ev_io *w, int revents) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	if(revents&EV_READ) {
		wc_cnx_on_readable(cnx);
	}
}

/* send keepalives to the webcom server periodically */
void ka_cb(EV_P_ ev_timer *w, int revents) {
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
			loop);

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
