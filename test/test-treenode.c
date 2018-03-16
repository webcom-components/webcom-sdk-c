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
#include <assert.h>
#include <stdint.h>

#include "stfu.h"

#include "../lib/datasync/cache/treenode.h"
#include "../lib/datasync/cache/treenode_sibs.h"
#include "../lib/datasync/path.h"

int hash_eq(treenode_hash_t *a, char *b) {
	return strncmp((char *)a, b, 28) == 0;
}

void check_node(struct treenode_sibs *_, char *key, struct treenode *n, void *param) {
	char *prev_nested_key = NULL;

	if (*(char **)param != NULL) {
		STFU_TRUE("the current key is greater than its previous sibling", wc_datasync_key_cmp(key, *(char **)param) > 0);
		printf("\t prev: '%s', current: '%s'\n", *(char **)param, key);
	}

	switch (n->type) {
	case TREENODE_TYPE_INTERNAL:
		treenode_sibs_foreach(n->uval.children, TREENODE_SIBS_INORDER, check_node, &prev_nested_key);
		break;
	default:
		break;
	}

	*(char **)param = key;
}

int main(void) {
	char *prev_key = NULL;

	struct treenode_sibs *tns = treenode_sibs_new();
	struct treenode *tn_null = treenode_sibs_add_null(tns, "foo");
	struct treenode *tn_bool_t = treenode_sibs_add_bool(tns, "bar_true", 1);
	struct treenode *tn_bool_f = treenode_sibs_add_bool(tns, "bar_false", 0);
	struct treenode *tn_num1 = treenode_sibs_add_number(tns, "bar_num1", 2.4);
	struct treenode *tn_num2 = treenode_sibs_add_number(tns, "bar_num2", 2);
	struct treenode *tn_str = treenode_sibs_add_string(tns, "bar_str", "toto");

	/* {"a": "va", "b": "vb"} */
	struct treenode_sibs *test_tree = treenode_sibs_new();
	treenode_sibs_add_string(test_tree, "b", "vb");
	treenode_sibs_add_string(test_tree, "a", "va");

	struct treenode *tn_obj = treenode_sibs_add_internal(tns, "object", test_tree);


	STFU_TRUE("Null node hash returns a NULL pointer",
			treenode_hash_get(tn_null) == NULL);

	STFU_TRUE("Boolean node (true) hash is correct",
			hash_eq(treenode_hash_get(tn_bool_t), "E5z61QM0lN/U2WsOnusszCTkR8M="));

	STFU_TRUE("Boolean node (false) hash is correct",
			hash_eq(treenode_hash_get(tn_bool_f), "aSSNoqcS4oQwJ2xxH20rvpp3zP0="));

	STFU_TRUE("String node (\"toto\") hash is correct",
			hash_eq(treenode_hash_get(tn_str), "g1BTmpk55UYf7132J9jNSCmAhlM="));

	STFU_TRUE("Numeric node (2.4) hash is correct",
			hash_eq(treenode_hash_get(tn_num1), "FPgWPSptcCOG5upXrkMHvEVR8Do="));

	STFU_TRUE("Numeric node (2) hash is correct",
			hash_eq(treenode_hash_get(tn_num2), "WtSt2Xo3L0JtPuArzQHofPrZOuU="));

	treenode_sibs_foreach(tns, TREENODE_SIBS_INORDER, check_node, &prev_key);

	STFU_TRUE("Object ({\"a\": \"va\", \"b\": \"vb\"}) hash is correct",
			hash_eq(treenode_hash_get(tn_obj), "fV62MCgJ1PdEaKAYIsg0as3xba0="));

	treenode_sibs_destroy(test_tree);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
