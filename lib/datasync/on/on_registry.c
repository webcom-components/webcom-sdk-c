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

#include <string.h>
#include <alloca.h>
#include <stddef.h>

#include "../../webcom_base_priv.h"
#include "on_registry.h"
#include "../path.h"
#include "../json.h"
#include "../../collection/avl.h"
#include "../listen/listen_registry.h"


struct on_registry {
	avl_t *sub_list;
};

struct internal_hash {
	char *key;
	treenode_hash_t hash;
};


struct deleted_cb {
	on_handle_t cb;
	struct deleted_cb *next;
};
static struct deleted_cb *deleted_cb_list = NULL;

static void on_val_trig(struct on_sub *sub, data_cache_t *cache);
static void on_child_trig(struct on_sub *sub, data_cache_t *cache);



static int compare_internal_hash_data(void *a, void *b) {
	struct internal_hash *node_a = a, *node_b = b;
	return wc_datasync_key_cmp(node_a->key, node_b->key);
}

static void clean_internal_hash_data(void *data) {
	struct internal_hash *node = data;
	free(node->key);
}

size_t internal_hash_data_size(void *data) {
	struct internal_hash *node = data;
	return sizeof(*node);
}

void copy_internal_hash_data(void *from, void *to) {
	struct internal_hash *node_from = from, *node_to = to;
	memcpy(node_to, node_from, sizeof(*node_from));
}

static int compare_on_sub_data(void *a, void *b) {
	struct on_sub *sub_a = a, *node_b = b;
	return wc_datasync_path_cmp(&sub_a->path, &node_b->path);
}

static void clean_on_sub_data(void *data) {
	struct on_sub *node = data;
	struct on_cb_list *p_cb, *tmp;
	int i;

	avl_destroy(node->children_hashes);
	wc_datasync_path_cleanup(&node->path);

	for (i = 0 ; i < ON_EVENT_TYPE_COUNT ; i++) {
		p_cb = node->cb_list[i];
		while (p_cb != NULL) {
			tmp = p_cb->next;
			free(p_cb);
			p_cb = tmp;
		}
	}
}

size_t on_sub_data_size(void *data) {
	struct on_sub *node = data;
	return sizeof(*node) + sizeof(*node->path.offsets) * node->path.nparts;
}

/* /!\ shallow copies the hash list */
/* /!\ shallow copies the callback list */
void copy_on_sub_data(void *from, void *to) {
	struct on_sub *node_from = from, *node_to = to;

	memcpy(node_to, node_from, on_sub_data_size(node_from));

	wc_datasync_path_copy(&node_from->path, &node_to->path);
}

static inline struct on_sub *on_child_sub_from_path(wc_ds_path_t *parsed_path) {
	return ((void *)parsed_path) - offsetof(struct on_sub, path);
}

struct on_registry *on_registry_new() {
	struct on_registry *ret = NULL;

	ret = malloc(sizeof(*ret));

	ret->sub_list = avl_new(
					(avl_key_cmp_f) compare_on_sub_data,
					(avl_data_copy_f) copy_on_sub_data,
					(avl_data_size_f) on_sub_data_size,
					(avl_data_cleanup_f) clean_on_sub_data);

	return ret;
}

static void refresh_on_child_sub_hashes(struct on_sub *sub, struct treenode *cached_value) {
	struct internal_node_element *p_cache;
	struct internal_hash new_ih;
	struct avl_it it_cache;
	treenode_hash_t *hash;

	avl_remove_all(sub->children_hashes);
	avl_it_start(&it_cache, cached_value->uval.children);

	while ((p_cache = avl_it_next(&it_cache)) != NULL) {
		hash = treenode_hash_get(&p_cache->node);
		new_ih.key = strdup(p_cache->key);
		if (hash != NULL) {
			new_ih.hash = *hash;
		} else {
			new_ih.hash = (treenode_hash_t){.bytes={0}};
		}
		avl_insert(sub->children_hashes, &new_ih);
	}
}

on_handle_t on_registry_add(wc_context_t *ctx, enum on_event_type type, char *path, on_callback_f cb) {
	struct on_sub *sub, *tmp;
	struct on_cb_list *p_cb;
	struct treenode *snapshot;
	char *json_snapshot;
	int json_len;
	treenode_hash_t *hash;

	sub = alloca(ON_SUB_STRUCT_MAX_SIZE);

	memset(sub, 0, ON_SUB_STRUCT_MAX_SIZE);

	wc_datasync_path_parse(path, &sub->path);

	p_cb = malloc(sizeof(struct on_cb_list));
	p_cb->cb = cb;

	tmp = avl_get(ctx->datasync.on_reg->sub_list, sub);

	if (tmp == NULL) {
		sub->ctx = ctx;
		p_cb->next = NULL;

		sub->cb_list[type] = p_cb;
		sub->hash = (treenode_hash_t ) { .bytes = { 0 } };
		sub->children_hashes = avl_new(
							(avl_key_cmp_f) compare_internal_hash_data,
							(avl_data_copy_f) copy_internal_hash_data,
							(avl_data_size_f) internal_hash_data_size,
							(avl_data_cleanup_f) clean_internal_hash_data);
		p_cb->sub = avl_insert(ctx->datasync.on_reg->sub_list, sub);
	} else {
		p_cb->sub = tmp;
		p_cb->next = tmp->cb_list[type];
		tmp->cb_list[type] = p_cb;
	}

	/* test whether the callbacks should be initially called */
	if ((snapshot = data_cache_get_parsed(ctx->datasync.cache, &p_cb->sub->path)) != NULL
			|| wc_is_listening(ctx, &p_cb->sub->path)
			|| ctx->datasync.state == WC_CNX_STATE_DISCONNECTED)
	{
		if (type == ON_VALUE) {
			json_len = treenode_to_json_len(snapshot);
			json_snapshot = malloc(json_len + 1);
			treenode_to_json(snapshot, json_snapshot);
			cb(ctx, p_cb, json_snapshot, NULL, NULL);

			free(json_snapshot);

			hash = treenode_hash_get(snapshot);
			if (hash != NULL) {
				p_cb->sub->hash = *hash;
			} else {
				p_cb->sub->hash = (treenode_hash_t ) { .bytes = { 0 } };
			}
		} else if (type == ON_CHILD_ADDED) {
			struct avl_it it;
			struct internal_node_element *cur;
			char *prev = NULL;

			if (snapshot != NULL && snapshot->type == TREENODE_TYPE_INTERNAL) {
				avl_it_start(&it, snapshot->uval.children);
				while (avl_it_has_next(&it)) {
					cur = avl_it_next(&it);
					json_len = treenode_to_json_len(&cur->node);
					json_snapshot = malloc(json_len + 1);
					treenode_to_json(&cur->node, json_snapshot);
					cb(ctx, p_cb, json_snapshot, cur->key, prev);
					free(json_snapshot);
					prev = cur->key;
				}
				refresh_on_child_sub_hashes(p_cb->sub, snapshot);
			}

		}
	}

	wc_datasync_path_cleanup(&sub->path);

	return p_cb;
}

#if 0
static void on_value_trigger_maybe(struct on_sub *sub, data_cache_t *cache) {
	struct treenode *cached_value;
	treenode_hash_t *hash;
	struct on_cb_list *p_cb;
	char *data_snapshot;
	int data_len;

	if (sub == NULL) return;

	cached_value = data_cache_get_parsed(cache, &sub->path);
	hash = treenode_hash_get(cached_value);

	if(!treenode_hash_eq(&sub->hash, hash)) {
		data_len = treenode_to_json_len(cached_value);
		data_snapshot = malloc(data_len + 1);
		treenode_to_json(cached_value, data_snapshot);
		for (p_cb = sub->cb_list ; p_cb != NULL ; p_cb = p_cb->next) {
			p_cb->cb(sub->ctx, data_snapshot, NULL, NULL);
		}
		if (hash != NULL) {
			sub->hash = *hash;
		} else {
			sub->hash = (treenode_hash_t){.bytes={0}};
		}
		free(data_snapshot);
	}
}
#endif

static int datasync_key_cmp_null(struct internal_node_element *a, struct internal_hash *b) {
	if (a == NULL && b == NULL) {
		return 0;
	} else if (a == NULL) {
		return 1;
	} else if (b == NULL) {
		return -1;
	} else {
		return wc_datasync_key_cmp(a->key, b->key);
	}
}

static void mark_cb_for_deletion(on_handle_t p_cb) {
	struct deleted_cb *tmp;
	tmp = malloc(sizeof *tmp);
	tmp->cb = p_cb;
	tmp->next = deleted_cb_list;
	deleted_cb_list = tmp;
}

static void deleted_cb_gc() {
	struct deleted_cb *next;
	struct on_cb_list *p_cb;
	wc_context_t *ctx;
	struct on_sub *sub;

	while (deleted_cb_list) {
		p_cb = deleted_cb_list->cb;
		next = deleted_cb_list->next;
		sub = p_cb->sub;
		ctx = sub->ctx;
		wc_datasync_unwatch_ex(ctx, &sub->path, 1);
		on_registry_remove(ctx, &sub->path, -1, p_cb);
		free(deleted_cb_list);
		deleted_cb_list = next;
	}
}

static void trigger_on_child_cb_list(wc_context_t *ctx, struct on_sub *sub, enum on_event_type type, struct treenode *snapshot, char *cur_key, char *prev_key) {
	struct on_cb_list *p_cb;
	char *data_snapshot = NULL;
	int data_len;

	for (p_cb = sub->cb_list[type] ; p_cb != NULL ; p_cb = p_cb->next) {
		if (snapshot != NULL && data_snapshot == NULL) {
			data_len = treenode_to_json_len(snapshot);
			data_snapshot = malloc(data_len + 1);
			treenode_to_json(snapshot, data_snapshot);
		}

		if(!p_cb->cb(ctx, p_cb, snapshot == NULL ? "null" : data_snapshot, cur_key, prev_key)) {
			mark_cb_for_deletion(p_cb);
		}
	}

	if (data_snapshot != NULL) free(data_snapshot);
}

static void on_child_trig(struct on_sub *sub, data_cache_t *cache) {
	struct treenode *cached_value;
	struct internal_node_element *p_cache;
	struct internal_hash *p_sub;
	treenode_hash_t *hash;
	char *prev_cached_key;
	int cmp, refresh;
	struct avl_it it_sub, it_cache;

	cached_value = data_cache_get_parsed(cache, &sub->path);

	if (cached_value == NULL || cached_value->type != TREENODE_TYPE_INTERNAL) {
		/* then every child has been removed */
		avl_it_start(&it_sub, sub->children_hashes);
		while ((p_sub = avl_it_next(&it_sub)) != NULL) {
			trigger_on_child_cb_list(sub->ctx, sub, ON_CHILD_REMOVED, NULL, p_sub->key, NULL);
		}
		avl_remove_all(sub->children_hashes);
	} else {
		refresh = 0;
		avl_it_start(&it_cache, cached_value->uval.children);
		avl_it_start(&it_sub, sub->children_hashes);

		p_cache = avl_it_next(&it_cache);
		p_sub = avl_it_next(&it_sub);
		prev_cached_key = NULL;

		while (p_cache != NULL || p_sub != NULL) {

			cmp = datasync_key_cmp_null(p_cache, p_sub);

			if (cmp < 0) {
				refresh = 1;
				trigger_on_child_cb_list(sub->ctx, sub, ON_CHILD_ADDED, &p_cache->node, p_cache->key, prev_cached_key);
				prev_cached_key = p_cache->key;
				p_cache = avl_it_next(&it_cache);
			} else if (cmp == 0) {
				hash = treenode_hash_get(&p_cache->node);
				if (!treenode_hash_eq(hash, &p_sub->hash)) {
					trigger_on_child_cb_list(sub->ctx, sub, ON_CHILD_CHANGED, &p_cache->node, p_cache->key, prev_cached_key);
					refresh = 1;
				}
				prev_cached_key = p_cache->key;
				p_cache = avl_it_next(&it_cache);
				p_sub = avl_it_next(&it_sub);
			} else {
				refresh = 1;
				trigger_on_child_cb_list(sub->ctx, sub, ON_CHILD_REMOVED, NULL, p_sub->key, prev_cached_key);
				p_sub = avl_it_next(&it_sub);
			}
		}

		if (refresh) {
			refresh_on_child_sub_hashes(sub, cached_value);
		}
	}

}

int on_registry_remove(wc_context_t *ctx, wc_ds_path_t *path, int type_mask, on_handle_t h) {
	struct on_sub *sub, *key = on_child_sub_from_path(path);
	struct on_cb_list *p_cb, *tmp, **prev;
	int removed = 0;
	int i;

	sub = avl_get(ctx->datasync.on_reg->sub_list, key);

	if (sub != NULL) {
		for (i = 0 ; i < ON_EVENT_TYPE_COUNT ; i++) {

			if (((1 << i) & type_mask) == 0) continue;

			prev = &sub->cb_list[i];
			p_cb = *prev;
			while (p_cb) {
				if (h == NULL || p_cb == h) {
					(*prev) = p_cb->next;
					tmp = p_cb;
					p_cb = p_cb->next;

					free(tmp);
					removed++;
				} else {
					prev = &(p_cb->next);
					p_cb = p_cb->next;
				}
			}
		}

		if (!(sub->cb_list[ON_VALUE] || sub->cb_list[ON_CHILD_ADDED]
				|| sub->cb_list[ON_CHILD_REMOVED] || sub->cb_list[ON_CHILD_CHANGED])) {
			avl_remove(ctx->datasync.on_reg->sub_list, sub);
		}
	}


	return removed;
}

static void on_val_trig(struct on_sub *sub, data_cache_t *cache) {
	treenode_hash_t *cached_hash;
	struct treenode *cached_data;
	struct on_cb_list *p_cb;
	char *data_snapshot;
	int data_len;

	if (sub->cb_list[ON_VALUE] != NULL) {

		cached_data = data_cache_get_parsed(cache, &sub->path);
		cached_hash = treenode_hash_get(cached_data);

		if (!treenode_hash_eq(cached_hash, &sub->hash)) {
			data_len = treenode_to_json_len(cached_data);
			data_snapshot = malloc(data_len + 1);
			treenode_to_json(cached_data, data_snapshot);
			p_cb = sub->cb_list[ON_VALUE];
			do {
				if (!p_cb->cb(sub->ctx, p_cb, data_snapshot, NULL, NULL)) {
					mark_cb_for_deletion(p_cb);
				}
				p_cb = p_cb->next;
			} while (p_cb != NULL);
			if (cached_hash != NULL) {
				sub->hash = *cached_hash;
			} else {
				sub->hash = (treenode_hash_t ) { .bytes = { 0 } };
			}
			free(data_snapshot);
		}
	}
}

static void trig(struct on_sub *sub, data_cache_t *cache) {
	if (sub->cb_list[ON_CHILD_ADDED]
				|| sub->cb_list[ON_CHILD_CHANGED]
				|| sub->cb_list[ON_CHILD_REMOVED])
	{
		on_child_trig(sub, cache);
	}

	if (sub->cb_list[ON_VALUE]) {
		on_val_trig(sub, cache);
	}
}

void on_registry_dispatch_on_event_ex(struct on_registry* reg, data_cache_t *cache, wc_ds_path_t *parsed_path) {
	struct on_sub *sub, *key;
	unsigned u, nparts;
	struct avl_it it;

	key = on_child_sub_from_path(parsed_path);

	nparts = key->path.nparts;

	/* try to trigger the subscriptions starting from the cache root
	 * e.g.:
	 *     on_registry_dispatch_on_event_ex("/foo/bar/baz");
	 *   will look for subscriptions for the following paths:
	 *     /
	 *     /foo/
	 *     /foo/bar/
	 */
	for (u = 0 ; u < nparts ; u++) {
		key->path.nparts = u;
		sub = avl_get(reg->sub_list, key);
		if (sub != NULL) {
			trig(sub, cache);
		}
	}

	key->path.nparts = nparts;

	/* then try to trigger any subscription "higher or equal" to the current path:
	 * e.g.:
	 *     on_registry_dispatch_on_event_ex("/foo/bar/baz");
	 *   will look for subscriptions for the following paths:
	 *     /foo/bar/baz/
	 *     /foo/bar/baz/buzz/
	 *     /foo/bar/baz/fizz/
	 *     /foo/bar/baz/fizz/qux/
	 *     ...
	 *
	 * this is done by browsing the on_value list, that is conveniently sorted,
	 * from the given path, until the next path does not begin with the given
	 * path
	 */

	avl_it_start_at(&it, reg->sub_list, key);

	while ((sub = avl_it_next(&it)) != NULL && wc_datasync_path_starts_with(&sub->path, &key->path)) {
		trig(sub, cache);
	}

	deleted_cb_gc();
}

void on_registry_dispatch_on_event(struct on_registry* reg, data_cache_t *cache, char *path) {
	wc_ds_path_t *parsed_path;

	parsed_path = wc_datasync_path_new(path);

	on_registry_dispatch_on_event_ex(reg, cache, parsed_path);

	wc_datasync_path_destroy(parsed_path);

}


void on_registry_destroy(struct on_registry *reg) {
	avl_destroy(reg->sub_list);
	free(reg);
}

void dump_on_registry(struct on_registry* reg, FILE *f) {
	struct avl_it it;
	struct on_sub *sub;
	char types[4];
	avl_it_start(&it, reg->sub_list);

	while((sub = avl_it_next(&it)) != NULL) {
		types[0] = sub->cb_list[ON_VALUE] != NULL ? 'v' : '-';
		types[1] = sub->cb_list[ON_CHILD_ADDED] != NULL ? 'a' : '-';
		types[2] = sub->cb_list[ON_CHILD_REMOVED] != NULL ? 'r' : '-';
		types[3] = sub->cb_list[ON_CHILD_CHANGED] != NULL ? 'c' : '-';

		fprintf(f, "%.4s %s\n", types, wc_datasync_path_to_str(&sub->path));
	}

}
