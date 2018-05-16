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

	ftreenode_to_json(mycache->root, stdout);
	putchar('\n');

	STFU_INFO("Setting '/' to the String value 'azertyuiop'");
	uval.str = "azertyuiop";
	data_cache_set_leaf(mycache, "/", TREENODE_TYPE_LEAF_STRING, uval);

	n = data_cache_get(mycache, "/");
	STFU_TRUE("The '/' leaf exists",
			n != NULL);
	STFU_TRUE("The '/' leaf is a string",
			n->type == TREENODE_TYPE_LEAF_STRING);
	STFU_STR_EQ("The '/' leaf value is 'azertyuiop'",
			n->uval.str, "azertyuiop");

	STFU_INFO("Now dumping the cache contents again:");
	ftreenode_to_json(mycache->root, stdout);
	putchar('\n');

	char *complex_doc =
			"{\"j\":[\"banana\",\"orange\",\"apple\"],\"b\":1.12e-45,\"f\":false"
			",\"c\":-0,\"a\":123456789,\"d\":-42,\"e\":true,\"i\":{\"bbbbb\":tr"
			"ue,\"zzzz\":{\"z\":{},\"x\":1,\"y\":2,\"foo\":null},\"aaaaa\":0,\""
			"001\":null,\"00002\":42,\"000003\":3.14159},\"g\":\"Flygande bäcka"
			"siner söka strax hwila på mjuka tuvor. Любя, съешь щипцы, — вздохн"
			"ёт мэр, — кайф жгуч. Nechť již hříšné saxofony ďáblů rozzvučí síň "
			"úděsnými tóny waltzu, tanga a quickstepu. Ξεσκεπάζω την ψυχοφθόρα "
			"βδελυγμία. Įlinkusi fechtuotojo špaga blykčiodama gręžė apvalų arb"
			"ūzą. Nne, nna, wepụ he'l'ụjọ dum n'ime ọzụzụ ụmụ, vufesi obi nye Ch"
			"ukwu, ṅụrịanụ, gbakọọnụ kpaa, kwee ya ka o guzoshie ike. Ìwò̩fà ń yò̩"
			"séji tó gbojúmó̩, ó hàn pákànpò̩ gan-an nis̩é̩ rè̩ bó dò̩la.\",\"h\":\"foo"
			"\"}";

	STFU_INFO("Setting the cache content via a the following JSON document:\n\t%s", complex_doc);
	data_cache_set(mycache, "/",
			complex_doc);
	STFU_INFO("Now dumping the cache contents again:");
	ftreenode_to_json(mycache->root, stdout);

	data_cache_destroy(mycache);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
