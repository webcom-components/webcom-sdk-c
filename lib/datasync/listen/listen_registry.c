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

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>

#include "../../webcom_base_priv.h"
#include "listen_registry.h"


static void mask_listen_request(wc_context_t *ctx, wc_ds_path_t *parsed_path);
static void unmask_listen_request(wc_context_t *ctx, wc_ds_path_t *parsed_path);
static void on_listen_result(wc_context_t *ctx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data, void *user);

static int compare_listen_item_data(void *a, void *b) {
	wc_ds_path_t *pa = &(((struct listen_item *)a)->path);
	wc_ds_path_t *pb = &(((struct listen_item *)b)->path);

	return wc_datasync_path_cmp(pa, pb);
}

static void clean_listen_item_data(void *data) {
	struct listen_item *node = data;
	wc_datasync_path_cleanup(&node->path);
}

size_t listen_item_data_size(void *data) {
	struct listen_item *node = data;
	return sizeof(*node) + sizeof(*node->path.offsets) * node->path.nparts;
}

/* shallow copy, duplicate the path buffers before insert */
void copy_listen_item_data(void *from, void *to) {
	struct listen_item *node_from = from, *node_to = to;

	memcpy(node_to, node_from, listen_item_data_size(node_from));
}

static inline struct listen_item *listen_item_from_path(wc_ds_path_t *parsed_path) {
	return ((void *)parsed_path) - offsetof(struct listen_item, path);
}

struct listen_registry* listen_registry_new() {
	struct listen_registry* ret;
	ret = malloc(sizeof *ret);
	ret->list = avl_new(
			(avl_key_cmp_f) compare_listen_item_data,
			(avl_data_copy_f) copy_listen_item_data,
			(avl_data_size_f) listen_item_data_size,
			(avl_data_cleanup_f) clean_listen_item_data);

	return ret;
}

void listen_registry_destroy(struct listen_registry* lr) {
	avl_destroy(lr->list);
	free(lr);
}

static int is_masked(struct listen_registry* lr, wc_ds_path_t *parsed_path) {
	unsigned nparts;
	struct listen_item *li;
	struct listen_item *k = listen_item_from_path(parsed_path);
	int ret = 0;

	nparts = parsed_path->nparts;

	for (parsed_path->nparts = 0 ; parsed_path->nparts < nparts ; parsed_path->nparts++) {
		if ((li = avl_get(lr->list, k)) != NULL
				&& (li->status == LISTEN_ACTIVE
						|| li->status == LISTEN_REQUIRED
						|| li->status == LISTEN_PENDING)) {
			ret = 1;
			break;
		}
	}

	parsed_path->nparts = nparts;
	return ret;
}

void wc_listen_suspend_all(wc_context_t *ctx) {
	struct listen_item *li;
	struct avl_it it;
	struct listen_registry* lr = ctx->datasync.listen_reg;

	avl_it_start(&it, lr->list);

	while ((li = avl_it_next(&it)) != NULL) {
		if (li->status == LISTEN_ACTIVE || li->status == LISTEN_PENDING) {
			li->status = LISTEN_REQUIRED;
		}
	}
}

void wc_listen_resume_all(wc_context_t *ctx) {
	struct listen_item *li;
	struct avl_it it;
	struct listen_registry* lr = ctx->datasync.listen_reg;

	avl_it_start(&it, lr->list);

	while ((li = avl_it_next(&it)) != NULL) {
		if (li->status == LISTEN_REQUIRED || li->status == LISTEN_PENDING) {
			li->status = LISTEN_PENDING;
			wc_datasync_listen(ctx, wc_datasync_path_to_str(&li->path), on_listen_result, li);
		}
	}
}

struct listen_item *listen_registry_add(struct listen_registry* lr, char *path) {
	struct listen_item * ret;

	wc_ds_path_t *parsed_path = wc_datasync_path_new(path);
	ret = listen_registry_add_ex(lr, parsed_path);
	wc_datasync_path_destroy(parsed_path);
	return ret;
}

struct listen_item *listen_registry_add_ex(struct listen_registry* lr, wc_ds_path_t *parsed_path) {
	struct listen_item *li, *k = listen_item_from_path(parsed_path);

	li = avl_get(lr->list, k);
	if (li == NULL) {
		li = alloca(LISTEN_ITEM_STRUCT_MAX_SIZE);
		li->ref = 1;
		li->stamp = 0;
		li->status = is_masked(lr, parsed_path) ? LISTEN_MASKED : LISTEN_REQUIRED;
		wc_datasync_path_copy(parsed_path, &li->path);
		li = avl_insert(lr->list, li);
	} else {
		li->ref++;
	}

	return li;
}

int listen_registry_unref(struct listen_registry* lr, wc_ds_path_t *parsed_path, unsigned n_unref) {
	int ret = 0;
	struct listen_item *li, *k = listen_item_from_path(parsed_path);

	li = avl_get(lr->list, k);

	if (li != NULL) {
		assert(li->ref >= n_unref);
		li->ref -= n_unref;
		if (li->ref > 0) {
			ret = 0;
		} else {
			avl_remove(lr->list, k);
			ret = 1;
		}
	}
	return ret;
}

static void on_listen_result(wc_context_t *ctx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data, void *user) {
	(void)ctx,(void)id,(void)type,(void)reason,(void)data;
	struct listen_item *li = user;
	if (status == WC_REQ_OK) {
		li->status = LISTEN_ACTIVE;
	} else {
		li->status = LISTEN_FAILED;
	}
}

void wc_datasync_watch(wc_context_t *ctx, char *path) {
	struct listen_item *li;
	wc_ds_path_t *parsed_path = wc_datasync_path_new(path);

	li = listen_registry_add_ex(ctx->datasync.listen_reg, parsed_path);

	if (li->status == LISTEN_REQUIRED || li->status == LISTEN_FAILED)
	{
		if (ctx->datasync.state == WC_CNX_STATE_CONNECTED) {
			li->status = LISTEN_PENDING;
			wc_datasync_listen(ctx, wc_datasync_path_to_str(parsed_path), on_listen_result, li);
		} else {
			li->status = LISTEN_REQUIRED;
		}
		mask_listen_request(ctx, parsed_path);
	}
	wc_datasync_path_destroy(parsed_path);
}

static void unmask_listen_request(wc_context_t *ctx, wc_ds_path_t *parsed_path) {
	struct avl_it it;
	struct listen_item *li;
	wc_ds_path_t *top_path = NULL;

	avl_it_start_at(&it, ctx->datasync.listen_reg->list, listen_item_from_path(parsed_path));

	while (
			(li = avl_it_next(&it)) != NULL
			&& wc_datasync_path_starts_with(&li->path, parsed_path)
		  )
	{
		if (top_path != NULL && wc_datasync_path_starts_with(&li->path, top_path)) {
			continue;
		} else {
			if (li->status == LISTEN_MASKED) {
				top_path = &li->path;
				if (ctx->datasync.state == WC_CNX_STATE_CONNECTED) {
					li->status = LISTEN_PENDING;
					wc_datasync_listen(ctx, wc_datasync_path_to_str(&li->path), on_listen_result, li);
				} else {
					li->status = LISTEN_REQUIRED;
				}
			}
		}
	}
}

static void mask_listen_request(wc_context_t *ctx, wc_ds_path_t *parsed_path) {
	struct avl_it it;
	struct listen_item *li;

	avl_it_start_at(&it, ctx->datasync.listen_reg->list, listen_item_from_path(parsed_path));
	avl_it_next(&it);
	while (
			(li = avl_it_next(&it)) != NULL
			&& wc_datasync_path_starts_with(&li->path, parsed_path)
		  )
	{
		if (li->status == LISTEN_ACTIVE) {
			li->status = LISTEN_MASKED;
			wc_datasync_unlisten(ctx, wc_datasync_path_to_str(&li->path), NULL, NULL);
		} else if (li->status == LISTEN_PENDING || li->status == LISTEN_REQUIRED) {
			li->status = LISTEN_MASKED;
		}
	}
}

void wc_datasync_unwatch_ex(wc_context_t *ctx, wc_ds_path_t *parsed_path, int ref_dec) {
	if (listen_registry_unref(ctx->datasync.listen_reg, parsed_path, ref_dec)) {
		wc_datasync_unlisten(ctx, wc_datasync_path_to_str(parsed_path), NULL, NULL);
		unmask_listen_request(ctx, parsed_path);
	}
}

void wc_datasync_unwatch_all(wc_context_t *ctx) {
	struct avl_it it;
	struct listen_item *li;

	avl_it_start(&it, ctx->datasync.listen_reg->list);


	while ((li = avl_it_next(&it)) != NULL) {
		wc_datasync_unlisten(ctx, wc_datasync_path_to_str(&li->path), NULL, NULL);
	}
	avl_remove_all(ctx->datasync.listen_reg->list);
}

static char *listen_status_to_str(enum listen_status status) {
	char *ret;
	switch (status) {
	case LISTEN_ACTIVE:
		ret = "ACTIVE";
		break;
	case LISTEN_FAILED:
		ret = "FAILED";
		break;
	case LISTEN_MASKED:
		ret = "MASKED";
		break;
	case LISTEN_PENDING:
		ret = "PENDING";
		break;
	case LISTEN_REQUIRED:
		ret = "REQUIRED";
		break;
	default:
		ret = "unknown";
		break;

	}
	return ret;
}

void dump_listen_registry(struct listen_registry* lr, FILE *f) {
	struct avl_it it;
	struct listen_item *li;

	avl_it_start(&it, lr->list);

	while ((li = avl_it_next(&it)) != NULL) {
		fprintf(f, "[%8.8s] %s => %u refs\n",
				listen_status_to_str(li->status),
				wc_datasync_path_to_str(&li->path),
				li->ref);
	}
}

