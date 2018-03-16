/*
 * ht
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

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <json-c/json.h>

#include "stfu.h"

#include "../lib/datasync/cache/treenode_cache.h"
#include "../lib/datasync/cache/treenode_sibs.h"

void dump_tree_r(struct treenode_sibs *_, char *key, struct treenode *n, void *param) {
	uintptr_t depth = (uintptr_t)param;
	switch(n->type) {
	case TREENODE_TYPE_LEAF_BOOL:
		STFU_INFO("%*s< %-10.10s > => [value:bool] %s", (int)(4 * depth), " ", key, n->uval.bool == TN_FALSE ? "false" : "true");
		break;
	case TREENODE_TYPE_LEAF_NULL:
		STFU_INFO("%*s< %-10.10s > => [value:null]", (int)(4 * depth), " ", key);
		break;
	case TREENODE_TYPE_LEAF_NUMBER:
		STFU_INFO("%*s< %-10.10s > => [value:num ] %.17g", (int)(4 * depth), " ", key, n->uval.number);
		break;
	case TREENODE_TYPE_LEAF_STRING:
		STFU_INFO("%*s< %-10.10s > => [value:str ] \"%s\"", (int)(4 * depth), " ", key, n->uval.str);
		break;
	case TREENODE_TYPE_INTERNAL:
		STFU_INFO("%*s< %-10.10s > => [internal  ] %u children:", (int)(4 * depth), " ", key, treenode_sibs_count(n->uval.children));
		treenode_sibs_foreach(n->uval.children, TREENODE_SIBS_INORDER, dump_tree_r, (void*)(depth+1));
		break;
	}
	if (key == NULL) {

	}
}

int main(void) {
	data_cache_t *mycache;
	struct treenode *n;
	union treenode_value uval;

	mycache = data_cache_new();

	STFU_TRUE("Instantiate a new empty cache",
			mycache != NULL);

	n = data_cache_get(mycache, "/");

	STFU_TRUE("The cache has a root",
				n != NULL);

	STFU_TRUE("The cache root is the 'Null' value",
				n->type == TREENODE_TYPE_LEAF_NULL);

	STFU_INFO("Creating the '/stairway/to/heaven/' path in the cache");

	data_cache_mkpath(mycache, "/stairway/to/heaven/");

	n = data_cache_get(mycache, "/stairway/");

	STFU_TRUE("The path '/stairway/' exists",
				n != NULL);

	n = data_cache_get(mycache, "/stairway/to/");

	STFU_TRUE("The path '/stairway/to/' exists",
				n != NULL);

	n = data_cache_get(mycache, "/stairway/to/heaven/");

	STFU_TRUE("The path '/stairway/to/heaven/' exists",
				n != NULL);

	STFU_INFO("Creating the '/stairway/to/Lannion/' path in the cache");

	data_cache_mkpath(mycache, "/stairway/to/Lannion/");

	n = data_cache_get(mycache, "/stairway/");

	STFU_TRUE("The path '/stairway/' still exists",
				n != NULL);

	n = data_cache_get(mycache, "/stairway/to/");

	STFU_TRUE("The path '/stairway/to/' still exists",
				n != NULL);

	n = data_cache_get(mycache, "/stairway/to/heaven/");

	STFU_TRUE("The path '/stairway/to/heaven/' still exists",
				n != NULL);

	n = data_cache_get(mycache, "/stairway/to/Lannion/");

	STFU_TRUE("The path '/stairway/to/Lannion/' exists",
				n != NULL);


	STFU_INFO("Creating the '/stairway/to/Lannion/country/' leaf in the cache with String value \"France\"");
	uval.str = "France";
	data_cache_set_leaf(mycache, "/stairway/to/Lannion/country/", TREENODE_TYPE_LEAF_STRING, uval);

	n = data_cache_get(mycache, "/stairway/to/Lannion/country/");
	STFU_TRUE("The path '/stairway/to/Lannion/country' leaf exists",
				n != NULL);
	STFU_TRUE("The path '/stairway/to/Lannion/country' leaf is a string",
				n->type == TREENODE_TYPE_LEAF_STRING);
	STFU_STR_EQ("The path '/stairway/to/Lannion/country' leaf's value is \"France\"",
					n->uval.str, "France");

	uval.bool = TN_TRUE ;
	data_cache_set_leaf(mycache, "/stairway/to/Lannion/sunny/", TREENODE_TYPE_LEAF_BOOL, uval);

	uval.number = 22300;
	data_cache_set_leaf(mycache, "/stairway/to/Lannion/zipcode/", TREENODE_TYPE_LEAF_NUMBER, uval);

	STFU_INFO("Now dumping the cache contents:");

	n = data_cache_get(mycache, "/");
	treenode_sibs_foreach(n->uval.children, TREENODE_SIBS_INORDER, dump_tree_r, 0);

	uval.str = "azertyuiop";
	data_cache_set_leaf(mycache, "/", TREENODE_TYPE_LEAF_STRING, uval);
	data_cache_destroy(mycache);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
