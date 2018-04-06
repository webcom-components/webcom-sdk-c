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

#ifndef SRC_TREENODE_H_
#define SRC_TREENODE_H_


typedef enum treenode_bool {TN_FALSE = 0, TN_TRUE = 1} treenode_bool_t;

struct treenode_sibs;

union treenode_value {
	double number;
	struct treenode_sibs *children;
	enum treenode_bool bool;
	char *str;
	void *null;
};

enum treenode_type {
	TREENODE_TYPE_LEAF_NULL = 0,
	TREENODE_TYPE_LEAF_BOOL,
	TREENODE_TYPE_LEAF_NUMBER,
	TREENODE_TYPE_LEAF_STRING,
	TREENODE_TYPE_INTERNAL,
};

typedef struct {char bytes[28];} treenode_hash_t;

struct treenode {
	enum treenode_type type:3;
	int hash_cached:1;
	union treenode_value uval;
	char hash[];
};

struct treenode_ex {
	struct treenode n;
	treenode_hash_t h;
};

#define TREENODE_STATIC(_name, _type, _val) \
		struct {struct treenode n; char h[sizeof(treenode_hash_t)];} (_name) = \
			{.n = {.type = (_type), .uval = (union treenode_value) (_val)}}

struct treenode *treenode_new(enum treenode_type type, union treenode_value uval);
void treenode_cleanup(struct treenode *node);
void treenode_destroy(struct treenode *node);
treenode_hash_t *treenode_hash_get(struct treenode *n);
int treenode_to_json_len(struct treenode *n);
int treenode_to_json(struct treenode *n, char *json);


#endif /* SRC_TREENODE_H_ */
