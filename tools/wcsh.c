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
#include <stdarg.h>

#include <webcom-c/webcom.h>
#include <webcom-c/webcom-libev.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <json-c/json.h>

#include "wcsh.h"
#include "wcsh_parse.h"
#include "wcsh_program.h"

#include "../lib/webcom_base_priv.h"
#include "../lib/datasync/cache/treenode_cache.h"
#include "../lib/datasync/on/on_registry.h"
#include "../lib/datasync/listen/listen_registry.h"
#include "../lib/datasync/path.h"
#include "../lib/collection/avl.h"

static void on_connected(wc_context_t *ctx);
static int on_disconnected(wc_context_t *ctx);
static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len);
static void update_prompt();
static void rlhandler(char* line);
void stdin_cb (EV_P_ ev_io *w, int revents);
void script_data_event (EV_P_ ev_io *w, UNUSED_PARAM(int revents));
static void printf_async(char *fmt, ...);

char **wcsh_completion(const char *, int, int);
char *command_generator (const char *text, int state);

static void exec_ls(int, char **);
static void exec_log(int, char **);
static void exec_cache(int argc, char **argv);
static void exec_cd(int, char **);
static void exec_connect(int, char **);
static void exec_disconnect(int, char **);
static void exec_exit(int, char **);
static void exec_help(int, char **);
static void exec_merge(int, char **);
static void exec_on(int, char **);
static void exec_off(int, char **);
static void exec_push(int, char **);
static void exec_put(int, char **);
static void exec_info(int, char **);
static void exec_sleep(int, char **);
static void exec_wait(int, char **);
static void wcsh_log(const char *f, const char *l, const char *file, const char *func, int line, const char *message);

static wc_context_t *ctx;
struct ev_loop *loop;
static int done = 0;
static int disconnect = 0;
static int connected = 0;
int is_interactive = 1;

FILE *out;
char prompt[65];

struct command {
	char *name;
	char *help;
	char *usage;
	int min_args;
	int max_args;
	void (*execute)(int, char **);
	char **complete_args;
};

/* keep these arrays in alphabetical order since we use binary search on them */
char *on_off_args[] = {"child_added", "child_changed", "child_removed","value", NULL};
char *info_args[] = {"listen", "on", NULL};
char *help_args[] = {"", "cache", "cd", "connect", "disconnect", "exit", "help", "info", "ls", "merge", "off", "on", "push", "put", NULL};
char *log_args[] = {"off", "on", "verbose", NULL};
char *cache_args[] = {"dump", "load", "save", NULL};

/* keep this array in alphabetical order since we use binary search on it */
static struct command commands[] = {
		{"cache",      "Displays internal informations about the \"on\"-subscriptions, listen status",
		               "show {cache,on,listen}",
		               1, 2, exec_cache, cache_args},
		{"cd",         "Sets the current working path to \"/\", or to the specified path",
		               "cd [PATH]",
		               0, 1, exec_cd},
		{"connect",    "Connects to the datasync server",
		               "connect",
		               0, 0, exec_connect},
		{"disconnect", "Disconnects from the datasync server",
		               "disconnect",
		               0, 0, exec_disconnect},
		{"exit",       "Exits from this shell",
		               "exit",
		               0, 0, exec_exit},
		{"help",       "Shows available commands or shows help about a command",
		               "help [COMMAND]",
		               0, 1, exec_help, help_args},
		{"info",       "Displays internal informations about the \"on\"-subscriptions, listen status",
		               "info {on,listen}",
		               1, 1, exec_info, info_args},
		{"log",        "Enable or disables logs",
		               "log {on,off,verbose}",
		               1, 1, exec_log, log_args},
		{"ls",         "Displays the cached contents of the datasync database at the current path or at the specified path",
		               "ls [PATH]",
		               0, 1, exec_ls},
		{"merge",      "Sends a merge request on a given path, with a given json document",
		               "merge PATH JSON",
		               2, 2, exec_merge},
		{"off",        "Unregisters from a given data event, or all data events on a given path",
		               "off [{value,child_added,child_removed,child_changed}] PATH",
		               1, 2, exec_off, on_off_args},
		{"on",         "Registers to a given data event on a gven path",
		               "on {value,child_added,child_removed,child_changed} PATH",
		               2, 2, exec_on, on_off_args},
		{"push",       "Sends a push request on a given path, with a given json document",
		               "push PATH JSON",
		               2, 2, exec_push},
		{"put",        "Sends a put request on a given path, with a given json document",
		               "put PATH JSON",
		               2, 2, exec_put},
		{"sleep",      "Suspends the interpreter for a given number of seconds",
		               "sleep SECONDS",
		               1, 1, exec_sleep},
		{"wait",       "Waits for a particular event before executing the following statements",
		               "wait {connected,disconnected} | wait data PATH JSON",
		               1, 3, exec_wait},
		{NULL}
};

static int commands_len = (sizeof(commands)/sizeof(commands[0]) - 1);

wc_ds_path_t root_path = {._buf = "", ._norm = "/", .nparts = 0, .offsets = {}};

wc_ds_path_t *cwd = &root_path;

struct wc_context_options options = {
		.host = "io.datasync.orange.com",
		.port = 443,
		.app_name = "playground"
};

static struct {
	unsigned
		connected:1,
		disconnected:1,
		data:1;

	on_handle_t data_cb;
	treenode_hash_t data_hash;
} waiting = {};

int main(int argc, char *argv[]) {
	ev_io input_watcher;
	int opt;
	int script_fd;

	struct wc_eli_callbacks cb = {
			.on_connected = on_connected,
			.on_disconnected = on_disconnected,
			.on_error = on_error,
	};
	out = stderr;


	while ((opt = getopt(argc, argv, "s:p:n:o:h")) != -1) {
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
		case 'o':
			out = fopen(optarg, "w");
			if (out == NULL) {
				perror(optarg);
				exit(1);
			}
			break;
		case 'h':
			printf(
					"%s [OPTIONS] [SCRIPT_FILE]\n"
					"Opens a new Webcom shell in interactive mode, or execute the script if a SCRIPT_FILE argument is given"
					"Options:\n"
					"-s HOST      : Connect to the specified host (current \"%s\")\n"
					"-p PORT      : Connect on the specified port (current %" PRIu16 ")\n"
					"-n NAMESPACE : Connect to the specified namespace (current \"%s\")\n"
					"-o FILE      : Print the notifications to the given file instead of stderr)\n"
					"-h           : Displays help\n",
					*argv, options.host, options.port, options.app_name);
			exit(0);
			break;
		}
	}

	if (optind < argc) {
		is_interactive = 0;
		script_fd = open(argv[optind], O_RDONLY);
		if (script_fd == -1) {
			perror(argv[optind]);
			return 1;
		}
	}

	loop = EV_DEFAULT;

	ctx = wc_context_create_with_libev(
			&options,
			loop,
			&cb);

	wc_log_use_custom(wcsh_log);

	wc_datasync_init(ctx);


	if (is_interactive) {
		rl_prep_terminal(0);
				update_prompt();
		rl_callback_handler_install(prompt, rlhandler);
		rl_attempted_completion_function = wcsh_completion;

		ev_io_init(&input_watcher, stdin_cb, STDIN_FILENO, EV_READ);
		input_watcher.data = ctx;
		ev_io_start(loop, &input_watcher);

	} else {
		ev_io_init(&input_watcher, script_data_event, script_fd, EV_READ);
		input_watcher.data = ctx;
		ev_io_start (loop, &input_watcher);
	}

	ev_run(loop, 0);

	if (is_interactive) {
		rl_callback_handler_remove();
		rl_deprep_terminal();

		puts("");
	}

	wc_context_destroy(ctx);
	return 0;
}

static void printf_async(char *fmt, ...) {
    char *saved_line;
	int saved_point;

	if (is_interactive) {
		saved_point = rl_point;
		saved_line = rl_copy_text(0, rl_end);
		rl_save_prompt();
		rl_replace_line("", 0);
		rl_redisplay();
	}

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	if (is_interactive) {
		rl_restore_prompt();
		rl_replace_line(saved_line, 0);
		rl_point = saved_point;
		rl_redisplay();
		free(saved_line);
	}
}

static void wcsh_log(const char *f, const char *l, const char *file, const char *func, int line, const char *message) {
	printf_async("[%s] (%s:%d) %s: %s", l, file, line, f, message);
}

static void on_connected(wc_context_t *ctx) {
	connected = 1;
	update_prompt();
	printf_async("Connected.\n");
	if (waiting.connected) {
		waiting.connected = 0;
		wcsh_program_resume();
	}
}

static int on_disconnected(wc_context_t *ctx) {
	connected = 0;
	update_prompt();
	printf_async("Disconnected.\n");
	if (waiting.disconnected) {
		waiting.disconnected = 0;
		wcsh_program_resume();
	}
	return !done && !disconnect;
}

static int on_error(wc_context_t *ctx, unsigned next_try, const char *error, int error_len) {
	connected = 0;
	update_prompt();
	printf_async("connection error in ctx %p, %.*s, next attempt in: %u seconds\n", ctx, error_len, error, next_try);
	return 1;
}


void on_value_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	printf_async("Value changed: %s\n", data);
}

void on_child_added_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	printf_async("Child added: [%s] %s\n", cur, data);
}

void on_child_removed_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	printf_async("Child removed: [%s]\n", cur);
}

void on_child_changed_cb(wc_context_t *ctx, char *data, char *cur, char *prev) {
	printf_async("Child changed: [%s] %s\n", cur, data);
}

static void exec_on(int argc, char **argv) {
	if (strcmp("value", argv[0]) == 0) {
		wc_datasync_on_value(ctx, argv[1], on_value_cb);
	} else if (strcmp("child_added", argv[0]) == 0) {
		wc_datasync_on_child_added(ctx, argv[1], on_child_added_cb);
	} else if (strcmp("child_removed", argv[0]) == 0) {
		wc_datasync_on_child_removed(ctx, argv[1], on_child_removed_cb);
	} else if (strcmp("child_changed", argv[0]) == 0) {
		wc_datasync_on_child_changed(ctx, argv[1], on_child_changed_cb);
	} else {
		fprintf(stderr, "Usage:\n on {value,child_added,child_removed_child_changed} PATH\n");
	}
}

static void exec_off(int argc, char **argv) {
	if (argc == 1) {
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

static void exec_info(int argc, char **argv) {
	if (argc == 0) {
		printf("Usage:\n info {on,listen}\n");
	} else if (strcmp("on", argv[0]) == 0) {
		dump_on_registry(ctx->datasync.on_reg, stdout);
	} else if (strcmp("listen", argv[0]) == 0) {
		dump_listen_registry(ctx->datasync.listen_reg, stdout);
	}
}

static void exec_cache_save(int argc, char **argv) {
	FILE *f;

	if (argc == 1) {
		f = fopen(*argv, "w");
		if (f == NULL) {
			perror(*argv);
		} else {
			ftreenode_to_json(ctx->datasync.cache->root, f);
			fclose(f);
		}
	}
}

static void exec_cache_load(int argc, char **argv) {
	json_object *json;

	if (argc == 1) {
		if ((json = json_object_from_file(*argv)) != NULL) {
			data_cache_set_ex(ctx->datasync.cache, &root_path, json);
		} else {
			fprintf(stderr, "error opening \"%s\"", *argv);
		}
	}
}

static void exec_cache(int argc, char **argv) {
	if (argc == 0) {
		printf("Usage:\n cache {dump,save,load}\n");
	} else if (strcmp("dump", argv[0]) == 0) {
		ftreenode_to_json(ctx->datasync.cache->root, stdout);
		puts("");
	} else if (strcmp("load", argv[0]) == 0) {
		exec_cache_load(--argc, ++argv);
	} else if (strcmp("save", argv[0]) == 0) {
		exec_cache_save(--argc, ++argv);
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

static void sleep_cb (struct ev_loop *loop, ev_timer *w, int revents) {
	wcsh_program_resume();
}

static void exec_sleep(int argc, char **argv) {
	static ev_timer ev_sleep;
	unsigned t;
	if (sscanf(argv[0],"%u",&t) == 1) {
		ev_timer_init(&ev_sleep, sleep_cb, (ev_tstamp) t, 0);
		ev_timer_start(loop, &ev_sleep);
		wcsh_program_pause();
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

static void exec_log(int argc, char **argv) {
	if (strcmp(argv[0], "on") == 0) {
		wc_set_log_level(WC_LOG_ALL, WC_LOG_INFO);
	} else if (strcmp(argv[0], "off") == 0) {
		wc_set_log_level(WC_LOG_ALL, WC_LOG_CRIT);
	} else if (strcmp(argv[0], "verbose") == 0) {
		wc_set_log_level(WC_LOG_ALL, WC_LOG_DEBUG);
	} else {

	}
}

static void exec_connect(int argc, char **argv) {
	disconnect = 0;
	wc_datasync_connect(ctx);
}

static void exec_disconnect(int argc, char **argv) {
	disconnect = 1;
	wc_datasync_close_cnx(ctx);
}

static void exec_exit(int argc, char **argv) {
	done = 1;
}
void data_wait_cb(wc_context_t *ctx, on_handle_t handle, char *data, char *cur, char *prev) {
	treenode_hash_t *cached_hash;

	if (handle == waiting.data_cb) {
		cached_hash = treenode_hash_get(data_cache_get(ctx->datasync.cache, wc_datasync_on_handle_get_path(handle)));

		if (treenode_hash_eq(&waiting.data_hash, cached_hash)) {
			waiting.data = 0;
			wcsh_program_resume();
		}
	}
}

static void exec_wait(int argc, char **argv) {
	struct treenode *n;
	treenode_hash_t *cached_hash;
	treenode_hash_t *wanted_hash;

	if (strcmp(argv[0], "connected") == 0) {
		if (!connected) {
			waiting.connected = 1;
			wcsh_program_pause();
		}
	} else if (strcmp(argv[0], "disconnected") == 0) {
		if (connected) {
			waiting.disconnected = 1;
			wcsh_program_pause();
		}
	} else if (strcmp(argv[0], "data") == 0 && argc == 3) {
		n = treenode_from_json(argv[2]);

		cached_hash = treenode_hash_get(data_cache_get(ctx->datasync.cache, argv[1]));
		wanted_hash = treenode_hash_get(n);

		if (!treenode_hash_eq(wanted_hash, cached_hash)) {
			treenode_hash_copy(wanted_hash, &waiting.data_hash);
			waiting.data = 1;
			waiting.data_cb = wc_datasync_on_value(ctx, argv[1], data_wait_cb);
			wcsh_program_pause();
		}

		treenode_destroy(n);
	}
}

struct command *find_command(char *name) {
	int min = 0, max = commands_len - 1;
	int mid;
	int cmp;
	struct command *ret = NULL;

	while (min <= max) {
		mid = min + (max - min) / 2;

		cmp = strcmp(commands[mid].name, name);
		if (cmp < 0) {
			min = mid + 1;
		} else if (cmp > 0) {
			max = mid - 1;
		} else {
			ret = &commands[mid];
			break;
		}
	}

	return ret;
}

static void exec_merge(int argc, char **argv) {
	wc_datasync_merge(ctx, argv[0], argv[1], NULL, NULL);
}

static void exec_push(int argc, char **argv) {
	wc_datasync_push(ctx, argv[0], argv[1], NULL, NULL);
}

static void exec_put(int argc, char **argv) {
	wc_datasync_put(ctx, argv[0], argv[1], NULL, NULL);
}

static void exec_help(int argc, char **argv) {
	int i;
	struct command *cmd;

	if (argc == 0) {
		printf("Available commands:\n");
		for (i = 0 ; i < commands_len ; i++) {
			printf("  - %s: %s\n", commands[i].name, commands[i].help);
		}
	} else {
		cmd = find_command(argv[0]);
		if (cmd == NULL) {
			printf("Unknown command \"%s\", see \"help\" for available commands\n", argv[0]);
		} else {
			printf("Description:\n\t%s\nUsage:\n\t%s\n", cmd->help, cmd->usage);
		}
	}
}

void wcsh_exec(int argc, char **argv) {
	struct command *cmd;

	cmd = find_command(argv[0]);

	if (cmd == NULL) {
		printf("Unknown command \"%s\", see \"help\"\n", argv[0]);
	} else {
		if (argc -1 > cmd->max_args) {
			fprintf(stderr,
					"%s: too many arguments\nusage: %s\n",
					cmd->name, cmd->usage);
		} else if (argc - 1 < cmd->min_args) {
			fprintf(stderr,
					"%s: not enough arguments\nusage: %s\n",
					cmd->name, cmd->usage);
		} else {
			cmd->execute(--argc, ++argv);
		}
	}
}

int count_heading_blanks(char *s) {
	int i;
	for (i = 0 ; s[i] && isspace(s[i]) ; i++);
	return i;
}

int count_non_blanks(char *s) {
	int i;
	for (i = 0 ; s[i] && !isspace(s[i]) ; i++);
	return i;
}

char *command_generator (const char *text, int state) {
	static struct command *cmd = NULL;
	static int text_len;

	int i;

	if (state == 0) {
		text_len = strlen(text);

		for (i = 0 ; i < commands_len ; i++) {
			if (strncmp(text, commands[i].name, text_len) == 0) {
				cmd = &commands[i];
				break;
			}
		}
	}

	if (cmd != NULL && cmd->name != NULL && strncmp(text, cmd->name, text_len) == 0) {
		return strdup((cmd++)->name);
	} else {
		return NULL;
	}
}

static char **arg_choices_list = NULL;

char *arg_generator (const char *text, int state) {
	static char **choices, **cur;
	static size_t text_len, choices_len;

	if (state == 0) {
		text_len = strlen(text);
		choices = arg_choices_list;

		cur = choices;

		for (cur = choices ; *cur ; cur++) {
			if (strncmp(text, *cur, text_len) == 0) {
				break;
			}
		}
	}

	if (*cur && strncmp(text, *cur, text_len) == 0) {
		return strdup(*cur++);
	} else {
		return NULL;
	}
}

char **wcsh_completion(const char *text, int start, int end) {
	char **matches = NULL;
	int command_start;
	struct command *cmd = NULL;

	command_start = count_heading_blanks(rl_line_buffer);

	if (start == command_start) {
		matches = rl_completion_matches(text, command_generator);
	} else {
		cmd = find_command(rl_line_buffer + command_start);
		if (cmd != NULL && cmd->complete_args != NULL) {
			arg_choices_list = cmd->complete_args;
			matches = rl_completion_matches(text, arg_generator);
		}
	}

	rl_attempted_completion_over = 1;
	return matches;
}

static void update_prompt() {
	snprintf(prompt, sizeof(prompt), "%s@%s:%"PRIu16"(%s) %s> ", options.app_name, options.host, options.port, connected ? "C":"D", wc_datasync_path_to_str(cwd));
	prompt[sizeof(prompt) - 1] = 0;

	rl_set_prompt(prompt);
}

static void rlhandler(char* line) {
	int argc;
	char **argv;
	if(line == NULL) {
		done = 1;
	} else {
		if (*line != 0) {
			add_history(line);
			wcsh_cmdline_eval(line);
		}

		free(line);
		update_prompt();
	}
}

void stdin_cb (EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	if (revents & EV_READ) {
		rl_callback_read_char();
	}

	if (done) {
		ev_io_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
	}
}

void script_data_event (EV_P_ ev_io *w, UNUSED_PARAM(int revents)) {
	char buf[2];
	size_t len;
	int done_reading = 0;
	static char *end = "exit";

	if (revents & EV_READ) {
		len = read(w->fd, buf, sizeof(buf));
		if (len > 0) {
			wcsh_chunk_eval(buf, len);
		} else {
			wcsh_chunk_eval(NULL, 0);
			done_reading = 1;
			wcsh_program_append(1, &end);
		}

	}

	if (done_reading) {
		ev_io_stop(EV_A_ w);
	}
}
