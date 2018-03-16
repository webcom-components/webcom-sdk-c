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

#include "treenode_cache.h"
#include "treenode.h"
#include "treenode_sibs.h"
#include "../path.h"

struct data_cache {
	struct treenode *root;
};

static void data_cache_mkpath_w(data_cache_t *cache, wc_ds_path_t *path);
static struct treenode *data_cache_get_r(struct treenode *node, wc_ds_path_t *path, unsigned depth);

data_cache_t *data_cache_new() {
	data_cache_t *ret = calloc(1, sizeof (*ret));

	struct treenode *root = calloc(1, sizeof (*root));

	root->type = TREENODE_TYPE_LEAF_NULL;
	ret->root = root;

	return ret;
}
void data_cache_update_put(data_cache_t *cache, char *path, json_object *data);
void data_cache_update_merge(data_cache_t *cache, char *path, json_object *data);

static void data_cache_destroy_nodes_r(struct treenode_sibs *l, char *key, struct treenode *node, void *_) {
	if (node->type == TREENODE_TYPE_INTERNAL) {
		treenode_sibs_foreach(node->uval.children, TREENODE_SIBS_POSTORDER, data_cache_destroy_nodes_r, _);
	}
	treenode_sibs_remove(l, key);
}

/* warning: this function leaves the cache in a transient state where its root
 * is the NULL pointer, it must be linked to an actual treenode right after */
static void data_cache_empty(data_cache_t *cache) {
	if (cache->root->type == TREENODE_TYPE_INTERNAL) {
		treenode_sibs_foreach(cache->root->uval.children, TREENODE_SIBS_POSTORDER, data_cache_destroy_nodes_r, NULL);
	}
	treenode_destroy(cache->root);
	cache->root = NULL;
}

void data_cache_destroy(data_cache_t *cache) {
	data_cache_empty(cache);
	free(cache);
}
/*
static void data_cache_remove_child(struct treenode *parent, char *key) {
	assert(parent->type == TREENODE_TYPE_INTERNAL);
	assert(treenode_sibs_get(parent->uval.children, key));

	treenode_sibs_remove(parent->uval.children, key);
}
*/

void data_cache_set_leaf(data_cache_t *cache, char *path, enum treenode_type type, union treenode_value uval) {
	assert(type != TREENODE_TYPE_INTERNAL);
	wc_ds_path_t *parsed_path = wc_ds_path_new(path);
	unsigned nparts = wc_datasync_path_get_part_count(parsed_path);

	if (nparts == 0) {
		/* chop the entire tree */
		if (cache->root->type == TREENODE_TYPE_INTERNAL) {
			data_cache_empty(cache);
		}

		/* replace the root */
		cache->root = treenode_new(type, uval);
#if 0
	} else if (nparts == 1) {
		/* insert in root object */
		if (cache->root->type != TREENODE_TYPE_INTERNAL) {
			/* it if the root is not an internal node, replace it */
			union treenode_value root_obj;

			root_obj.children = treenode_sibs_new();
			treenode_destroy(cache->root);
			cache->root = treenode_new(TREENODE_TYPE_INTERNAL, root_obj);
		}
		treenode_sibs_add_ex(cache->root->uval.children, wc_datasync_path_get_part(parsed_path, 0), type, uval);
#endif
	} else {
		struct treenode *n;

		parsed_path->nparts--; /* push(hack) */
		data_cache_mkpath_w(cache, parsed_path);
		n = data_cache_get_r(cache->root, parsed_path, 0);
		parsed_path->nparts++; /* pop() */

		treenode_sibs_add_ex(n->uval.children, wc_datasync_path_get_part(parsed_path, nparts - 1), type, uval);
	}
	wc_datasync_path_destroy(parsed_path);
}

/*
static void data_cache_set_r(struct treenode *parent, wc_ds_path_t *path, unsigned depth, json_object *data) {
	struct treenode *n, *p;
	if (wc_datasync_path_get_part_count(path) - depth > 1) {
		if ((n = treenode_sibs_get(parent->uval.children,
				wc_datasync_path_get_part(path, depth)))) {
			if (n->type == TREENODE_TYPE_INTERNAL) {
				data_cache_set_r(n, path, depth + 1, data);
			} else {
				data_cache_remove_child(parent, n->key);
				data_cache_set_r(parent, path, depth, data);
			}
		} else {
			p = calloc(1, sizeof(*p) + sizeof(treenode_hash_t));
			p->key = strdup(wc_datasync_path_get_part(path, depth));
			p->type = TREENODE_TYPE_INTERNAL;
			p->uval.children = treenode_sibs_new();

			treenode_sibs_add_internal(parent->uval.children, wc_datasync_path_get_part(path, depth), p);

			data_cache_set_r(p, path, depth + 1, data);
		}
	} else {
		assert(parent->type == TREENODE_TYPE_INTERNAL);

		treenode_sibs_add_ex(parent->uval.children, wc_datasync_path_get_part(path, depth), type, uval);

	}
}
*/
static void data_cache_mkpath_w(data_cache_t *cache, wc_ds_path_t *path) {
	struct treenode *cur;
	struct treenode_sibs *prev;
	union treenode_value uval;
	unsigned i;

	if (cache->root->type != TREENODE_TYPE_INTERNAL) {
		treenode_destroy(cache->root);
		cache->root = treenode_new(TREENODE_TYPE_INTERNAL, (union treenode_value)(struct treenode_sibs *)treenode_sibs_new());
	}

	prev = cache->root->uval.children;

	for (i = 0 ; i < wc_datasync_path_get_part_count(path) ; i++) {
		cur = treenode_sibs_get(prev, wc_datasync_path_get_part(path, i));

		if (cur == NULL) {
			uval.children = treenode_sibs_new();
			cur = treenode_sibs_add_ex(prev, wc_datasync_path_get_part(path, i), TREENODE_TYPE_INTERNAL, uval);
		} else if (cur->type != TREENODE_TYPE_INTERNAL) {
			treenode_sibs_remove(prev, wc_datasync_path_get_part(path, i));
			uval.children = treenode_sibs_new();
			cur = treenode_sibs_add_ex(prev, wc_datasync_path_get_part(path, i), TREENODE_TYPE_INTERNAL, uval);
		}
		prev = cur->uval.children;
	}
}

void data_cache_mkpath(data_cache_t *cache, char *path) {
	wc_ds_path_t *parsed_path = wc_ds_path_new(path);
	data_cache_mkpath_w(cache, parsed_path);
	wc_datasync_path_destroy(parsed_path);
}
/*
void data_cache_set(data_cache_t *cache, char *path, json_object *data) {
	wc_ds_path_t *parsed_path = wc_ds_path_new(path);

	if (wc_datasync_path_get_part_count(parsed_path) == 0) {
		//replace root
	} else {
		data_cache_set_r(cache->root, parsed_path, 0, data);
	}

	wc_datasync_path_destroy(parsed_path);
}*/

static struct treenode *data_cache_get_r(struct treenode *node, wc_ds_path_t *path, unsigned depth) {
	struct treenode *n;

	if (depth == wc_datasync_path_get_part_count(path)) {
		return node;
	} else if (node->type == TREENODE_TYPE_INTERNAL
			&& wc_datasync_path_get_part_count(path) - depth > 0
			&& (n = treenode_sibs_get(node->uval.children, wc_datasync_path_get_part(path, depth))) != NULL)
	{
		return data_cache_get_r(n, path, depth + 1);
	} else {
		return NULL;
	}
}

struct treenode *data_cache_get(data_cache_t *cache, char *path) {
	struct treenode *ret = NULL;
	wc_ds_path_t *parsed_path = wc_ds_path_new(path);

	ret = data_cache_get_r(cache->root, parsed_path, 0);

	wc_datasync_path_destroy(parsed_path);

	return ret;
}
