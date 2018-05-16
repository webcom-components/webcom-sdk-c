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
#include <assert.h>
#include <json-c/json.h>


#include "../path.h"

#include "../../collection/avl.h"
#include "../on/on_registry.h"

#include "treenode_cache.h"
#include "treenode.h"

static void data_cache_mkpath_w(data_cache_t *cache, wc_ds_path_t *path);
static struct treenode *data_cache_get_r(struct treenode *node, wc_ds_path_t *path, unsigned depth);

data_cache_t *data_cache_new() {
	data_cache_t *ret = calloc(1, sizeof (*ret));

	struct treenode *root = calloc(1, sizeof (*root));

	root->type = TREENODE_TYPE_LEAF_NULL;
	ret->root = root;

	ret->registry = on_registry_new();

	return ret;
}

/* warning: this function leaves the cache in a transient state where its root
 * is the NULL pointer, it must be linked to an actual treenode right after */
static void data_cache_empty(data_cache_t *cache) {
	treenode_destroy(cache->root);
	cache->root = NULL;
}

void data_cache_destroy(data_cache_t *cache) {
	data_cache_empty(cache);
	on_registry_destroy(cache->registry);
	free(cache);
}

void data_cache_set_leaf(data_cache_t *cache, char *path, enum treenode_type type, union treenode_value uval) {
	assert(type != TREENODE_TYPE_INTERNAL);

	wc_ds_path_t *parsed_path = wc_datasync_path_new(path);
	unsigned nparts = wc_datasync_path_get_part_count(parsed_path);

	if (nparts == 0) {
		data_cache_empty(cache);

		/* replace the root */
		switch (type) {
		case TREENODE_TYPE_LEAF_NUMBER:
			cache->root = treenode_new_number(uval.number);
			break;
		case TREENODE_TYPE_LEAF_BOOL:
			cache->root = treenode_new_bool(uval.bool);
			break;
		case TREENODE_TYPE_LEAF_STRING:
			cache->root = treenode_new_string(uval.str);
			break;
		case TREENODE_TYPE_LEAF_NULL:
			cache->root = treenode_new_null();
			break;
		default:
			break;
		}
	} else {
		struct treenode *n;
		char *key;

		parsed_path->nparts--; /* push(hack) */
		data_cache_mkpath_w(cache, parsed_path);
		n = data_cache_get_r(cache->root, parsed_path, 0);
		parsed_path->nparts++; /* pop() */

		key = wc_datasync_path_get_part(parsed_path, nparts - 1);
		internal_remove(n, key);

		switch (type) {
		case TREENODE_TYPE_LEAF_NUMBER:
			internal_add_new_number(n, key, uval.number);
			break;
		case TREENODE_TYPE_LEAF_BOOL:
			internal_add_new_bool(n, key, uval.bool);
			break;
		case TREENODE_TYPE_LEAF_STRING:
			internal_add_new_string(n, key, uval.str);
			break;
		case TREENODE_TYPE_LEAF_NULL:
			internal_add_new_null(n, key);
			break;
		default:
			break;
		}
	}
	wc_datasync_path_destroy(parsed_path);
}

static struct treenode *treenode_from_json_val(json_object *val) {
	assert(json_object_get_type(val) != json_type_object
			&& json_object_get_type(val) != json_type_array);

	struct treenode *ret = NULL;

	switch (json_object_get_type(val)) {
	case json_type_boolean:
		ret = treenode_new_bool(json_object_get_boolean(val) == FALSE ? TN_FALSE : TN_TRUE);
		break;
	case json_type_double:
		ret = treenode_new_number(json_object_get_double(val));
		break;
	case json_type_int:
		ret = treenode_new_number((double)json_object_get_int64(val));
		break;
	case json_type_string:
		ret = treenode_new_string((char*)json_object_get_string(val));
		break;
	default:
		break;
	}

	return ret;
}

static void internal_add_from_json_val(struct treenode *root, char *key, json_object *val) {
	assert(root->type == TREENODE_TYPE_INTERNAL);
	assert(json_object_get_type(val) != json_type_object
			&& json_object_get_type(val) != json_type_array);

	switch (json_object_get_type(val)) {
	case json_type_boolean:
		internal_add_new_bool(root, key, json_object_get_boolean(val) == FALSE ? TN_FALSE : TN_TRUE);
		break;
	case json_type_double:
		internal_add_new_number(root, key, json_object_get_double(val));
		break;
	case json_type_int:
		internal_add_new_number(root, key, (double)json_object_get_int64(val));
		break;
	case json_type_string:
		internal_add_new_string(root, key, (char*)json_object_get_string(val));
		break;
	case json_type_null:
		avl_remove(root->uval.children, &key);
		break;
	default:
		break;
	}
}

static void data_cache_set_r(data_cache_t *cache, struct treenode *root, char *key, json_object *value);

void data_cache_set(data_cache_t *cache, char *path, char *json_doc) {
	wc_ds_path_t *parsed_path;
	json_object *parsed_json;

	parsed_path = wc_datasync_path_new(path);
	parsed_json = json_tokener_parse(json_doc);

	data_cache_set_ex(cache, parsed_path, parsed_json);

	json_object_put(parsed_json);
	wc_datasync_path_destroy(parsed_path);

}

void data_cache_set_ex(data_cache_t *cache, wc_ds_path_t * parsed_path, json_object *parsed_json) {
	unsigned nparts;

	nparts = wc_datasync_path_get_part_count(parsed_path);
	if (nparts == 0) {
		data_cache_empty(cache);

		data_cache_set_r(cache, NULL, NULL, parsed_json);
	} else {
		struct treenode *n;

		parsed_path->nparts--; /* push(hack) */
		data_cache_mkpath_w(cache, parsed_path);
		n = data_cache_get_r(cache->root, parsed_path, 0);
		parsed_path->nparts++; /* pop() */

		data_cache_set_r(cache, n, wc_datasync_path_get_part(parsed_path, nparts - 1), parsed_json);
	}
}

static void data_cache_set_r(data_cache_t *cache, struct treenode *root, char *key, json_object *value) {
	int i;
	struct treenode *sub;
	char sidx[32];

	sidx[31] = 0;

	switch(json_object_get_type(value)) {
	case json_type_array:
		if (root == NULL) {
			sub = treenode_new_internal();
			cache->root = sub;
		} else {
			sub = internal_add_new_internal(root, key);
		}

		for (i = 0 ; i < json_object_array_length(value) ; i++) {
			snprintf(sidx, sizeof(sidx) - 1, "%d", i);
			data_cache_set_r(cache, sub, sidx, json_object_array_get_idx(value, i));
		}
		break;
	case json_type_object:
		if (root == NULL) {
			sub = treenode_new_internal();
			cache->root = sub;
		} else {
			sub = internal_add_new_internal(root, key);
		}

		json_object_object_foreach(value, obj_key, obj_val) {
			data_cache_set_r(cache, sub, obj_key, obj_val);
		}
		break;
	case json_type_boolean:
	case json_type_double:
	case json_type_int:
	case json_type_null:
	case json_type_string:
		if (root == NULL) {
			sub = treenode_from_json_val(value);
			cache->root = sub;
		} else {
			internal_add_from_json_val(root, key, value);
		}
		break;
	}
}

static void data_cache_mkpath_w(data_cache_t *cache, wc_ds_path_t *path) {
	struct treenode *cur;
	struct treenode *prev;
	char *key;
	unsigned i;

	if (cache->root->type != TREENODE_TYPE_INTERNAL) {
		treenode_destroy(cache->root);

		cache->root = treenode_new_internal();
	}

	prev = cache->root;

	cache->root->hash_cached = 0;

	for (i = 0 ; i < wc_datasync_path_get_part_count(path) ; i++) {
		key = wc_datasync_path_get_part(path, i);
		cur = internal_get(prev, key);

		if (cur == NULL) {
			cur = internal_add_new_internal(prev, key);
		} else if (cur->type != TREENODE_TYPE_INTERNAL) {
			internal_remove(prev, key);

			cur = internal_add_new_internal(prev, key);
		} else {
			cur->hash_cached = 0;
		}
		prev = cur;
	}
}

void data_cache_mkpath(data_cache_t *cache, char *path) {
	wc_ds_path_t *parsed_path = wc_datasync_path_new(path);
	data_cache_mkpath_w(cache, parsed_path);
	wc_datasync_path_destroy(parsed_path);
}

static struct treenode *data_cache_get_r(struct treenode *node, wc_ds_path_t *path, unsigned depth) {
	struct treenode *n;

	if (depth == wc_datasync_path_get_part_count(path)) {
		return node;
	} else if (node->type == TREENODE_TYPE_INTERNAL
			&& wc_datasync_path_get_part_count(path) - depth > 0
			&& (n = internal_get(node, wc_datasync_path_get_part(path, depth))) != NULL)
	{
		return data_cache_get_r(n, path, depth + 1);
	} else {
		return NULL;
	}
}

struct treenode *data_cache_get(data_cache_t *cache, char *path) {
	struct treenode *ret = NULL;
	wc_ds_path_t *parsed_path = wc_datasync_path_new(path);

	ret = data_cache_get_r(cache->root, parsed_path, 0);

	wc_datasync_path_destroy(parsed_path);

	return ret;
}

struct treenode *data_cache_get_parsed(data_cache_t *cache, wc_ds_path_t *path) {
	return data_cache_get_r(cache->root, path, 0);
}
