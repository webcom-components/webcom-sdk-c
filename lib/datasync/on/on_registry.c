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

#include "on_registry.h"
#include "../json.h"
#include "../../collection/ht.h"

struct on_registry {
	ht_t *map[ON_TYPE_NUM];
};

static int path_eq(wc_ds_path_t *a, wc_ds_path_t *b) {
	return wc_datasync_path_cmp(a, b) == 0;
}

struct on_registry *on_registry_new() {
	struct on_registry *ret = NULL;
	int i;

	ret = malloc(sizeof(*ret));

	for (i = 0 ; i < ON_TYPE_NUM ; i++) {
		ret->map[i] = ht_new(
				(ht_hash_f) wc_datasync_path_hash,
				(ht_key_eq_f) path_eq,
				(ht_key_free_f) NULL,
				(ht_val_free_f) on_sub_destroy_list);
	}

	return ret;
}

void on_registry_attach(struct on_registry* reg, struct on_sub *sub) {
	sub->next = ht_get(reg->map[sub->type], sub->path);
	ht_insert(reg->map[sub->type], sub->path, sub);
}

void on_registry_dispatch(struct on_registry* reg, data_cache_t *cache, wc_ds_path_t * path, struct treenode *old_data, struct treenode *new_data) {
	struct on_sub *sub;
	(void) cache;

	if (ht_get_ex(reg->map[ON_VALUE], path, (void**)&sub)) {
		do {
			if (treenode_hash_get(old_data) != treenode_hash_get(new_data)) {
				char *json_str = malloc(treenode_to_json_len(new_data) + 1);
				treenode_to_json(new_data, json_str);
				sub->cb.on_value_cb(json_str);
				free(json_str);
			}
			sub = sub->next;
		} while (sub);
	}

	if (ht_get_ex(reg->map[ON_CHILD_ADDED], path, (void**)&sub)) {
		do {
			if (treenode_hash_get(old_data) != treenode_hash_get(new_data)) {
				char *json_str = malloc(treenode_to_json_len(new_data) + 1);
				treenode_to_json(new_data, json_str);
				sub->cb.on_value_cb(json_str);
				free(json_str);
			}
			sub = sub->next;
		} while (sub);
	}
}

void on_registry_detach(struct on_registry* reg, enum on_sub_type type, char *path) {
	wc_ds_path_t *parsed_path = wc_datasync_path_new(path);

	ht_remove(reg->map[type], parsed_path);

	wc_datasync_path_destroy(parsed_path);
}

void on_registry_destroy(struct on_registry *reg) {
	int i;

	for (i = 0 ; i < ON_TYPE_NUM ; i++) {
		 ht_destroy(reg->map[i]);
	}
}
