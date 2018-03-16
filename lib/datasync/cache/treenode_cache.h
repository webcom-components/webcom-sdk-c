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

#ifndef SRC_TREENODE_CACHE_H_
#define SRC_TREENODE_CACHE_H_

#include <json-c/json.h>

#include "treenode.h"
typedef struct data_cache data_cache_t;

data_cache_t *data_cache_new();
void data_cache_destroy(data_cache_t *);
void data_cache_update_put(data_cache_t *cache, char *path, json_object *data);
void data_cache_update_merge(data_cache_t *cache, char *path, json_object *data);

void data_cache_set(data_cache_t *cache, char *path, json_object *data);
void data_cache_mkpath(data_cache_t *cache, char *path);
void data_cache_set_leaf(data_cache_t *cache, char *path, enum treenode_type type, union treenode_value uval);

struct treenode *data_cache_get(data_cache_t *cache, char *path);


#endif /* SRC_TREENODE_CACHE_H_ */
