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

#include <string.h>
#include <stdint.h>

#include "webcom-c/webcom.h"
#include "webcom_priv.h"

typedef struct wc_on_data_handler {
	char *path;
	wc_on_data_callback_t callback;
	void *user;
	struct wc_on_data_handler *next;
} wc_on_data_handler_t;

static int path_eq(char *p1, char *p2) {
	do {
		while (*p1 == '/') p1++;
		while (*p2 == '/') p2++;
		while (*p1 && *p1 != '/' && *p2 && *p2 != '/') {
			if (*p1 != *p2) {
				return 0;
			}
			p1++;
			p2++;
		}
		if ((*p1 != 0 && *p1 != '/') || (*p2 != 0 && *p2 != '/')) {
			return 0;
		}
	} while (*p1 && *p2);
	return 1;
}

/* djb2 hash */
static uint32_t str_hash_update(unsigned char **str, uint32_t hash) {
	while (**str != '/' && **str != '\0') {
		hash = ((hash << 5) + hash) + **str;
		(*str)++;
	}

	return ((hash << 5) + hash) + '/';
}

static uint32_t path_hash(char *path) {
	uint32_t hash = 177620; /* '/' hashed through djb2 */
	while (*path) {
		while (*path == '/') {
			path++;
		}
		if (*path) {
			hash = str_hash_update((unsigned char **)&path, hash);
		}
	}
	return hash;
}

/*
 * calls
 *
 *     __func(wc_cnx_t *__cnx, char *__path, char *__path_chunk, __hash, ...)
 *
 * for each chunk on the given path.
 *
 * Example: if "/aaa/bbb/ccc/" is given as input path, this macro will
 * sequentially call
 *
 * - __func(__cnx, "/aaa/bbb/ccc/", "/aaa/", H("/aaa/"), ...)
 * - __func(__cnx, "/aaa/bbb/ccc/", "/aaa/bbb/", H("/aaa/bbb/"), ...)
 * - __func(__cnx, "/aaa/bbb/ccc/", "/aaa/bbb/ccc/", H("/aaa/bbb/ccc/"), ...)
 *
 */
#define FOR_EACH_PATH_CHUNK_HASH(__func, __cnx, __path, ...)                          \
	do {                                                                              \
		char *__path_ptr = strdup(__path), *__path_chunk = __path_ptr, __bak;         \
		uint32_t __hash = 177620;                                                     \
		while (*__path_ptr) {                                                         \
			while (*__path_ptr == '/') {                                              \
				__path_ptr++;                                                         \
			}                                                                         \
			if (*__path_ptr) {                                                        \
				__hash = str_hash_update((unsigned char **)&__path_ptr, __hash);      \
				__bak = *__path_ptr;                                                  \
				*__path_ptr = '\0';                                                   \
				__func(__cnx, __path, __path_chunk, __hash, ## __VA_ARGS__);          \
				*__path_ptr = __bak;                                                  \
			}                                                                         \
		}                                                                             \
		free(__path_chunk);                                                           \
	} while (0)

void wc_on_data(wc_context_t *cnx, char *path, wc_on_data_callback_t callback, void *user) {
	wc_on_data_handler_t *new_h, *tmp;
	uint32_t slot;

	new_h = malloc(sizeof(wc_on_data_handler_t));

	new_h->path = strdup(path);
	new_h->callback = callback;
	new_h->user = user;

	slot = path_hash(path) % (1 << DATA_HANDLERS_HASH_FACTOR);

	tmp = cnx->handlers[slot];

	new_h->next = tmp;
	cnx->handlers[slot] = new_h;
}

void wc_off_data(wc_context_t *cnx, char *path, wc_on_data_callback_t callback) {
	wc_on_data_handler_t *h, *tmp, **prev;
	uint32_t slot;

	slot = path_hash(path) % (1 << DATA_HANDLERS_HASH_FACTOR);

	prev = &cnx->handlers[slot];
	h = *prev;

	while (h) {
		if (path_eq(h->path, path)
				&& (callback == NULL || callback == h->callback)) {
			(*prev) = h->next;
			tmp = h;
			h = h->next;
			free(tmp->path);
			free(tmp);
		} else {
			prev = &(h->next);
			h = h->next;
		}
	}
}

static void _dispatch_helper(wc_context_t *cnx, char *full_path, char *path_chunk, uint32_t hash, ws_on_data_event_t event, char *json_data) {
	wc_on_data_handler_t *p, *next;

	p = cnx->handlers[hash % (1 << DATA_HANDLERS_HASH_FACTOR)];

	while (p != NULL) {
		next = p->next;
		if (path_eq(path_chunk, p->path)) {
			p->callback(cnx,
					event,
					full_path,
					json_data,
					p->user);
		}
		p = next;
	}
}

void wc_on_data_dispatch(wc_context_t *cnx, wc_push_t *push) {
	ws_on_data_event_t event;
	char *updated_path;
	char *updated_data;

	if (push->type == WC_PUSH_DATA_UPDATE_PUT) {
		updated_path = push->u.update_put.path;
		updated_data = push->u.update_put.data;
		event = WC_ON_DATA_PUT;
	} else if(push->type == WC_PUSH_DATA_UPDATE_MERGE) {
		updated_path = push->u.update_merge.path;
		updated_data = push->u.update_merge.data;
		event = WC_ON_DATA_MERGE;
	} else {
		return;
	}

	FOR_EACH_PATH_CHUNK_HASH (
			_dispatch_helper, cnx, updated_path, event, updated_data
	);
}

void wc_free_on_data_handlers(wc_on_data_handler_t **table) {
	wc_on_data_handler_t *p, *q;
	size_t i;

	for (i = 0 ; i < (1 << DATA_HANDLERS_HASH_FACTOR) ; i++) {
		p = table[i];
		while(p) {
			q = p->next;
			free(p->path);
			free(p);
			p = q;
		}
	}
}
