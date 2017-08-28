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

#define _GNU_SOURCE
#include <string.h>
#include <getopt.h>
#include <webcom-c/webcom.h>
#include <ev.h>
#include <ncurses.h>
#include <json-c/json.h>

void keepalive_cb(EV_P_ ev_timer *w, int revents);
void webcom_socket_cb(EV_P_ ev_io *w, int revents);
void stdin_cb (EV_P_ ev_io *w, int revents);
void webcom_service_cb(wc_event_t event, wc_cnx_t *cnx, void *data,
		size_t len, void *user);
void print_new_message(json_object *message);

WINDOW *chat, *chatbox;
WINDOW *input, *inputbox;
char *name = "C-SDK-demo";
const char *escaped_name;

int main(int argc, char *argv[]) {
	wc_cnx_t *cnx;
	int wc_fd;
	struct ev_loop *loop = EV_DEFAULT;
	ev_io stdin_watcher;
	ev_io webcom_watcher;
	ev_timer ka_timer;
	int opt;
	json_object *json_name;

	/* [mildly boring] set stdout unbuffered to see the bricks appear in real
	 * time */
	setbuf(stdout, NULL);

	/* [boring] parse the command line options (proxy settings, board) */
	while ((opt = getopt(argc, argv, "n:h")) != -1) {
		switch((char)opt) {
		case 'n':
			name = optarg;
			break;
		case 'h':
			printf(
					"%s [OPTIONS]\n"
					"Note: quit the chatroom by entering \"/quit\"\n"
					"Options:\n"
					"-n NICKNAME: Set your nickname in the chatroom (default: \"C-SDK-demo\")\n"
					"-h         : Displays this help message.\n",
					*argv);
			exit(0);
			break;
		}
	}

	cnx = wc_cnx_new(
			"io.datasync.orange.com",
			443,
			"chat",
			webcom_service_cb,
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
	ev_io_init(&stdin_watcher, stdin_cb, fileno(stdin), EV_READ);
	stdin_watcher.data = cnx;
	ev_io_start (loop, &stdin_watcher);

	initscr();

	keypad(stdscr, FALSE);

	chatbox = subwin(stdscr, LINES - 3, COLS, 0, 0);
	chat = derwin(chatbox, LINES - 5, COLS - 2, 1, 1);
	inputbox = subwin(stdscr, 3, COLS, LINES - 3, 0);
	input = derwin(inputbox, 1, COLS - 2, 1, 1);
	box(chatbox, ACS_VLINE, ACS_HLINE);
	box(inputbox, ACS_VLINE, ACS_HLINE);
	scrollok(chat, TRUE);
	wrefresh(stdscr);

	json_name = json_object_new_string(name);
	escaped_name = json_object_to_json_string(json_name);

	ev_run(loop, 0);

	endwin();

	json_object_put(json_name);
	return 0;
}

/* called by libev on read event on the webcom TCP socket */
void webcom_socket_cb(EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	UNUSED_VAR(loop);
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	wc_cnx_on_readable(cnx);

}

/* This callback is called by the webcom SDK when events such as "connection is
 * ready", "connection was closed", or "incoming webcom message" occur on the
 * webcom connection.
 */
void webcom_service_cb(wc_event_t event, wc_cnx_t *cnx, void *data,
		UNUSED_PARAM(size_t len), void *user)
{
	wc_msg_t *msg = (wc_msg_t*) data;
	struct ev_loop *loop = (struct ev_loop*) user;

	switch (event) {
		case WC_EVENT_ON_CNX_ESTABLISHED:

			wc_req_listen(cnx, NULL, "/");

			break;
		case WC_EVENT_ON_MSG_RECEIVED:

			if (msg->type == WC_MSG_DATA
					&& msg->u.data.type == WC_DATA_MSG_PUSH
					&& msg->u.data.u.push.type == WC_PUSH_DATA_UPDATE_PUT)
			{
				json_object *json = json_tokener_parse(msg->u.data.u.push.u.update_put.data);
				if (json != NULL) {
					if(strcmp(msg->u.data.u.push.u.update_put.path, "/") == 0) {
						json_object_object_foreach(json, key, val) {
							UNUSED_VAR(key);
							print_new_message(val);
						}
					} else {
						print_new_message(json);
					}
				}
				json_object_put(json);
			}

			break;
		case WC_EVENT_ON_CNX_CLOSED:
			ev_break(EV_A_ EVBREAK_ALL);
			wc_cnx_free(cnx);
			break;
		default:
			break;
	}
}

void print_new_message(json_object *message) {
	json_object *name;
	json_object *text;

	json_object_object_get_ex(message, "name", &name);
	json_object_object_get_ex(message, "text", &text);

	wprintw(chat, "%*s: %s\n",
			15,
			name == NULL ? "<Anonymous>" : json_object_get_string(name),
			text == NULL ? "<NULL>" : json_object_get_string(text));
	wrefresh(chat);
	werase(input);
	wrefresh(input);
}

/* libev timer callback, to send keepalives to the webcom server periodically
 */
void keepalive_cb(EV_P_ ev_timer *w, UNUSED_PARAM(int revents)) {
	UNUSED_VAR(loop);
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	/* just this */
	wc_cnx_keepalive(cnx);
}

/*
 * libev callback when data is available on stdin
 */
void stdin_cb (EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;
	static char buf[2048];
	json_object *text;
	char *json_str;
	const char *escaped_text;

	if (wgetnstr(input, buf, sizeof(buf)) == OK) {
		if(strcmp(buf, "/quit") == 0) {
			ev_io_stop(EV_A_ w);
			ev_break(EV_A_ EVBREAK_ALL);
			wc_cnx_close(cnx);
		}
		text = json_object_new_string(buf);
		escaped_text = json_object_to_json_string(text);
		asprintf(&json_str, "{\"name\":%s,\"text\":%s}", escaped_name, escaped_text);
		wc_req_push(cnx, NULL, "/", json_str);
		free(json_str);
		json_object_put(text);
	}
	if (feof(stdin)) {
		/* quit if EOF on stdin */
		ev_io_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
		wc_cnx_close(cnx);
	}
}
