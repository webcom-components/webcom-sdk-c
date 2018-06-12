/*
 * webom-sdk-c
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
#include <alloca.h>

#include "path.h"

static char * empty_path_norm = "/";

wc_ds_path_t path_root = {.nparts = 0};

wc_ds_path_t *wc_datasync_path_new(const char *path) {
	struct wc_ds_path *ret = NULL;
	struct wc_ds_path *tmp;

	tmp = alloca(PATH_STRUCT_MAX_SIZE);

	if(wc_datasync_path_parse(path, tmp) == 0) {
		return NULL;
	} else {
		ret = malloc(sizeof (*ret) + tmp->nparts * sizeof (*ret->offsets));
		if (ret == NULL) {
			wc_datasync_path_cleanup(tmp);
			return NULL;
		} else {
			memcpy(ret, tmp, sizeof (*ret) + tmp->nparts * sizeof (*ret->offsets));
			return ret;
		}
	}
}

int wc_datasync_path_parse(const char *path, struct wc_ds_path *parsed) {
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
	char *p;
	size_t path_norm_len;
	unsigned nparts = 0;
	unsigned i = 0, part_begin;
	int state = 0;

	path_cpy = strdup(path);

	if (path_cpy == NULL) {
		return 0;
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
	if (nparts > 0) {
		parsed->_buf = path_cpy;

		path_norm_len = 0;
		for (i = 0 ; i < nparts ; i++) {
			parsed->offsets[i] = offsets[i];
			path_norm_len += strlen(&parsed->_buf[offsets[i]]) + 1;
		}

		parsed->_norm = malloc(path_norm_len + 1);

		parsed->_norm[path_norm_len] = 0;

		p = parsed->_norm;

		for (i = 0 ; i < nparts ; i++) {
			*p++ = '/';
			strcpy(p, &parsed->_buf[offsets[i]]);
			p += strlen(&parsed->_buf[offsets[i]]);
		}

	} else {
		free(path_cpy);
		parsed->_buf = NULL;
		parsed->_norm = empty_path_norm;
	}
	parsed->nparts = nparts;


	return 1;
}



unsigned wc_datasync_path_get_part_count(wc_ds_path_t *path) {
	return path->nparts;
}

char *wc_datasync_path_get_part(wc_ds_path_t *path, unsigned part) {
	return &path->_buf[path->offsets[part]];
}

void wc_datasync_path_cleanup(wc_ds_path_t *path) {
	if (path->nparts > 0) {
		free(path->_buf);
		free(path->_norm);
	}
}

void wc_datasync_path_destroy(wc_ds_path_t *path) {
	wc_datasync_path_cleanup(path);
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
	int cmp;
	unsigned part_a = 0, part_b = 0;

	for (part_a = 0, part_b = 0 ; part_a < a->nparts && part_b < b->nparts ; part_a++, part_b++) {
		if ((cmp = wc_datasync_key_cmp(
						wc_datasync_path_get_part(a, part_a),
						wc_datasync_path_get_part(b, part_b))))
		{
			goto end;
		}
	}

	cmp = a->nparts - b->nparts;
end:
	return cmp;
}

wc_hash_t wc_datasync_path_hash(wc_ds_path_t *path) {
	unsigned i;
	wc_hash_t hash = 5381;
	hash = (hash << 5) + hash + '/';

	for (i = 0 ; i < wc_datasync_path_get_part_count(path) ; i++) {
		wc_djb2_hash_update(wc_datasync_path_get_part(path, i), &hash);
		hash = (hash << 5) + hash + '/';
	}

	return hash;
}

int wc_datasync_path_starts_with(wc_ds_path_t *path, wc_ds_path_t *prefix) {
	int ret = 0;
	unsigned u;

	if (path->nparts >= prefix->nparts) {
		for (u = 0 ; u < wc_datasync_path_get_part_count(prefix) ; u++) {
			if (strcmp(wc_datasync_path_get_part(path, u), wc_datasync_path_get_part(prefix, u)) != 0) {
				goto end;
			}
		}
		ret = 1;
	}
end:
	return ret;
}

void wc_datasync_path_copy(const wc_ds_path_t *from, wc_ds_path_t *to) {
	size_t path_buf_len;

	to->nparts = from->nparts;
	if (from->nparts > 0) {
		path_buf_len = from->offsets[from->nparts - 1] + strlen(from->_buf + from->offsets[from->nparts - 1]);
		to->_buf = malloc(path_buf_len + 1);
		memcpy(to->_buf, from->_buf, path_buf_len + 1);
		memcpy(to->offsets, from->offsets, from->nparts * sizeof(*from->offsets));

		to->_norm = strdup(from->_norm);
	} else {
		to->_buf = NULL;
		to->_norm = empty_path_norm;
	}
}

char *wc_datasync_path_to_str(wc_ds_path_t *path) {
	return path->_norm;
}
