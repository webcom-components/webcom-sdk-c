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

#ifndef LIB_COLLECTION_AVL_H_
#define LIB_COLLECTION_AVL_H_

typedef struct avl avl_t;

typedef int (*avl_key_cmp_f)(void *a, void *b);
typedef size_t (*avl_data_size_f)(void *data);
typedef void (*avl_data_copy_f)(void *from, void *to);
typedef void (*avl_data_cleanup_f)(void *data);

enum avl_order {AVL_PREORDER, AVL_INORDER, AVL_POSTORDER};

typedef void(*avl_walker_f)(avl_t *avl, void *data, void *param);

avl_t *avl_new(
		avl_key_cmp_f key_cmp,
		avl_data_copy_f data_copy,
		avl_data_size_f data_size,
		avl_data_cleanup_f data_cleanup);
unsigned avl_count(avl_t *avl);
void *avl_get(avl_t *avl, void *key);
void *avl_insert(avl_t *avl, void *data);
void avl_remove(avl_t *avl, void *key);
void avl_walk(avl_t *avl, enum avl_order order, avl_walker_f walker, void* param);
void avl_destroy(avl_t *avl);

struct avl_it {
/* treat as opaque */
	struct avl_node **_sp; /* stack pointer */
	struct avl_node *_s[32]; /* stack, size is the max avl depth */
	struct avl_node *_rt;
	struct avl_node *__rt;
	struct avl_node *_c;
};

void avl_it_start(struct avl_it *it, avl_t *avl);
void avl_it_start_at(struct avl_it *it, avl_t *avl, void *key);
void *avl_it_next(struct avl_it *it);
int avl_it_has_next(struct avl_it *it);
void *avl_it_peek_prev(struct avl_it *it);
void *avl_it_peek_next(struct avl_it *it);

#endif /* LIB_COLLECTION_AVL_H_ */
