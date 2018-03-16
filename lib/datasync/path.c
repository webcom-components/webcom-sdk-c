/*
 * ht
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "path.h"


wc_ds_path_t *wc_ds_path_new(const char *path) {
	/*
	 * Path parser states:
	 *
	 *   {0}                    {1}
	 * ((init)) ---[^/]*--> (inside_part)
	 *   | ^                   | |
	 *   | +--------[/]--------+ |
	 * [NUL]                   [NUL]
	 *   +-------->(end)<--------+
	 */
	uint16_t offsets[WC_DS_MAX_DEPTH];
	char *path_cpy;
	unsigned nparts = 0;
	unsigned i = 0, part_begin;
	int state = 0;
	struct wc_ds_path *ret = NULL;

	path_cpy = strdup(path);
	if (path_cpy == NULL) {
		goto error_strdup;
	}

	for (i = 0 ; nparts < WC_DS_MAX_DEPTH ; i++) {
		switch (state) {
		case 0: /* init */
			switch (path_cpy[i]) {
			case '/':
				break;
			case '\0':
				goto end;
				break;
			default:
				state = 1;
				part_begin = i;
				break;
			}
			break;
		case 1: /* inside_part */
			switch (path_cpy[i]) {
			case '/':
				if (i > part_begin) {
					offsets[nparts++] = part_begin;
					path_cpy[i] = 0;
				}
				state = 0;
				break;
			case '\0':
				if (i > part_begin) {
					offsets[nparts++] = part_begin;
				}
				goto end;
				break;
			default:
				break;
			}
			break;
		}
	}
	/*
	___                                        ___
	   \                                      /
	    \____________________________________/
	              (spaghetti plate)
	 */
end:

	ret = malloc(sizeof (*ret) + nparts * sizeof (*ret->offsets));
	if (ret == NULL) {
		goto error_malloc;
	}

	ret->_buf = path_cpy;
	ret->nparts = nparts;

	for (i = 0 ; i < nparts ; i++) {
		ret->offsets[i] = offsets[i];
	}

	return ret;

error_malloc:
	free(path_cpy);
error_strdup:
	return ret;
}

unsigned wc_datasync_path_get_part_count(wc_ds_path_t *path) {
	return path->nparts;
}

char *wc_datasync_path_get_part(wc_ds_path_t *path, unsigned part) {
	return &path->_buf[path->offsets[part]];
}

void wc_datasync_path_destroy(wc_ds_path_t *path) {
	free(path->_buf);
	free(path);
}

int wc_datasync_key_cmp(const char *sa, const char *sb) {
	int64_t ia;
	int64_t ib;
	int read, ret = 0, alen, blen;

	char status = 0;

	alen = (int)strlen(sa);
	blen = (int)strlen(sb);

	if (sscanf(sa, "%"SCNi64"%n", &ia, &read) == 1 && read == alen) {
		status |= 1;
	}
	if (sscanf(sb, "%"SCNi64"%n", &ib, &read) == 1 && read == blen) {
		status |= 2;
	}

	switch (status) {
	case 0:
		ret = strcmp(sa, sb);
		break;
	case 1:
		ret = -1;
		break;
	case 2:
		ret = 1;
		break;
	case 3:
		ret = (ia == ib) ? alen - blen : ia - ib;
		break;
	}

	return ret;
}

int wc_datasync_path_cmp(wc_ds_path_t *a, wc_ds_path_t *b) {
	int cmp = 0;
	unsigned part_a = 0, part_b = 0;

	while (part_a < a->nparts && part_b < b->nparts) {
		cmp = wc_datasync_key_cmp(wc_datasync_path_get_part(a, part_a), wc_datasync_path_get_part(b, part_b));

		if (cmp != 0) {
			goto done;
		}

		part_a++;
		part_b++;
	}

	if (a->nparts > b->nparts) {
		cmp = 1;
	} else if (b->nparts > a->nparts) {
		cmp = -1;
	}

done:
	return cmp;
}
