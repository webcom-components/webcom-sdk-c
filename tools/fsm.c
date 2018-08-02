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
#include <stdio.h>

#include "fsm.h"

struct fsm {
	struct fsm_state_infos *fsm;
	enum parse_result ps;
	int state;
	void *user;
	unsigned stream_index;
	unsigned line;
	unsigned col;
};

/**
 * Performs a binary search of the given character in the given character array
 * @param needle the character to look for
 * @param haystack the character array, sorted in ascending order
 * @param len the lengh of the array
 * @return 1 if the character was found in the array, 0 otherwise
 */
static int binsearch(char needle, char *haystack, size_t len) {
	int min, max;
	int mid;
	int cmp;

	if (len != 0) {
		min = 0;
		max = len - 1;
		while (min <= max) {
			mid = min + (max - min) / 2;

			cmp = (int)haystack[mid] - (int)needle;

			if (cmp < 0) {
				min = mid + 1;
			} else if (cmp > 0) {
				max = mid - 1;
			} else {
				return 1;
			}
		}
	}
	return 0;
}

struct fsm *fsm_new(struct fsm_state_infos *fsm, int init_state, void *user) {
	struct fsm *res;

	res = malloc(sizeof(*res));
	res->fsm = fsm;
	res->state = init_state;
	res->user = user;
	res->ps = PARSE_PARSING;
	res->stream_index = 0;
	res->line = 1;
	res->col = 0;

	return res;
}

enum parse_result fsm_feed(struct fsm *fsm, char *buf, size_t len) {
	size_t pos;
	int found;
	struct fsm_transition_infos *tr;

	for (pos = 0 ; pos < len ; pos++) {
		if (buf[pos] == '\n') {
			fsm->line++;
			fsm->col = 0;
		}

		fsm->stream_index++;
		fsm->col++;

		found = 0;

		for (tr = (fsm->fsm[fsm->state].transitions) ; tr->valid ; tr++) {
			if (tr->match == NULL
			    || binsearch(buf[pos], tr->match, tr->match_len)
			   ) {
				found = 1;
				break;
			}
		}
		if (found) {
			if (tr->exec) {
				tr->exec(buf[pos], fsm->user);
			}
			fsm->state = tr->next;
		} else {
			fsm->ps = PARSE_UNEXPECTED_INPUT;
			break;
		}
	}
	return fsm->ps;
}

enum parse_result fsm_end(struct fsm *fsm) {
	if (fsm->fsm[fsm->state].accept) {
		fsm->ps = PARSE_OK;
	} else {
		fsm->ps = PARSE_UNEXPECTED_END;
	}
	return fsm->ps;
}

void fsm_destroy(struct fsm *fsm) {
	free(fsm);
}

void fsm_perror(struct fsm *fsm) {
	if (fsm->ps == PARSE_UNEXPECTED_INPUT) {
		fprintf(stderr, "Parse error at character %u: unexpected character\n", fsm->col);
	} else {
		fprintf(stderr, "Parse error at character %u: unexpected end of input\n", fsm->col);
	}
}
