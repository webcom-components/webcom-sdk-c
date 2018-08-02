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

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fsm.h"
#include "wcsh.h"
#include "wcsh_parse.h"
#include "wcsh_program.h"

enum cmdline_parse_state {
	STATE_BLANK,
	STATE_TOKEN,
	STATE_GROUP,
	STATE_ESCAPE,
	STATE_GROUP_END,
	STATE_COMMENT,
};

struct wcsh_cmd_parse {
	char *token_list[32];
	unsigned token_list_len;
	char token[1024];
	size_t token_len;

};
static char token_first_char[] = "!\"$%&()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
static char token_char[] =       "!\"#$%&'()*+,-./0123456789:<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
static char group_char[] =       "\t !\"#$%&()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

void on_token_start(char c, struct wcsh_cmd_parse *user) {
	user->token_len = 0;
	user->token[user->token_len++] = c;
}

void save_char(char c, struct wcsh_cmd_parse *user) {
	user->token[user->token_len++] = c;
}

void on_group_start(char c, struct wcsh_cmd_parse *user) {
	user->token_len = 0;
}

void save_token(char c, struct wcsh_cmd_parse *user) {
	user->token[user->token_len] = 0;
	user->token_list[user->token_list_len++] = strdup(user->token);
}

void on_command_end(char c, struct wcsh_cmd_parse *user) {
	unsigned u;
	if (user->token_list_len > 0) {
		wcsh_program_append((int)user->token_list_len, user->token_list);
	}

	for (u = 0 ; u < user->token_list_len ; u++)
		free(user->token_list[u]);

	user->token_list_len = 0;
}

void save_token_end_command(char c, struct wcsh_cmd_parse *user) {
	save_token(c, user);
	on_command_end(c, user);
}

#if 0
static struct fsm_state_infos wcsh_parse_script[] = {
	[STATE_BLANK] = {
		.accept = 1,
		.transitions = (struct fsm_transition_infos[]) {
			{MATCH("\t "),      .next = STATE_BLANK},
			{MATCH("'"),        .next = STATE_GROUP, EXEC(on_group_start)},
			{MATCH("#"),        .next = STATE_COMMENT},
			{MATCH("\n;"),      .next = STATE_BLANK, EXEC(on_command_end)},
			{MATCH(token_first_char), .next = STATE_TOKEN, EXEC(on_token_start)},
			{}
		},
	},
	[STATE_TOKEN] = {
		.accept = 1,
		.transitions = (struct fsm_transition_infos[]) {
			{MATCH("\t "),      .next = STATE_BLANK, EXEC(save_token)},
			{MATCH("#"),        .next = STATE_COMMENT, EXEC(save_token)},
			{MATCH("\n;"),      .next = STATE_BLANK, EXEC(save_token_end_command)},
			{MATCH(token_char), .next = STATE_TOKEN, EXEC(save_char)},
			{}
		},
	},
	[STATE_GROUP] = {
		.transitions = (struct fsm_transition_infos[]) {
			{MATCH("'"),        .next = STATE_GROUP_END, EXEC(save_token)},
			{MATCH("\\"),       .next = STATE_ESCAPE},
			{MATCH(group_char), .next = STATE_GROUP,     EXEC(save_char)},
		},
	},
	[STATE_ESCAPE] = {
		.transitions = (struct fsm_transition_infos[]) {
			{MATCH("'\\"),      .next = STATE_GROUP, EXEC(save_char)},
			{}
		},
	},
	[STATE_GROUP_END]  = {
		.accept = 1,
		.transitions = (struct fsm_transition_infos[]) {
			{MATCH("\n;"),      .next = STATE_BLANK, EXEC(on_command_end)},
			{MATCH("\t "),      .next = STATE_BLANK},
			{}
		},
	},
	[STATE_COMMENT]  = {
		.accept = 1,
		.transitions = (struct fsm_transition_infos[]) {
			{MATCH("\n"),      .next = STATE_BLANK, EXEC(on_command_end)},
			{MATCH_ANY,        .next = STATE_COMMENT},
			{}
		},
	},
};
#else

#include "wcsh.fsm"

#endif

void wcsh_cmdline_eval(char *buf) {
	struct wcsh_cmd_parse user = {};
	size_t len = strlen(buf);
	struct fsm *fsm = fsm_new(wcsh_parse_script, STATE_BLANK, &user);

	buf[len] = '\n';

	if (fsm_feed(fsm, buf, len + 1) != PARSE_PARSING
	    || fsm_end(fsm) != PARSE_OK)
	{
		fsm_perror(fsm);
	}

	fsm_destroy(fsm);
}

void wcsh_chunk_eval(char *buf, size_t len) {
	static struct wcsh_cmd_parse user = {};
	static struct fsm *fsm = NULL;

	if (fsm == NULL) {
		fsm = fsm_new(wcsh_parse_script, STATE_BLANK, &user);
	}

	if (len > 0) {
		if (fsm_feed(fsm, buf, len) != PARSE_PARSING) {
			fsm_perror(fsm);
		}
	} else {
		if (fsm_feed(fsm, "\n", 1) != PARSE_PARSING
			|| fsm_end(fsm) != PARSE_OK) {
			fsm_perror(fsm);
		}
	}
}
