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

#ifndef SRC_TREENODE_SIBS_H_
#define SRC_TREENODE_SIBS_H_

#include "treenode.h"

enum treenode_sibs_order {TREENODE_SIBS_PREORDER, TREENODE_SIBS_INORDER, TREENODE_SIBS_POSTORDER};

struct treenode_sibs;
struct treenode_sibs *treenode_sibs_new();
unsigned treenode_sibs_count(struct treenode_sibs *l);
void treenode_sibs_destroy(struct treenode_sibs *l);

struct treenode *treenode_sibs_get(struct treenode_sibs *l, const char *key);
struct treenode *treenode_sibs_add_ex(struct treenode_sibs *l, char *key, enum treenode_type type, union treenode_value uval);
void treenode_sibs_remove(struct treenode_sibs *l, char *key);


void treenode_sibs_foreach(struct treenode_sibs *l, enum treenode_sibs_order order, void(*f)(struct treenode_sibs *, char *, struct treenode *, void*), void* param);

#define treenode_sibs_add_null(_l, _key) \
	treenode_sibs_add_ex(_l, _key, TREENODE_TYPE_LEAF_NULL, (union treenode_value) NULL);

#define treenode_sibs_add_bool(_l, _key, _val) \
	treenode_sibs_add_ex(_l, _key, TREENODE_TYPE_LEAF_BOOL, (union treenode_value) ((enum treenode_bool)(_val)));

#define treenode_sibs_add_number(_l, _key, _val) \
	treenode_sibs_add_ex(_l, _key, TREENODE_TYPE_LEAF_NUMBER, (union treenode_value) ((double)(_val)));

#define treenode_sibs_add_string(_l, _key, _val) \
	treenode_sibs_add_ex(_l, _key, TREENODE_TYPE_LEAF_STRING, (union treenode_value) ((char*)(_val)));

#define treenode_sibs_add_internal(_l, _key, _val) \
	treenode_sibs_add_ex(_l, (char *)_key, TREENODE_TYPE_INTERNAL, (union treenode_value) ((struct treenode_sibs *)(_val)));


#endif /* SRC_TREENODE_SIBS_H_ */
