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

/*
 * This code was loosely adapted from musl libc's tsearch_avl.c.
 *
 * https://www.musl-libc.org/
 *
 * musl as a whole is licensed under the standard MIT license.
 *
 * Copyright © 2005-2014 Rich Felker, et al.
 *
 * The XSI search API (src/search/ *.c) functions are Copyright © 2011 Szabolcs
 * Nagy and licensed under following terms: "Permission to use, copy, modify,
 * and/or distribute this code for any purpose with or without fee is hereby
 * granted. There is no warranty."
 */

#include <stdlib.h>
#include <string.h>

#include "../path.h"

#include "treenode_sibs.h"

struct treenode_sibs_entry {
	struct treenode_sibs_entry *l;
	struct treenode_sibs_entry *r;
	int height;
	char *key;
	struct treenode node;
};

struct treenode_sibs {
	struct treenode_sibs_entry *root;
	unsigned count;
};

struct treenode_sibs *treenode_sibs_new() {
	struct treenode_sibs * ret;
	ret = calloc(1, sizeof (*ret));
	return ret;
}

unsigned treenode_sibs_count(struct treenode_sibs *l) {
	return l->count;
}

static void updateheight(struct treenode_sibs_entry *n) {
	n->height = 0;
	if (n->l && n->l->height > n->height) {
		n->height = n->l->height;
	}
	if (n->r && n->r->height > n->height) {
		n->height = n->r->height;
	}
	n->height++;
}

static int delta(struct treenode_sibs_entry *n) {
	return (n->l ? n->l->height : 0) - (n->r ? n->r->height : 0);
}

static struct treenode_sibs_entry *rotl(struct treenode_sibs_entry *n) {
	struct treenode_sibs_entry *r = n->r;
	n->r = r->l;
	r->l = n;
	updateheight(n);
	updateheight(r);
	return r;
}

static struct treenode_sibs_entry *rotr(struct treenode_sibs_entry *n) {
	struct treenode_sibs_entry *l = n->l;
	n->l = l->r;
	l->r = n;
	updateheight(n);
	updateheight(l);
	return l;
}

static struct treenode_sibs_entry *balance(struct treenode_sibs_entry *n) {
	int d = delta(n);

	if (d < -1) {
		if (delta(n->r) > 0) {
			n->r = rotr(n->r);
		}
		return rotl(n);
	} else if (d > 1) {
		if (delta(n->l) < 0) {
			n->l = rotl(n->l);
		}
		return rotr(n);
	}
	updateheight(n);
	return n;
}

static struct treenode_sibs_entry *insert(struct treenode_sibs_entry **root, struct treenode_sibs_entry *entry, int *ins) {
	int c;
	struct treenode_sibs_entry *node = *root;

	if (!*root) {
		*root = entry;
		entry->l = entry->r = NULL;
		entry->height = 1;
		*ins = 1;
		return entry;
	}

	c = wc_datasync_key_cmp(entry->key, node->key);

	if (c < 0) {
		node = insert(&node->l, entry, ins);
	} else {
		node = insert(&node->r, entry, ins);
	}

	if (*ins) {
		*root = balance(*root);
	}

	return node;
}

struct treenode *treenode_sibs_get(struct treenode_sibs *l, const char *key) {
	struct treenode *ret = NULL;
	struct treenode_sibs_entry *n;
	int cmp;

	n = l->root;

	while (n) {
		cmp = wc_datasync_key_cmp(key, n->key);
		if (cmp < 0) {
			n = n->l;
		} else if (cmp > 0) {
			n = n->r;
		} else {
			ret = &n->node;
			break;
		}
	}

	return ret;
}

struct treenode *treenode_sibs_add_ex(struct treenode_sibs *l, char *key, enum treenode_type type, union treenode_value uval) {
	struct treenode_sibs_entry *ret = NULL;
	int ins = 0;

	if (treenode_sibs_get(l, key)) {
		goto end;
	}

	ret = calloc(1,
			(type == TREENODE_TYPE_LEAF_BOOL || type == TREENODE_TYPE_LEAF_NULL)
			? sizeof(*ret)
			: sizeof(*ret) + sizeof (treenode_hash_t));

	ret->key = strdup(key);
	ret->node.type = type;
	ret->node.uval = uval;

	insert(&l->root, ret, &ins);
	l->count++;

end:
	return &ret->node;
}

static struct treenode_sibs_entry *movr(struct treenode_sibs_entry *n, struct treenode_sibs_entry *r) {
	if (!n)
		return r;
	n->r = movr(n->r, r);
	return balance(n);
}

static struct treenode_sibs_entry *remove(struct treenode_sibs_entry **root, char *key, struct treenode_sibs_entry *parent) {
	int cmp;

	if (!*root)
		return 0;
	cmp = wc_datasync_key_cmp(key, (*root)->key);
	if (cmp == 0) {
		struct treenode_sibs_entry *r = *root;
		*root = movr(r->l, r->r);
		treenode_cleanup(&r->node);
		free(r->key);
		free(r);
		return parent;
	}
	if (cmp < 0)
		parent = remove(&(*root)->l, key, *root);
	else
		parent = remove(&(*root)->r, key, *root);
	if (parent)
		*root = balance(*root);
	return parent;
}

void treenode_sibs_remove(struct treenode_sibs *l, char *key) {
	struct treenode_sibs_entry *n = l->root;
	if (treenode_sibs_get(l, key) == NULL) {
		goto end;
	}
	remove(&n, key, n);
	l->root = n;
	l->count--;
end:
	;
}


static void walk_r(struct treenode_sibs *l, enum treenode_sibs_order order, struct treenode_sibs_entry *root, void(*f)(struct treenode_sibs *l, char*, struct treenode *, void*), void* param) {
	if (root == NULL) return;

	if (order == TREENODE_SIBS_PREORDER) f(l, root->key, &root->node, param);
	walk_r(l, order, root->l, f, param);
	if (order == TREENODE_SIBS_INORDER) f(l, root->key, &root->node, param);
	walk_r(l, order, root->r, f, param);
	if (order == TREENODE_SIBS_POSTORDER) f(l, root->key, &root->node, param);
}

void treenode_sibs_foreach(struct treenode_sibs *l, enum treenode_sibs_order order, void(*f)(struct treenode_sibs *l, char*, struct treenode *, void*), void* param) {
	walk_r(l, order, l->root, f, param);
}

void treenode_sibs_destroy(struct treenode_sibs *l) {
	free(l);
}
