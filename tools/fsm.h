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

#ifndef TOOLS_FSM_H_
#define TOOLS_FSM_H_

#include <stddef.h>

enum parse_result {
	PARSE_PARSING,
	PARSE_OK,
	PARSE_UNEXPECTED_END,
	PARSE_UNEXPECTED_INPUT
};

struct fsm_transition_infos {
	int next;
	unsigned char valid:1;
	unsigned char match_len;
	char *match;
	void (*exec)(char c, void *user);
};

struct fsm_state_infos {
	unsigned accept:1; /**< whether or not this state is an accept state */
	struct fsm_transition_infos *transitions;
};

struct fsm;

/* FSM "DSL" */
#define MATCH(_static_str) .valid = 1, .match = _static_str, .match_len = sizeof(_static_str) - 1
#define MATCH_ANY .valid = 1, .match = NULL, .match_len = 0
#define NEXT(_s) .next_state = _s
#define FSM(_name)        struct fsm_state_infos _name[] = {
#define STATE(_s)         [_s]={
#define ACCEPT            .accept=1,
#define TRANSITIONS       .transitions = (struct fsm_transition_infos[]){
#define WHEN(_static_str) { .valid = 1, .match = _static_str, .match_len = sizeof(_static_str) - 1,
#define ELSE              { .valid = 1, .match = NULL, .match_len = 0,
#define EXEC(_fn)         .exec = (void (*)(char, void*))(_fn),
#define GOTO(_s)          .next = _s },
#define END_TRANSITIONS   {},},
#define END_STATE         },
#define END_FSM           };

struct fsm *fsm_new(struct fsm_state_infos *fsm, int init_state, void *user);
enum parse_result fsm_feed(struct fsm *fsm, char *buf, size_t len);
enum parse_result fsm_end(struct fsm *fsm);
void fsm_destroy(struct fsm *fsm);
void fsm_perror(struct fsm *fsm);

#endif /* TOOLS_FSM_H_ */
