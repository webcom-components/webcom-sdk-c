#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <webcom-c/webcom.h>
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
static void draw_brick(int x, int y, legorange_brick_t brick);
static void move_to(int x, int y);
static void clear_screen();
void keepalive_cb(EV_P_ ev_timer *w, int revents);
void webcom_socket_cb(EV_P_ ev_io *w, int revents);
void stdin_cb (EV_P_ ev_io *w, int revents);
void webcom_service_cb(wc_event_t event, wc_cnx_t *cnx, void *data,
		size_t len, void *user);
static void on_data_update_update_put_cb(wc_cnx_t *cnx, int put_or_merge, char *path, char *json_data, void *param);
static void on_brick_update(char *key, json_object *data);
/*
 * end of boredom
 */

/* Enter here */
int main(int argc, char *argv[]) {
	wc_cnx_t *cnx;
	int wc_fd;
	struct ev_loop *loop = EV_DEFAULT;
	ev_io stdin_watcher;
	ev_io webcom_watcher;
	ev_timer ka_timer;
	int opt;

	char *proxy_host = NULL;
	uint16_t proxy_port = 8080;
	board_name = "/brick";

	/* [mildly boring] set stdout unbuffered to see the bricks appear in real
	 * time */
	setbuf(stdout, NULL);

	/* [boring] parse the command line options (proxy settings, board) */
	while ((opt = getopt(argc, argv, "P:p:b:")) != -1) {
		switch((char)opt) {
		case 'P':
			proxy_host = optarg;
			break;
		case 'p':
			proxy_port = (uint64_t) strtoul(optarg, NULL, 10);
			break;
		case 'b':
			board_name = optarg;
		}
	}

	/* establish the connection to the webcom server (without or with a proxy) */
	if (proxy_host == NULL) {
		cnx = wc_cnx_new(
				"io.datasync.orange.com",
				443,
				"legorange",
				webcom_service_cb, /* this is the callback that gets called
				when a webcom-level event occurs */
				loop);
	} else {
		cnx = wc_cnx_new_with_proxy(
				proxy_host,
				proxy_port,
				"io.datasync.orange.com",
				443,
				"legorange",
				webcom_service_cb,
				loop);
	}

	if (cnx == NULL) {
		fputs("could not connect\n", stderr);
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
void webcom_socket_cb(EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	UNUSED_VAR(loop);
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;

	/* that's all we must do here: let the webcom SDK handle the data and it
	 * will call the callback defined when creating the connection if a new
	 * webcom event should be triggered */
	wc_cnx_on_readable(cnx);

}

/* This callback is called by the webcom SDK when events such as "connection is
 * ready", "connection was closed", or "incoming webcom message" occur on the
 * webcom connection.
 */
void webcom_service_cb(wc_event_t event, wc_cnx_t *cnx, UNUSED_PARAM(void *data),
		UNUSED_PARAM(size_t len), void *user)
{
	struct ev_loop *loop = (struct ev_loop*) user;

	switch (event) {
		case WC_EVENT_ON_SERVER_HANDSHAKE:
			/* when the connection is established, register to events in the
			 * BOARD_NAME path
			 */
			wc_on_data(cnx, board_name, on_data_update_update_put_cb, NULL);
			wc_req_listen(cnx, NULL, board_name);
			clear_screen();
			break;
		case WC_EVENT_ON_CNX_CLOSED:
			puts("Connection closed, Bye");
			ev_break(EV_A_ EVBREAK_ALL);
			wc_cnx_free(cnx);
			break;
		default:
			break;
	}
}

static void on_data_update_update_put_cb(UNUSED_PARAM(wc_cnx_t *cnx),
		UNUSED_PARAM(int put_or_merge),
		char *path,
		char *json_data,
		UNUSED_PARAM(void *param)) {
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
		if (json_object_is_type(data, json_type_null)) {
			draw_brick(x, y, NO_BRICK);
		} else {
			if (json_object_object_get_ex(data, "color", &jcolor)) {
				scolor = (char *)json_object_get_string(jcolor);
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

				/* it works */
				draw_brick(x, y, brick);
			}
		}
	}
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
 * libev callback when data is available on stdin: parse it and send a put
 * message to the webcom server if the input matches "x y color"
 */
void stdin_cb (EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	wc_cnx_t *cnx = (wc_cnx_t *)w->data;
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
			if (wc_req_put(cnx, NULL, path, data) > 0) {
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
		wc_cnx_close(cnx);
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
		MAKE_BRICK(VT100_YELLOW,	VT100_NORMAL),
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

