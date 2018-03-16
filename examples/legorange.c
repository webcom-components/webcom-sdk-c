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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <webcom-c/webcom.h>
#include <webcom-c/webcom-libev.h>
#include <ev.h>
#include <getopt.h>
#include <json-c/json.h>

/*
 * boring forward declarations to make the compiler happy, skip this part
 */
typedef enum {
	NO_BRICK = 0, WHITE_BRICK, GREEN_BRICK, RED_BRICK, GREY_BRICK, BLUE_BRICK,
	YELLOW_BRICK, BROWN_BRICK, OTHER_BRICK
} legorange_brick_t;
char *board_name;
extern const char *bricks[];
int max_l = 250, max_c = 250;
static void on_connected(wc_context_t *ctx);
static int on_disconnected(wc_context_t *ctx);
static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len);
static void draw_brick(int x, int y, legorange_brick_t brick);
static void draw_rgb_brick(int x, int y, int r, int g, int b);
static void move_to(int x, int y);
static void clear_screen();
void stdin_cb (EV_P_ ev_io *w, int revents);
static void on_data_update(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param);
static void on_brick_update(char *key, json_object *data);
/*
 * end of boredom
 */

/* Enter here */
int main(int argc, char *argv[]) {
	wc_context_t *ctx;
	struct ev_loop *loop = EV_DEFAULT;
	ev_io stdin_watcher;
	int opt;

	board_name = "/brick";

	/* [mildly boring] set stdout unbuffered to see the bricks appear in real
	 * time */
	setbuf(stdout, NULL);

	while ((opt = getopt(argc, argv, "b:hl:c:")) != -1) {
		switch((char)opt) {
		case 'b':
			if (*optarg != '/') {
				fprintf(stderr, "the board name must begin with '/'\n");
				exit(1);
			}
			board_name = optarg;
			break;
		case 'h':
			printf(
					"%s [OPTIONS]\n"
					"Options:\n"
					"-b BOARDNAME: Use the board BOARDNAME instead of the default board. It MUST\n"
					"              contain a leading '/', e.g. \"-b /demo\"\n"
					"-l MAX_LINES: Don't display bricks beyond the MAX_LINES line\n"
					"-c MAX_COLS : Don't display bricks beyond the MAX_COLS column\n"
					"-h          : Displays this help message.\n",
					*argv);
			exit(0);
			break;
		case 'l':
			max_l = (int) atoi(optarg);
			break;
		case 'c':
			max_c = (int) atoi(optarg);
			break;
		}

	}

	/*
	 * let's prepare a few informations to pass to the SDK
	 */
	struct wc_eli_callbacks cb = {
			.on_connected = on_connected,
			.on_disconnected = on_disconnected,
			.on_error = on_error,
	};

	struct wc_context_options options = {
			.host = "io.datasync.orange.com",
			.port = 443,
			.app_name = "legorange",
	};

	wc_log_use_stderr();

	APP_INFO("Starting legorange");

	/* establish the connection to the webcom server, and let it integrate in
	 * our libev event loop
	 */
	ctx = wc_context_create_with_libev(
			&options,
			loop,
			&cb);

	wc_datasync_init(ctx);
	wc_datasync_connect(ctx);

	/* if stdin has data to read, call stdin_watcher() */
	ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
	stdin_watcher.data = ctx;
	ev_io_start (loop, &stdin_watcher);

	/* enter the event loop */
	ev_run(loop, 0);

	return 0;
}

/*
 * this callback is called by the Webcom SDK once the connection was
 * established
 */
static void on_connected(wc_context_t *ctx) {
	/*
	 * let's configure a route: if some data update happens on the given path,
	 * we instruct the SDK to call on_data_update()
	 */
	wc_datasync_route_data(ctx, board_name, on_data_update, NULL);

	/*
	 * now that the route is configured, let's subscribe to the path
	 */
	wc_datasync_listen(ctx, board_name, NULL);

	clear_screen();
}

/*
 * called by the SDK on disconnection
 */
static int on_disconnected(wc_context_t *ctx) {
	APP_INFO("on disconnect");
	clear_screen();
	return 1;
}

static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len) {
	APP_INFO("connection error in ctx %p, %.*s, next attempt in: %u seconds", ctx, error_len, error, next_try);
	return 1;
}

/*
 * called by the SDK if some data update (put or merge) happens on the
 * registered path
 * (we configured it with wc_on_data(..., on_data_update, ...) )
 */
static void on_data_update(
		UNUSED_PARAM(wc_context_t *cnx),
		UNUSED_PARAM(ws_on_data_event_t event),
		char *path,
		char *json_data,
		UNUSED_PARAM(void *param))
{
	json_object *data;

	data = json_tokener_parse(json_data);

	if (strcmp(board_name, path) == 0) {
		/* we got informations for the entire board */
		if (data == NULL) {
			/* the board was reset */
			clear_screen();
		} else {
			json_object_object_foreach(data, key, val) {
				on_brick_update(key, val);
			}
		}
	} else if (strncmp(board_name, path, strlen(board_name)) == 0
			&& path[strlen(board_name)] == '/') {
		/* just one single brick data was modified */
		on_brick_update(path + strlen(board_name) + 1, data);
	}

	json_object_put(data);
}

/*
 * interprets the received json data for a single brick
 */
static void on_brick_update(char *key, json_object *data) {
	int x, y;
	json_object *jcolor;
	char *scolor;
	legorange_brick_t brick;

	if (sscanf(key, "%4d-%4d", &x, &y) == 2) {
		if (x >= max_c/2 || y >= max_l || x < 0 || y < 0) {
			return;
		}
		if (json_object_is_type(data, json_type_null)) {
			draw_brick(x, y, NO_BRICK);
		} else {
			if (json_object_object_get_ex(data, "color", &jcolor)) {
				scolor = (char *)json_object_get_string(jcolor);
				if (*scolor == '#') {
					int r, g, b;
					if (sscanf(scolor + 1, "%2x%2x%2x", &r, &g, &b) == 3) {
						draw_rgb_brick(x, y, r, g, b);
					}
				} else {
					if (strcmp("white", scolor) == 0) {
						brick = WHITE_BRICK;
					} else if (strcmp("green", scolor) == 0) {
						brick = GREEN_BRICK;
					} else if (strcmp("red", scolor) == 0) {
						brick = RED_BRICK;
					} else if (strcmp("darkgrey", scolor) == 0) {
						brick = GREY_BRICK;
					} else if (strcmp("blue", scolor) == 0) {
						brick = BLUE_BRICK;
					} else if (strcmp("yellow", scolor) == 0) {
						brick = YELLOW_BRICK;
					} else if (strcmp("brown", scolor) == 0) {
						brick = BROWN_BRICK;
					} else {
						brick = OTHER_BRICK;
					}

					draw_brick(x, y, brick);
				}
			}
		}
	}
}

/*
 * libev callback when data is available on stdin: parse it and send a put
 * message to the webcom server if the input matches "x y color"
 */
void stdin_cb (EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	wc_context_t *cnx = (wc_context_t *)w->data;
	static char buf[2048];
	int x, y, col;
	char *col_str, *path, *data, nl;
	if (fgets(buf, sizeof(buf), stdin)) {
		if(sscanf(buf, "%4d %4d %4d%c", &x, &y, &col, &nl) == 4) {
			col_str = col ? "white" : "black";

			/* build the path and the data */
			asprintf(&path, "%s/%d-%d",board_name, x, y);
			asprintf(&data,"{\"color\":\"%s\",\"uid\":\"anonymous\",\"x\":%d,"
					"\"y\":%d}", col_str, x, y);

			/* send the put message to the webcom server */
			if (wc_datasync_put(cnx, path, data, NULL) > 0) {
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
		move_to(0, 0);
		puts("Closing...");
		ev_io_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
		wc_datasync_close_cnx(cnx);
	}
}

/* the bricks are drawn on the terminal using some VT100 magic, this is their
 * story:
 */

#define VT100_NORMAL	0
#define VT100_BRIGHT	1
#define VT100_DIM		2
#define VT100_BLACK		30
#define VT100_RED		31
#define VT100_GREEN		32
#define VT100_YELLOW	33
#define VT100_BLUE		34
#define VT100_MAGENTA	35
#define VT100_CYAN		36
#define VT100_WHITE		37

#define STRINGIFY(e) #e
#define MAKE_BRICK(color, attr) "\033[" STRINGIFY(attr) ";" STRINGIFY(color) \
	"m ●\033[0m"

/* this tab can be indexed with the legorange_brick_t enum, how convenient! */
const char *bricks[] = {
		"  ", /* 2 spaces to erase one brick */
		MAKE_BRICK(VT100_WHITE,		VT100_BRIGHT),
		MAKE_BRICK(VT100_GREEN,		VT100_NORMAL),
		MAKE_BRICK(VT100_RED,		VT100_NORMAL),
		MAKE_BRICK(VT100_BLACK,		VT100_BRIGHT),
		MAKE_BRICK(VT100_BLUE,		VT100_NORMAL),
		MAKE_BRICK(VT100_YELLOW,	VT100_BRIGHT),
		MAKE_BRICK(VT100_RED,		VT100_DIM),
		MAKE_BRICK(VT100_WHITE,		VT100_NORMAL),
};

static void clear_screen() {
	fputs("\033[2J", stdout);
}

static void move_to(int x, int y) {
	printf("\033[%u;%uf",y + 1, 2*x + 1);
}

/*
 * Helper function that draws one brick on the terminal.
 */
static void draw_brick(int x, int y, legorange_brick_t brick) {
	move_to(x, y);
	fputs(bricks[brick], stdout);
}

static void draw_rgb_brick(int x, int y, int r, int g, int b) {
	int ccode;

	ccode = 16 + 36 * (r/43) + 6 * (g/43) + b/43;
	move_to(x, y);
	printf("\033[38;5;%dm ●\033[0m", ccode);
}
