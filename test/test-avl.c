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

#include <stdlib.h>

#include "stfu.h"

#include "../lib/collection/avl.h"

struct test_data {
	char *key;
	struct {
		size_t s;
		float f;
		char flex[];
	} value;
};

char *words[] = { "penché", "rallumant", "réarmeront", "pannetons",
		"acquisition", "banquiers", "prolongèrent", "dompterait", "scanderait",
		"condensaient", "fredaines", "discriminerai", "conjurerais",
		"relateraient", "embarrasses", "opposons", "attribut", "stockée",
		"dégonflements", "revendu", "fierai", "contribueras", "segmentaient",
		"particulariserions", "pénaux", "inauthentique", "effrités",
		"désordonnés", "circonscrits", "inclure", "chiffraient",
		"petites-filles", "ovationnèrent", "interprofessionnel", "complexer",
		"mongols", "décaissée", "effarant", "malvoyant", "lesterions",
		"brevetterait", "aigus", "piétinait", "informative", "dirigera",
		"expliquais", "généralisera", "patrimoines", "impertinent", "agressait",
		"commentateur", "acclament", "entravant", "sportivement",
		"dix-neuvièmes", "goberais", "extradons", "orthographiaient", "rougit",
		"briguerons", "pressurais", "résulteront", "emmêler", "surenchérirait",
		"contestable", "déposerons", "épouvantail", "vénérerons", "impure",
		"croc", "câbleront", "sources", "crédules", "soûlèrent", "pester",
		"balourds", "produirions", "gauchissiez", "mains", "origine",
		"filtration", "châtaignier", "éteigniez", "brimait", "inculqueriez",
		"incendie", "régionalisent", "colmaterons", "coïnculpées",
		"européanisiez", "attachait", "réorganisions", "intensifies", "libérée",
		"bombements", "triperie", "maudissait", "assurait", "rectiligne",
		"cramoisies" };

int key_cmp(void *a, void *b) {
	struct test_data *A = a, *B = b;
	return strcmp(A->key, B->key);
}

void data_copy(void *from, void *to) {
	struct test_data *f = from, *t = to;

	t->key = strdup(f->key);
	t->value = f->value;
	memcpy(t->value.flex, f->value.flex, f->value.s);
}

void data_cleanup(void *data) {
	struct test_data *d = data;
	free(d->key);
}

size_t data_size(void *data) {
	struct test_data *d = data;
	return sizeof(*d) + d->value.s;
}

int main(void) {
	avl_t *tree;
	struct test_data data, *p, *q, *r;
	char *key = "Foo";
	unsigned i;

	tree = avl_new(key_cmp, data_copy, data_size, data_cleanup);

	STFU_TRUE("Create a tree",
			tree != NULL);

	data.key = "foo";
	data.value.f = 42.42;
	data.value.s = 0;

	p = avl_insert(tree, &data);

	STFU_TRUE("Insertion returned the pointer to the stored data",
			p != NULL && p != &data);

	q = avl_get(tree, &data);

	STFU_TRUE("Getting the previously inserted data",
			p == q);

	STFU_TRUE("The element count is now 1",
			avl_count(tree) == 1);

	avl_remove(tree, &key);

	STFU_TRUE("After removing a non-existing element, the count is still 1",
			avl_count(tree) == 1);

	avl_remove(tree, &data);

	STFU_TRUE("After removing the existing element, the count is now 0",
			avl_count(tree) == 0);

	STFU_INFO("Now inserting %zu entries in the tree...", sizeof(words) / sizeof (*words));

	for (i = 0 ; i < sizeof(words) / sizeof (*words) ; i++) {
		data.key = words[i];
		data.value.f = (float)i;
		data.value.s = 0;
		avl_insert(tree, &data);
	}

	STFU_TRUE("The element count is now 100",
			avl_count(tree) == 100);

	struct avl_it it;

	avl_it_start(&it, tree);

	while((p = avl_it_next(&it))) {
		STFU_INFO("Iterating in-order: got key %s, insertion order: %g", p->key, p->value.f);
	}

	key = "aaa";
	avl_it_start_at(&it, tree, &key);
	q = NULL;

	while((p = avl_it_next(&it))) {
		r = avl_it_peek_prev(&it);
		STFU_INFO("Checking avl_it_peek_prev(%s): expect %s, got %s", p->key, q ? q->key : "(null)", r ? r->key : "(null)");

		STFU_TRUE("The previous key is correct", q == r);
		q = p;
	}

	avl_it_start_at(&it, tree, &key);

	while((p = avl_it_next(&it))) {
		STFU_INFO("Iterating from '%s': got key %s, insertion order: %g", key, p->key, p->value.f);
	}

	key = "mam";

	avl_it_start_at(&it, tree, &key);
	p = avl_it_peek_prev(&it);
	STFU_INFO("peek 1 element before '%s': got key %s, insertion order: %g", key, p->key, p->value.f);

	avl_destroy(tree);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}

