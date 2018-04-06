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

#include <stdlib.h>

#include "ht.h"

#define HT_MIN_TWO_FACTOR    (3)
#define HT_MIN_SIZE          pow2(HT_MIN_TWO_FACTOR)
#define HT_MAX_TWO_FACTOR    (8 * sizeof (unsigned) - 1)
#define HT_MAX_SIZE          pow2(HT_MAX_TWO_FACTOR)
#define HT_INIT_TWO_FACTOR   HT_MIN_TWO_FACTOR
#define HT_INIT_SIZE         pow2(HT_MIN_TWO_FACTOR)

struct ht_item {
	wc_hash_t hash;
	struct ht_item *next;
	ht_key_t *key;
	ht_val_t *value;
};

struct ht_table {
	unsigned hash_two_factor;
	unsigned count;
	struct ht_item **table;
	wc_hash_t mask;
	ht_key_eq_f key_eq;
	ht_hash_f key_hash;
	ht_key_free_f key_free;
	ht_val_free_f val_free;
};

enum iter_state {iter_init, iter_iterating, iter_invalid};

static inline unsigned pow2(unsigned u) {
	return 1 << u;
}

static inline wc_hash_t mask(unsigned factor) {
	return ((wc_hash_t) (pow2(factor))) - 1;
}

static inline unsigned ht_capacity(ht_t *t) {
	return pow2(t->hash_two_factor);
}

static inline struct ht_item **ht_bucket_addr(ht_t *t, ht_key_t *key) {
	return &t->table[t->key_hash(key) & t->mask];
}

static inline int ht_is_overloaded(ht_t *t) {
	return (t->hash_two_factor < HT_MAX_TWO_FACTOR) && ((t->count >> 1) + t->count > ht_capacity(t));
}

static inline int ht_is_underloaded(ht_t *t) {
	return (t->hash_two_factor > HT_MIN_TWO_FACTOR) && ((t->count << 1) < ht_capacity(t));
}

static void ht_rehash(ht_t *t, unsigned new_factor) {
	struct ht_item **new_table;
	struct ht_item *e, **old, *next;
	wc_hash_t new_mask;
	unsigned i;

	new_mask = mask(new_factor);

	new_table = calloc(pow2(new_factor), sizeof (*new_table));

	for (i = 0 ; i < ht_capacity(t) ; i++) {
		e = t->table[i];
		while (e != NULL) {
			old = &new_table[e->hash & new_mask];
			next = e->next;
			e->next = *old;
			*old = e;
			e = next;
		}
	}

	free(t->table);
	t->table = new_table;
	t->hash_two_factor = new_factor;
	t->mask = new_mask;
}


static void ht_enlarge(ht_t *t) {
	ht_rehash(t, t->hash_two_factor + 1);
}

static void ht_reduce(ht_t *t) {
	ht_rehash(t, t->hash_two_factor - 1);
}

ht_t *ht_new(ht_hash_f hash, ht_key_eq_f eq, ht_key_free_f kf, ht_val_free_f vf) {
	ht_t *ret;

	ret = malloc(sizeof (*ret));

	if (ret != NULL) {
		ret->table = calloc(HT_INIT_SIZE, sizeof(struct ht_item*));
		if (ret->table == NULL) {
			free(ret);
			ret = NULL;
		} else {
			ret->count = 0;
			ret->hash_two_factor = HT_INIT_TWO_FACTOR;
			ret->mask = mask(HT_INIT_TWO_FACTOR);
			ret->key_hash = hash;
			ret->key_eq = eq;
			ret->key_free = kf;
			ret->val_free = vf;
		}
	}

	return ret;
}

void ht_destroy(ht_t *t) {
	struct ht_item *e, *next;
	unsigned i;

	for (i = 0 ; i < ht_capacity(t) ; i++) {
		e = t->table[i];

		while (e) {
			next = e->next;
			if (t->key_free != NULL) {
				t->key_free(e->key);
			}
			if (t->val_free != NULL) {
				t->val_free(e->value);
			}
			free(e);
			e = next;
		}
	}

	free(t->table);
	free(t);
}

int ht_insert(ht_t *t, ht_key_t *key, ht_val_t *val) {
	int ret = 0;
	struct ht_item *e, **old;

	if (ht_is_overloaded(t)) {
		ht_enlarge(t);
	}

	if (!ht_contains(t, key)) {
		e = malloc(sizeof (*e));
		e->hash = t->key_hash(key);
		e->key = key;
		e->value = val;
		old = ht_bucket_addr(t, key);
		e->next = *old;
		*old = e;
		t->count += 1;
		ret = 1;
	}

	return ret;
}

int ht_contains(ht_t *t, ht_key_t *key) {
	return ht_get_ex(t, key, NULL);
}

int ht_get_ex(ht_t *t, ht_key_t *key, ht_val_t **ret) {
	struct ht_item *e;

	for (e = *ht_bucket_addr(t, key) ; e ; e = e->next) {
		if (t->key_eq(key, e->key)) {
			if (ret != NULL) {
				*ret = e->value;
			}
			return 1;
		}
	}

	*ret = NULL;
	return 0;
}

ht_val_t *ht_get(ht_t *t, ht_key_t *key) {
	ht_val_t *ret;

	ht_get_ex(t, key, &ret);

	return ret;
}

int ht_remove(ht_t *t, ht_key_t *key) {
	int ret = 0;
	struct ht_item *e, **prev;

	prev = ht_bucket_addr(t, key);
	e = *prev;

	while (e) {
		if (t->key_eq(key, e->key)) {
			*prev = e->next;
			if (t->key_free != NULL) {
				t->key_free(e->key);
			}
			if (t->val_free != NULL) {
				t->val_free(e->value);
			}
			free(e);
			t->count -= 1;
			ret = 1;
			break;
		}
		prev = &e;
		e = e->next;
	}

	if (ht_is_underloaded(t)) {
		ht_reduce(t);
	}

	return ret;
}

unsigned ht_count(ht_t *t) {
	return t->count;
}

void ht_it_init(ht_iter_t *it, ht_t *t) {
	it->_c = NULL;
	it->_s = iter_init;
	it->_t = t;
}

int ht_it_fetch_next(ht_iter_t *it) {
	if (it->_s == iter_invalid) {
		return 0;
	} else if (it->_c != NULL && it->_c->next != NULL) {
		it->_c = it->_c->next;
		return 1;
	} else {
		if (it->_s == iter_init) {
			it->_b = 0;
			it->_s = iter_iterating;
		} else {
			it->_b = it->_b + 1;
		}
		for ( ; it->_b < ht_capacity(it->_t) ; it->_b++) {
			if (it->_t->table[it->_b] != NULL) {
				it->_c = it->_t->table[it->_b];
				return 1;
			}
		}
		it->_s = iter_invalid;
		return 0;
	}
}

ht_key_t *ht_it_key(ht_iter_t *it) {
	if (it->_s == iter_invalid)
		return NULL;
	return it->_c->key;
}

ht_val_t *ht_it_val(ht_iter_t *it) {
	if (it->_s == iter_invalid)
		return NULL;
	return it->_c->value;
}
