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
#include <stdio.h>
#include <stdarg.h>
#include <getopt.h>
#include <unistd.h>
#include <webcom-c/webcom.h>
#include <webcom-c/webcom-libev.h>
#include <ev.h>
#include <json-c/json.h>
#include <readline/readline.h>
#include <readline/history.h>

static void on_connected(wc_context_t *ctx);
static int on_disconnected(wc_context_t *ctx);
static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len);
void stdin_cb (EV_P_ ev_io *w, int revents);
void on_child_added_cb(wc_context_t *ctx, char *data, char *cur, char *prev);
void print_new_message(json_object *message);
static void printf_async(char *fmt, ...);
static void rlhandler(char* line);

static wc_context_t *ctx;
static int done = 0;


char *name = NULL;
const char *escaped_name;

int main(int argc, char *argv[]) {
	struct ev_loop *loop = EV_DEFAULT;
	ev_io stdin_watcher;
	int opt;
	json_object *json_name;

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

	if (name == NULL) {
		name = calloc(17,1);
		getlogin_r(name, 16);
	}

	struct wc_eli_callbacks cb = {
			.on_connected = on_connected,
			.on_disconnected = on_disconnected,
			.on_error = on_error,
	};

	struct wc_context_options options = {
			.host = "io.datasync.orange.com",
			.port = 443,
			.app_name = "chat"
	};

	ctx = wc_context_create_with_libev(
			&options,
			loop,
			&cb);

	wc_datasync_init(ctx);
	wc_datasync_connect(ctx);

	json_name = json_object_new_string(name);
	escaped_name = json_object_to_json_string(json_name);

	rl_prep_terminal(0);
	rl_callback_handler_install("> ", rlhandler);

	ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
	stdin_watcher.data = ctx;
	ev_io_start (loop, &stdin_watcher);

	ev_run(loop, 0);
	rl_callback_handler_remove();
	rl_deprep_terminal();
	json_object_put(json_name);
	return 0;
}

/*
 * this callback is called by the Webcom SDK once the connection was
 * established
 */
static void on_connected(wc_context_t *ctx) {

	wc_datasync_on_child_added(ctx, "/", on_child_added_cb);
	printf_async("*** [INFO] Connected\n");

}

static int on_disconnected(wc_context_t *ctx) {
	printf_async("*** [INFO] Disconnected (connection closed)\n");
	return 1;
}

static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len) {
	printf_async("*** [INFO] Disconnected (%.*s)\n", error_len, error);
	return 1;
}

void on_child_added_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	json_object *json = json_tokener_parse(data);
	print_new_message(json);
	json_object_put(json);
}

void print_new_message(json_object *message) {
	json_object *name;
	json_object *text;

	json_object_object_get_ex(message, "name", &name);
	json_object_object_get_ex(message, "text", &text);

	printf_async("%*s: %s\n",
			15,
			name == NULL ? "<Anonymous>" : json_object_get_string(name),
			text == NULL ? "<NULL>" : json_object_get_string(text));
}

/*
 * libev callback when data is available on stdin
 */
void stdin_cb (EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	if (revents & EV_READ) {
		rl_callback_read_char();
	}

	if (done) {
		ev_io_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
	}
}

static void rlhandler(char* line) {
	char *json_str;
	const char *escaped_text;
	json_object *text;

	if(line == NULL) {
		done = 1;
	} else {
		if (*line != 0) {
			add_history(line);
			text = json_object_new_string(line);
			escaped_text = json_object_to_json_string(text);
			asprintf(&json_str, "{\"name\":%s,\"text\":%s}", escaped_name, escaped_text);
			wc_datasync_push(ctx, "/", json_str, NULL, NULL);
			free(json_str);
			json_object_put(text);
		}

		free(line);
	}
}

static void printf_async(char *fmt, ...) {
    char *saved_line;
	int saved_point;

	saved_point = rl_point;
	saved_line = rl_copy_text(0, rl_end);
	rl_save_prompt();
	rl_replace_line("", 0);
	rl_redisplay();

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	rl_restore_prompt();
	rl_replace_line(saved_line, 0);
	rl_point = saved_point;
	rl_redisplay();
	free(saved_line);
}
