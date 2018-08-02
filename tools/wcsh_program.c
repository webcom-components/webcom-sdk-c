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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "wcsh.h"
#include "wcsh_program.h"

struct wcsh_program_statement {
	int argc;
	char **argv;
	struct wcsh_program_statement *next;
};

enum {PAUSED, RUNNING} program_state = RUNNING;
enum {INACTIVE, ACTIVE} runner_state = INACTIVE;

struct wcsh_program_statement *program_head = NULL, **program_tail = &program_head;
int pending_commands = 0;

void wcsh_program_append(int argc, char **argv) {
	int i;

	if (argc > 0) {
		*program_tail = malloc(sizeof **program_tail);

		(*program_tail)->argc = (int)argc;
		(*program_tail)->argv = malloc(argc * sizeof(*argv));

		for (i = 0 ; i < argc ; i++) {
			(*program_tail)->argv[i] = strdup(argv[i]);
		}

		program_tail = &(*program_tail)->next;
		pending_commands++;

		if (pending_commands > 0
				&& program_state ==  RUNNING
				&& runner_state == INACTIVE) {
			wcsh_program_resume();
		}
	}
}

static void remove_head() {
	struct wcsh_program_statement *bak;
	int i;

	if (pending_commands > 0) {
		bak = program_head->next;

		for (i = 0 ; i < program_head->argc ; i++) {
			free(program_head->argv[i]);
		}
		free(program_head->argv);

		free(program_head);

		program_head = bak;
		pending_commands--;

		if (pending_commands == 0) {
			program_tail = &program_head;
			program_state = PAUSED;
		}
	}
}

int wcsh_program_is_finished() {
	return pending_commands == 0;
}

int wcsh_program_run_step() {
	if (!wcsh_program_is_finished()) {
		wcsh_exec(program_head->argc, program_head->argv);
		remove_head();
		return 0;
	}

	return 1;
}

static struct ev_idle idle;

static void idle_cb(struct ev_loop *loop, ev_idle *w, int revents) {
	wcsh_program_run_step();
}

void wcsh_program_resume() {
	program_state = RUNNING;
	if (pending_commands > 0) {
		runner_state = ACTIVE;
		ev_idle_init(&idle, idle_cb);
		ev_idle_start(loop, &idle);
	}
}

void wcsh_program_pause() {
	program_state = PAUSED;
	runner_state = INACTIVE;
	ev_idle_stop(loop, &idle);
}
