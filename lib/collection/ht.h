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

#ifndef SRC_HT_H_
#define SRC_HT_H_

#include "../hash.h"

typedef void ht_key_t;
typedef void ht_val_t;

typedef int (*ht_key_eq_f)(ht_key_t*, ht_key_t*);
typedef wc_hash_t (*ht_hash_f)(ht_key_t*);

typedef void (*ht_key_free_f)(ht_key_t*);
typedef void (*ht_val_free_f)(ht_val_t*);

typedef struct ht_table ht_t;

typedef struct {
	unsigned _b;
	int _s;
	ht_t *_t;
	struct ht_item *_c;
} ht_iter_t;

ht_t *ht_new(ht_hash_f, ht_key_eq_f, ht_key_free_f, ht_val_free_f);
void ht_destroy(ht_t *);
int ht_insert(ht_t *t, ht_key_t *key, ht_val_t *val);
int ht_contains(ht_t *t, ht_key_t *key);
int ht_get_ex(ht_t *t, ht_key_t *key, ht_val_t **ret);
ht_val_t *ht_get(ht_t *t, ht_key_t *key);
int ht_remove(ht_t *t, ht_key_t *key);
unsigned ht_count(ht_t *t);

void ht_it_init(ht_iter_t *it, ht_t *t);
int ht_it_fetch_next(ht_iter_t*);
ht_key_t *ht_it_key(ht_iter_t*);
ht_val_t *ht_it_val(ht_iter_t*);


#endif /* SRC_HT_H_ */
