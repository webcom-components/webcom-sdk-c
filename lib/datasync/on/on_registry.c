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

#include "on_registry.h"
#include "../path.h"
#include "../json.h"
#include "../../collection/avl.h"

struct on_registry {
	avl_t *on_value;
	avl_t *on_child[ON_CHILD_REMOVED + 1];
};

static int compare_on_value_data(void *a, void *b) {
	struct on_value_sub *sub_a = a, *node_b = b;
	return wc_datasync_path_cmp(&sub_a->path, &node_b->path);
}

static void clean_on_value_data(void *data) {
	struct on_value_sub *node = data;
	wc_datasync_path_cleanup(&node->path);
}

size_t on_value_data_size(void *data) {
	struct on_value_sub *node = data;
	return sizeof(*node) + sizeof(*node->path.offsets) * node->path.nparts;
}

void copy_on_value_data(void *from, void *to) {
	struct on_value_sub *node_from = from, *node_to = to;
	size_t path_buf_len;

	memcpy(node_to, node_from, on_value_data_size(node_from));

	if (node_from->path.nparts > 0) {
		path_buf_len = node_from->path.offsets[node_from->path.nparts - 1] + strlen(node_from->path._buf + node_from->path.offsets[node_from->path.nparts - 1]);
		node_to->path._buf = malloc(path_buf_len + 1);
		memcpy(node_to->path._buf, node_from->path._buf, path_buf_len + 1);
	}

}


static int compare_on_child_data(void *a, void *b) {
	struct on_child_sub *sub_a = a, *node_b = b;
	return wc_datasync_path_cmp(&sub_a->path, &node_b->path);
}

static void clean_on_child_data(void *data) {
	struct on_child_sub *node = data;
	wc_datasync_path_cleanup(&node->path);
}

size_t on_child_data_size(void *data) {
	struct on_child_sub *node = data;
	return sizeof(*node) + sizeof(*node->path.offsets) * node->path.nparts;
}

void copy_on_child_data(void *from, void *to) {
	struct on_child_sub *node_from = from, *node_to = to;
	memcpy(node_to, node_from, on_value_data_size(node_from));
	node_to->path._buf = strdup(node_from->path._buf);
}

struct on_registry *on_registry_new() {
	struct on_registry *ret = NULL;
	int i;

	ret = malloc(sizeof(*ret));

	ret->on_value = avl_new(
					(avl_key_cmp_f) compare_on_value_data,
					(avl_data_copy_f) copy_on_value_data,
					(avl_data_size_f) on_value_data_size,
					(avl_data_cleanup_f) clean_on_value_data);

	for (i = 0; i < ON_CHILD_REMOVED + 1; i++) {
		ret->on_child[i] = avl_new(
						(avl_key_cmp_f) compare_on_child_data,
						(avl_data_copy_f) copy_on_child_data,
						(avl_data_size_f) on_child_data_size,
						(avl_data_cleanup_f) clean_on_child_data);
	}

	return ret;
}

void on_registry_add_on_value(struct on_registry* reg, data_cache_t *cache, char *path, on_value_f cb) {
	struct on_value_sub *sub, *tmp;
	struct treenode *node;
	treenode_hash_t *hash;

	sub = alloca(ON_CHILD_STRUCT_MAX_SIZE);

	wc_datasync_path_parse(path, &sub->path);

	tmp = avl_get(reg->on_value, sub);

	if (tmp == NULL) {
		sub->cb = cb;
		node = data_cache_get(cache, path);

		hash = treenode_hash_get(node);

		if (hash != NULL) {
			sub->hash = *hash;
		} else {
			sub->hash = (treenode_hash_t){.bytes={0}};
		}
		avl_insert(reg->on_value, sub);
	} else {
		// TODO, sub->cb should be a list of functions
		tmp->cb = cb;
	}

	wc_datasync_path_cleanup(&sub->path);
}

static void on_value_trigger_maybe(struct on_value_sub *sub, data_cache_t *cache) {
	struct treenode *cached_value;
	treenode_hash_t *hash;
	char *data_snapshot;
	int data_len;

	if (sub == NULL) return;

	cached_value = data_cache_get_parsed(cache, &sub->path);
	hash = treenode_hash_get(cached_value);


	if(!treenode_hash_eq(&sub->hash, hash)) {
		data_len = treenode_to_json_len(cached_value);
		data_snapshot = malloc(data_len + 1);
		treenode_to_json(cached_value, data_snapshot);
		sub->cb(data_snapshot);
		if (hash != NULL) {
			sub->hash = *hash;
		} else {
			sub->hash = (treenode_hash_t){.bytes={0}};
		}
		free(data_snapshot);
	}

}

void on_registry_dispatch_on_value_ex(struct on_registry* reg, data_cache_t *cache, wc_ds_path_t *parsed_path) {
	struct on_value_sub *sub, *key;
	unsigned u, nparts;
	struct avl_it it;

	key = ((void *)parsed_path) - offsetof(struct on_value_sub, path);

	nparts = key->path.nparts;

	/* try to trigger the subscriptions starting from the cache root
	 * e.g.:
	 *     on_registry_dispatch_on_value("/foo/bar/baz");
	 *   will look for subscriptions for the following paths:
	 *     /
	 *     /foo/
	 *     /foo/bar/
	 */
	for (u = 0 ; u < nparts ; u++) {
		key->path.nparts = u;
		sub = avl_get(reg->on_value, key);
		on_value_trigger_maybe(sub, cache);
	}

	key->path.nparts = nparts;

	/* then try to trigger any subscription "higher or equal" to the current path:
	 * e.g.:
	 *     on_registry_dispatch_on_value("/foo/bar/baz");
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

	avl_it_start_at(&it, reg->on_value, key);

	while ((sub = avl_it_next(&it)) != NULL && wc_datasync_path_starts_with(&sub->path, &key->path)) {
		on_value_trigger_maybe(sub, cache);
	}
}

void on_registry_dispatch_on_value(struct on_registry* reg, data_cache_t *cache, char *path) {
	wc_ds_path_t *parsed_path;

	parsed_path = wc_datasync_path_new(path);

	on_registry_dispatch_on_value_ex(reg, cache, parsed_path);

	wc_datasync_path_cleanup(parsed_path);
}


void on_registry_destroy(struct on_registry *reg) {
	int i;

	for (i = 0; i < ON_CHILD_REMOVED + 1; i++) {
		avl_destroy(reg->on_child[i]);
	}

	avl_destroy(reg->on_value);

	free(reg);
}
