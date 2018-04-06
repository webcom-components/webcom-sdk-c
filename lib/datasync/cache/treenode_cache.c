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


#include "../path.h"

#include "../on/on_registry.h"

#include "treenode_cache.h"
#include "treenode.h"
#include "treenode_sibs.h"

struct data_cache {
	struct treenode *root;
	struct on_registry *registry;
};

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
	on_registry_destroy(cache->registry);
	free(cache);
}

void data_cache_set_leaf(data_cache_t *cache, char *path, enum treenode_type type, union treenode_value uval) {
	assert(type != TREENODE_TYPE_INTERNAL);

	struct treenode_ex new_node = {.n = {.type = type, .uval = uval}};

	wc_ds_path_t *parsed_path = wc_datasync_path_new(path);
	unsigned nparts = wc_datasync_path_get_part_count(parsed_path);

	if (nparts == 0) {
		/* chop the entire tree */
		if (cache->root->type == TREENODE_TYPE_INTERNAL) {
			on_registry_dispatch(cache->registry, cache, &path_root, data_cache_get(cache, path), &new_node.n);
			data_cache_empty(cache);
		}

		/* replace the root */
		cache->root = treenode_new(type, uval);
	} else {
		struct treenode *n;

		parsed_path->nparts--; /* push(hack) */
		data_cache_mkpath_w(cache, parsed_path);
		n = data_cache_get_r(cache->root, parsed_path, 0);
		parsed_path->nparts++; /* pop() */

		/* XXX the old leaf must be removed */
		treenode_sibs_remove(n->uval.children, wc_datasync_path_get_part(parsed_path, nparts - 1));

		treenode_sibs_add_ex(n->uval.children, wc_datasync_path_get_part(parsed_path, nparts - 1), type, uval);
	}
	wc_datasync_path_destroy(parsed_path);
}

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
			&& (n = treenode_sibs_get(node->uval.children, wc_datasync_path_get_part(path, depth))) != NULL)
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

struct dump_helper {
	FILE *stream;
	int depth;
};

void dump_tree_r(struct treenode_sibs *_, char *key, struct treenode *n, void *param) {
	struct dump_helper *hlp = (struct dump_helper *)param, up_hlp;
	(void)_;
	switch(n->type) {
	case TREENODE_TYPE_LEAF_BOOL:
		fprintf(hlp->stream, "%*s< %-10.10s > => [value:bool] %s\n", 4 * hlp->depth, " ", key, n->uval.bool == TN_FALSE ? "false" : "true");
		break;
	case TREENODE_TYPE_LEAF_NULL:
		fprintf(hlp->stream, "%*s< %-10.10s > => [value:null]\n", 4 * hlp->depth, " ", key);
		break;
	case TREENODE_TYPE_LEAF_NUMBER:
		fprintf(hlp->stream, "%*s< %-10.10s > => [value:num ] %.17g\n", 4 * hlp->depth, " ", key, n->uval.number);
		break;
	case TREENODE_TYPE_LEAF_STRING:
		fprintf(hlp->stream, "%*s< %-10.10s > => [value:str ] \"%s\"\n", 4 * hlp->depth, " ", key, n->uval.str);
		break;
	case TREENODE_TYPE_INTERNAL:
		fprintf(hlp->stream, "%*s< %-10.10s > => [internal  ] %u children:\n", 4 * hlp->depth, " ", key, treenode_sibs_count(n->uval.children));
		up_hlp.depth = hlp->depth + 1;
		up_hlp.stream = hlp->stream;
		treenode_sibs_foreach(n->uval.children, TREENODE_SIBS_INORDER, dump_tree_r, &up_hlp);
		break;
	}
	if (key == NULL) {

	}
}

void data_cache_debug_print(data_cache_t *cache, FILE *stream) {
	struct dump_helper hlp;

	if (cache->root->type == TREENODE_TYPE_INTERNAL) {
		hlp.depth = 0;
		hlp.stream = stream;
		treenode_sibs_foreach(cache->root->uval.children, TREENODE_SIBS_INORDER, dump_tree_r, &hlp);
	} else {
		switch(cache->root->type) {
		case TREENODE_TYPE_LEAF_BOOL:
			fprintf(stream, "ROOT => [value:bool] %s\n", cache->root->uval.bool == TN_FALSE ? "false" : "true");
			break;
		case TREENODE_TYPE_LEAF_NULL:
			fprintf(stream, "ROOT => [value:null]\n");
			break;
		case TREENODE_TYPE_LEAF_NUMBER:
			fprintf(stream, "ROOT => [value:num ] %.17g\n", cache->root->uval.number);
			break;
		case TREENODE_TYPE_LEAF_STRING:
			fprintf(stream, "ROOT => [value:str ] \"%s\"\n", cache->root->uval.str);
			break;
		default:;
		}
	}
}
