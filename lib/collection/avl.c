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

/*
 * This code was somehow inspired by musl libc's tsearch_avl.c.
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
#include <stddef.h>
#include <stdint.h>

#include "avl.h"

struct avl_node {
	struct avl_node *l;
	struct avl_node *r;
	int height;
	unsigned char data[];
};

struct avl {
	struct avl_node *root;
	unsigned count;
	avl_key_cmp_f key_cmp;
	avl_data_copy_f data_copy;
	avl_data_size_f data_size;
	avl_data_cleanup_f data_cleanup;
};

avl_t *avl_new(
		avl_key_cmp_f key_cmp,
		avl_data_copy_f data_copy,
		avl_data_size_f data_size,
		avl_data_cleanup_f data_cleanup)
{
	avl_t *ret;

	ret = malloc(sizeof(*ret));

	ret->root = NULL;
	ret->count = 0;

	ret->key_cmp = key_cmp;
	ret->data_copy = data_copy;
	ret->data_size = data_size;
	ret->data_cleanup = data_cleanup;

	return ret;
}

unsigned avl_count(avl_t *avl) {
	return avl->count;
}

static void avl_update_height(struct avl_node *n) {
	n->height = 0;
	if (n->l && n->l->height > n->height) {
		n->height = n->l->height;
	}
	if (n->r && n->r->height > n->height) {
		n->height = n->r->height;
	}
	n->height++;
}

static int avl_delta(struct avl_node *n) {
	return (n->l ? n->l->height : 0) - (n->r ? n->r->height : 0);
}

static struct avl_node *avl_rotl(struct avl_node *n) {
	struct avl_node *r = n->r;
	n->r = r->l;
	r->l = n;
	avl_update_height(n);
	avl_update_height(r);
	return r;
}

static struct avl_node *avl_rotr(struct avl_node *n) {
	struct avl_node *l = n->l;
	n->l = l->r;
	l->r = n;
	avl_update_height(n);
	avl_update_height(l);
	return l;
}

static struct avl_node *avl_balance(struct avl_node *n) {
	int d = avl_delta(n);

	if (d < -1) {
		if (avl_delta(n->r) > 0) {
			n->r = avl_rotr(n->r);
		}
		return avl_rotl(n);
	} else if (d > 1) {
		if (avl_delta(n->l) < 0) {
			n->l = avl_rotl(n->l);
		}
		return avl_rotr(n);
	}
	avl_update_height(n);
	return n;
}

static struct avl_node *avl_insert_rec(avl_key_cmp_f cmp, struct avl_node **root, struct avl_node *entry, int *ins) {
	int c;
	struct avl_node *node = *root;

	if (!*root) {
		*root = entry;
		entry->l = entry->r = NULL;
		entry->height = 1;
		*ins = 1;
		return entry;
	}

	c = cmp(entry->data, node->data);

	if (c < 0) {
		node = avl_insert_rec(cmp, &node->l, entry, ins);
	} else {
		node = avl_insert_rec(cmp, &node->r, entry, ins);
	}

	if (*ins) {
		*root = avl_balance(*root);
	}

	return node;
}

void *avl_get(avl_t *avl, void *key) {
	void *ret = NULL;
	struct avl_node *n;
	int cmp;

	n = avl->root;

	while (n) {
		cmp = avl->key_cmp(key, n->data);
		if (cmp < 0) {
			n = n->l;
		} else if (cmp > 0) {
			n = n->r;
		} else {
			ret = n->data;
			break;
		}
	}

	return ret;
}

void *avl_insert(avl_t *avl, void *data) {
	struct avl_node *ret = NULL;
	int ins = 0;

	if (avl_get(avl, data)) {
		return NULL;
	}

	ret = malloc(sizeof(*ret) + avl->data_size(data));

	avl->data_copy(data, &ret->data);

	avl_insert_rec(avl->key_cmp, &avl->root, ret, &ins);
	avl->count++;

	return &ret->data;
}

static struct avl_node *avl_movr(struct avl_node *n, struct avl_node *r) {
	if (!n)
		return r;
	n->r = avl_movr(n->r, r);
	return avl_balance(n);
}

static struct avl_node *avl_remove_rec(avl_t *avl, struct avl_node **root, void *key, struct avl_node *parent) {
	int key_cmp;

	if (!*root) {
		return NULL;
	}
	key_cmp = avl->key_cmp(key, (*root)->data);
	if (key_cmp == 0) {
		struct avl_node *r = *root;
		*root = avl_movr(r->l, r->r);
		avl->data_cleanup(r->data);
		free(r);
		avl->count--;
		return parent;
	}
	if (key_cmp < 0) {
		parent = avl_remove_rec(avl, &(*root)->l, key, *root);
	} else {
		parent = avl_remove_rec(avl, &(*root)->r, key, *root);
	}
	if (parent) {
		*root = avl_balance(*root);
	}
	return parent;
}

void avl_remove(avl_t *avl, void *key) {
	struct avl_node *n = avl->root;

	avl_remove_rec(avl, &n, key, n);
	avl->root = n;
}

static void avl_it_init(struct avl_it *it) {
	it->_rt = it->__rt = NULL;
	it->_sp = it->_s;
	it->_c = NULL;
}

void avl_it_start(struct avl_it *it, avl_t *avl) {
	struct avl_node *cur;

	avl_it_init(it);

	if (avl == NULL) {
		return;
	}

	cur = avl->root;

	while (cur != NULL) {
		*it->_sp++ = cur; /* push(cur) */
		cur = cur->l;
	}
}

void avl_it_start_at(struct avl_it *it, avl_t *avl, void *key) {
	struct avl_node *cur;
	int cmp;

	avl_it_init(it);

	cur = avl->root;

	while (cur != NULL) {
		it->_c = cur;
		it->_rt = it->__rt;
		cmp = avl->key_cmp(key, cur->data);
		if (cmp < 0) {
			*it->_sp++ = cur; /* push(cur) */
			cur = cur->l;
		} else if (cmp > 0) {
			it->__rt = cur;
			cur = cur->r;
		} else {
			*it->_sp++ = cur; /* push(cur) */
			break;
		}
	}
}

void *avl_it_next(struct avl_it *it) {
	struct avl_node *top, *cur;

	if (it->_sp == it->_s) { /* the stack is empty */
		return NULL;
	}

	top = *--it->_sp; /* top = pop() */
	it->_rt = it->__rt;
	if (top->r != NULL) {
		it->__rt = top;
		cur = top->r;
		while (cur != NULL) {
			*it->_sp++ = cur; /* push(cur) */
			cur = cur->l;
		}
	}
	it->_c = top;
	return top->data;
}

void *avl_it_peek(struct avl_it *it) {
	return it->_c ? it->_c->data : NULL;
}

int avl_it_has_next(struct avl_it *it) {
	return it->_sp > it->_s;
}

void *avl_it_peek_next(struct avl_it *it) {
	if (avl_it_has_next(it)) { /* the stack is empty */
		return (*(it->_sp - 1))->data;
	} else {
		return NULL;
	}
}

void *avl_it_peek_prev(struct avl_it *it) {
	if (it->_c == NULL) {
		return NULL;
	} else if (it->_c->l != NULL) {
		struct avl_node *cur = it->_c->l;
		while (cur->r != NULL) {
			cur = cur->r;
		}
		return cur->data;
	} else if (it->_rt != NULL) {
		return it->_rt->data;
	} else {
		return NULL;
	}
}

static void avl_walk_rec(avl_t *avl, enum avl_order order, struct avl_node *root, avl_walker_f walker, void *param) {
	if (root == NULL) return;

	if (order == AVL_PREORDER) walker(avl, root->data, param);
	avl_walk_rec(avl, order, root->l, walker, param);
	if (order == AVL_INORDER) walker(avl, root->data, param);
	avl_walk_rec(avl, order, root->r, walker, param);
	if (order == AVL_POSTORDER) walker(avl, root->data, param);
}

void avl_walk(avl_t *avl, enum avl_order order, avl_walker_f walker, void* param) {
	avl_walk_rec(avl, order, avl->root, walker, param);
}

static inline struct avl_node *avl_node_from_data(void *data) {
	return (struct avl_node *)(((char *)data) - offsetof(struct avl_node, data));
}

static void avl_destroyer_walker(avl_t *avl, void *data, void *param) {
	(void)param;
	avl->data_cleanup(data);
	free(avl_node_from_data(data));
}

void avl_destroy(avl_t *avl) {
	avl_walk(avl, AVL_POSTORDER, avl_destroyer_walker, NULL);
	free(avl);
}
