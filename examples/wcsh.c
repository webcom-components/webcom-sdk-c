/*
 * webcom-sdk-c
 *
 * Copyright 2018 Orange
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
#include <getopt.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include <webcom-c/webcom.h>
#include <webcom-c/webcom-libev.h>

#include "../lib/webcom_base_priv.h"
#include "../lib/datasync/cache/treenode_cache.h"
#include "../lib/datasync/on/on_registry.h"
#include "../lib/datasync/listen/listen_registry.h"
#include "../lib/datasync/path.h"
#include "../lib/collection/avl.h"

static void on_connected(wc_context_t *ctx);
static int on_disconnected(wc_context_t *ctx);
static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len);
static void show_prompt();
void stdin_cb (EV_P_ ev_io *w, int revents);


static wc_context_t *ctx;
struct ev_loop *loop;
static int _do_exit = 0;
static int connected = 0;

wc_ds_path_t root_path = {._buf = "", ._norm = "/", .nparts = 0, .offsets = {}};

wc_ds_path_t *cwd = &root_path;

struct wc_context_options options = {
		.host = "io.datasync.orange.com",
		.port = 443,
		.app_name = "playground"
};

int main(int argc, char *argv[]) {
	ev_io stdin_watcher;
	int opt;

	struct wc_eli_callbacks cb = {
			.on_connected = on_connected,
			.on_disconnected = on_disconnected,
			.on_error = on_error,
	};


	while ((opt = getopt(argc, argv, "s:p:n:h")) != -1) {
		switch((char)opt) {
		case 's':
			options.host = optarg;
			break;
		case 'p':
			sscanf(optarg, "%"SCNu16, &options.port);
			break;
		case 'n':
			options.app_name = optarg;
			break;
		case 'h':
			printf(
					"%s [OPTIONS]\n"
					"Options:\n"
					"-s HOST      : Connect to the specified host (current \"%s\")\n"
					"-p PORT      : Connect on the specified port (current %" PRIu16 ")\n"
					"-n NAMESPACE : Connect to the specified namespace (current \"%s\")\n"
					"-h           : Displays help\n",
					*argv, options.host, options.port, options.app_name);
			exit(0);
			break;
		}
	}

	loop = EV_DEFAULT;

	wc_set_log_level(WC_LOG_APPLICATION, WC_LOG_INFO);

	ctx = wc_context_create_with_libev(
			&options,
			loop,
			&cb);

	wc_datasync_init(ctx);

	int flag = fcntl(STDIN_FILENO, F_GETFL);

	fcntl(STDIN_FILENO, F_SETFL, flag | O_NONBLOCK);

	ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
	stdin_watcher.data = ctx;
	ev_io_start (loop, &stdin_watcher);

	show_prompt();

	ev_run(loop, 0);

	puts("");
	return 0;
}

static void on_connected(wc_context_t *ctx) {
	connected = 1;
	fprintf(stderr, "Connected.\n");
}

static int on_disconnected(wc_context_t *ctx) {
	connected = 0;
	fprintf(stderr, "Disconnected.\n");
	return 0;
}

static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len) {
	connected = 0;
	fprintf(stderr, "connection error in ctx %p, %.*s, next attempt in: %u seconds\n", ctx, error_len, error, next_try);
	return 1;
}


void on_value_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	fprintf(stderr, "Value changed: %s\n", data);
}

void on_child_added_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	fprintf(stderr, "Child added: [%s] %s\n", cur, data);
}

void on_child_removed_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	fprintf(stderr, "Child removed: [%s]\n", cur);
}

void on_child_changed_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	fprintf(stderr, "Child changed: [%s] %s\n", cur, data);
}

static void exec_on(int argc, char **argv) {
	if (argc != 2) {
		printf("Usage:\n on {value,child_added,child_removed_child_changed} PATH\n");
	} else if (strcmp("value", argv[0]) == 0) {
		wc_datasync_on_value(ctx, argv[1], on_value_cb);
	} else if (strcmp("child_added", argv[0]) == 0) {
		wc_datasync_on_child_added(ctx, argv[1], on_child_added_cb);
	} else if (strcmp("child_removed", argv[0]) == 0) {
		wc_datasync_on_child_removed(ctx, argv[1], on_child_removed_cb);
	} else if (strcmp("child_changed", argv[0]) == 0) {
		wc_datasync_on_child_changed(ctx, argv[1], on_child_changed_cb);
	} else {
		printf("Usage:\n on {value,child_added,child_removed_child_changed} PATH\n");
	}
}

static void exec_off(int argc, char **argv) {
	if (argc == 0) {
		printf("Usage:\n off [{value,child_added,child_removed_child_changed}] PATH\n");
	} else if (argc == 1) {
		wc_datasync_off_path(ctx, argv[0]);
	} else if (argc == 2) {
		if (strcmp("value", argv[0]) == 0) {
			wc_datasync_off_path_type(ctx, argv[1], ON_VALUE);
		} else if (strcmp("child_added", argv[0]) == 0) {
			wc_datasync_off_path_type(ctx, argv[1], ON_CHILD_ADDED);
		} else if (strcmp("child_removed", argv[0]) == 0) {
			wc_datasync_off_path_type(ctx, argv[1], ON_CHILD_REMOVED);
		} else if (strcmp("child_changed", argv[0]) == 0) {
			wc_datasync_off_path_type(ctx, argv[1], ON_CHILD_CHANGED);
		}
	}
}

static void exec_show(int argc, char **argv) {
	if (argc == 0) {
		printf("Usage:\n dump {cache,on,listen}\n");
	} else if (strcmp("cache", argv[0]) == 0) {
		if (ctx->datasync_init) {
			ftreenode_to_json(ctx->datasync.cache->root, stdout);
			puts("");
		}
	} else if (strcmp("on", argv[0]) == 0) {
		if (ctx->datasync_init) {
			dump_on_registry(ctx->datasync.on_reg, stdout);
			puts("");
		}
	} else if (strcmp("listen", argv[0]) == 0) {
		if (ctx->datasync_init) {
			dump_listen_registry(ctx->datasync.listen_reg, stdout);
			puts("");
		}
	}
}

static void exec_cd(int argc, char **argv) {
	if (argc == 0) {
		cwd = &root_path;
	} else {
		if (cwd != &root_path) {
			wc_datasync_path_destroy(cwd);
		}
		cwd = wc_datasync_path_new(argv[0]);
	}
}


static void print_treenode_entry(struct treenode *n, char *key) {
	switch(n->type) {
	case TREENODE_TYPE_LEAF_BOOL:
		printf("b %s: %s\n", key, n->uval.bool == TN_TRUE ? "true":"false");
		break;
	case TREENODE_TYPE_LEAF_NULL:
			printf("- %s\n", key);
			break;
	case TREENODE_TYPE_LEAF_NUMBER:
			printf("n %s\n", key);
			break;
	case TREENODE_TYPE_LEAF_STRING:
			printf("s %s\n", key);
			break;
	case TREENODE_TYPE_INTERNAL:
			printf("i %s/\n", key);
			break;
	}
}

static void exec_ls(int argc, char **argv) {
	struct treenode *n;
	struct internal_node_element *i;
	struct avl_it it;

	n = data_cache_get_parsed(ctx->datasync.cache, cwd);

	if (n == NULL) {
		printf("- .\n");
	} else if (n->type == TREENODE_TYPE_INTERNAL) {
		avl_it_start(&it, n->uval.children);

		while((i = avl_it_next(&it)) != NULL) {
			print_treenode_entry(&i->node, i->key);
		}
	} else {
		print_treenode_entry(n, ".");
	}
}

static void execl0(int argc, char **argv) {
	if (strcmp("connect", argv[0]) == 0) {
		wc_datasync_connect(ctx);
	} else if (strcmp("disconnect", argv[0]) == 0) {
		wc_datasync_close_cnx(ctx);
	} else if (strcmp("exit", argv[0]) == 0) {
		_do_exit = 1;
	} else if (strcmp("on", argv[0]) == 0) {
		exec_on(--argc, ++argv);
	} else if (strcmp("off", argv[0]) == 0) {
		exec_off(--argc, ++argv);
	} else if (strcmp("cd", argv[0]) == 0) {
		exec_cd(--argc, ++argv);
	} else if (strcmp("ls", argv[0]) == 0) {
		exec_ls(--argc, ++argv);
	} else if (strcmp("show", argv[0]) == 0) {
		exec_show(--argc, ++argv);
	} else if (strcmp("help", argv[0]) == 0) {
		printf("Valid commands:\ncd connect disconnect show exit help ls off on\n");
	} else {
		printf("Unknown command \"%s\", see \"help\"\n", argv[0]);
	}
}

static int parse_cmd_line(char *buf, char **argv) {
	int state = 0;
	int argc = 0;
	char *p;

	for (p = buf ; *p ; p++) {
		if (isspace(*p)) {
			state = 0;
			*p = 0;
		} else {
			if (state == 0) {
				state = 1;
				*argv++ = p;
				argc++;
			}
		}
	}
	return argc;
}

static void show_prompt() {
	printf("%s@%s:%"PRIu16"(%s) %s> ", options.app_name, options.host, options.port, connected ? "C":"D", wc_datasync_path_to_str(cwd));
	fflush(stdout);
}

void stdin_cb (EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	static char buf[2049];
	static char *argv[1024];
	int argc;

	if (fgets(buf, sizeof(buf), stdin)) {
		argc = parse_cmd_line(buf, argv);
		if (argc > 0) {
			execl0(argc, argv);
		}
	}

	if (feof(stdin) || _do_exit) {
		ev_io_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
	} else {
		show_prompt();
	}
}
