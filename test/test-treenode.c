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
#include "../lib/datasync/path.h"

int main(void) {

	struct treenode *root = treenode_new_internal();
	struct treenode *tn_null = internal_add_new_null(root, "foo");
	struct treenode *tn_bool_t = internal_add_new_bool(root, "bar_true", 1);
	struct treenode *tn_bool_f = internal_add_new_bool(root, "bar_false", 0);
	struct treenode *tn_num1 = internal_add_new_number(root, "bar_num1", 2.4);
	struct treenode *tn_num2 = internal_add_new_number(root, "bar_num2", 2);
	struct treenode *tn_str = internal_add_new_string(root, "bar_str", "toto");

	/* {"a": "va", "b": "vb"} */
	struct treenode *test_tree = treenode_new_internal();
	internal_add_new_string(test_tree, "b", "vb");
	internal_add_new_string(test_tree, "a", "va");



	STFU_TRUE("Null node hash returns a NULL pointer",
			treenode_hash_get(tn_null) == NULL);

	STFU_TRUE("Boolean node (true) hash is correct",
			treenode_hash_eq(treenode_hash_get(tn_bool_t), (treenode_hash_t*)"E5z61QM0lN/U2WsOnusszCTkR8M="));

	STFU_TRUE("Boolean node (false) hash is correct",
			treenode_hash_eq(treenode_hash_get(tn_bool_f), (treenode_hash_t*)"aSSNoqcS4oQwJ2xxH20rvpp3zP0="));

	STFU_TRUE("String node (\"toto\") hash is correct",
			treenode_hash_eq(treenode_hash_get(tn_str), (treenode_hash_t*)"g1BTmpk55UYf7132J9jNSCmAhlM="));

	STFU_TRUE("Numeric node (2.4) hash is correct",
			treenode_hash_eq(treenode_hash_get(tn_num1), (treenode_hash_t*)"FPgWPSptcCOG5upXrkMHvEVR8Do="));

	STFU_TRUE("Numeric node (2) hash is correct",
			treenode_hash_eq(treenode_hash_get(tn_num2), (treenode_hash_t*)"WtSt2Xo3L0JtPuArzQHofPrZOuU="));

	STFU_TRUE("Object ({\"a\": \"va\", \"b\": \"vb\"}) hash is correct",
			treenode_hash_eq(treenode_hash_get(test_tree), (treenode_hash_t*)"fV62MCgJ1PdEaKAYIsg0as3xba0="));

	treenode_destroy(test_tree);
	treenode_destroy(root);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
