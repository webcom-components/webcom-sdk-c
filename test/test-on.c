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

#include "stfu.h"

#include "../lib/datasync/cache/treenode_cache.h"
#include "../lib/datasync/on/on_registry.h"

int on_value1(char * data) {
	STFU_INFO("on_value1: %s", data);

	return 0;
}

int on_value2(char * data) {
	STFU_INFO("on_value2: %s", data);

	return 0;
}

int on_value3(char * data) {
	STFU_INFO("on_value3: %s", data);

	return 0;
}

int on_value4(char * data) {
	STFU_INFO("on_value4: %s", data);

	return 0;
}

int main(void) {
	data_cache_t *cache;
	struct on_registry *reg;

	cache = data_cache_new();

	reg = cache->registry;

	data_cache_set_leaf(cache, "/foo/bar", TREENODE_TYPE_LEAF_NUMBER, (union treenode_value) 42.);
	data_cache_set_leaf(cache, "/foo/baz", TREENODE_TYPE_LEAF_NUMBER, (union treenode_value) 3.14);
	data_cache_set_leaf(cache, "/foo/qux/aaa", TREENODE_TYPE_LEAF_STRING, (union treenode_value) "AAA");
	data_cache_set_leaf(cache, "/foo/qux/bbb", TREENODE_TYPE_LEAF_STRING, (union treenode_value) "BBB");
	data_cache_set_leaf(cache, "/foo/qux/ccc/aaa", TREENODE_TYPE_LEAF_STRING, (union treenode_value) "AAA");
	data_cache_set_leaf(cache, "/foo/qux/ccc/azerty", TREENODE_TYPE_LEAF_NUMBER, (union treenode_value) 0.);


	on_registry_add_on_value(reg, cache, "/foo/qux", on_value1);
	on_registry_add_on_value(reg, cache, "/", on_value2);
	on_registry_add_on_value(reg, cache, "/foo/qux/ccc", on_value3);
	on_registry_add_on_value(reg, cache, "/foo/qux/ddd", on_value4);

	data_cache_set_leaf(cache, "/foo/qux/ddd", TREENODE_TYPE_LEAF_STRING, (union treenode_value) "DDD");

	on_registry_dispatch_on_value(reg, cache, "/foo/qux");
	on_registry_dispatch_on_value(reg, cache, "/foo/qux");

	data_cache_set_leaf(cache, "/foo/qux/ddd", TREENODE_TYPE_LEAF_STRING, (union treenode_value) "FFF");

	on_registry_dispatch_on_value(reg, cache, "/foo/qux");

	data_cache_set_leaf(cache, "/foo/qux/ccc", TREENODE_TYPE_LEAF_STRING, (union treenode_value) "CCC");

	on_registry_dispatch_on_value(reg, cache, "/foo/qux");

	STFU_TRUE("",
			0);

	STFU_SUMMARY();

	data_cache_destroy(cache);

	return STFU_NUMBER_FAILED;
}

