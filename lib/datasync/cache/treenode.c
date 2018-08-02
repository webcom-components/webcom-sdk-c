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

#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <json-c/json.h>

#include "treenode.h"

#include "../json.h"

#include "../../sha1.h"
#include "../../base64.h"
#include "../path.h"

static int internal_element_key_cmp(void *a, void *b) {
	struct internal_node_element *ea = a;
	struct internal_node_element *eb = b;

	return wc_datasync_key_cmp(ea->key, eb->key);
}

static size_t internal_element_size(void *data) {
	struct internal_node_element *elem = data;

	return (elem->node.type == TREENODE_TYPE_LEAF_BOOL || elem->node.type == TREENODE_TYPE_LEAF_NULL)
			? sizeof(*elem)
			: sizeof(*elem) + sizeof (treenode_hash_t);
}

static void internal_element_data_cleanup(void *data) {
	struct internal_node_element *elem = data;

	free(elem->key);

	treenode_cleanup(&elem->node);
}

/* deep copy */
static void internal_element_copy(void *from, void *to) {
	struct internal_node_element *efrom = from;
	struct internal_node_element *eto = to;
	struct internal_node_element *e;
	struct avl_it it;

	eto->key = strdup(efrom->key);
	eto->node.hash_cached = 0;
	eto->node.type = efrom->node.type;

	switch (efrom->node.type) {
	case TREENODE_TYPE_LEAF_STRING:
		eto->node.uval.str = strdup(efrom->node.uval.str);
		break;
	case TREENODE_TYPE_INTERNAL:
		eto->node.uval.children = avl_new(internal_element_key_cmp, internal_element_copy, internal_element_size, internal_element_data_cleanup);

		avl_it_start(&it, efrom->node.uval.children);
		while ((e = avl_it_next(&it)) != NULL) {
			avl_insert(eto->node.uval.children, e);
		}
		break;
	default:
		eto->node.uval = efrom->node.uval;
		break;
	}
}

struct treenode *internal_get(struct treenode *internal, char *key) {
	struct internal_node_element *tmp;
	assert(internal->type == TREENODE_TYPE_INTERNAL);

	tmp = avl_get(internal->uval.children, &key);

	return tmp != NULL ? &tmp->node : NULL;
}

void internal_add(struct treenode *internal, char *key, struct treenode *node) {
	struct internal_node_element tmp;

	assert(internal->type == TREENODE_TYPE_INTERNAL);

	tmp.key = key;
	tmp.node = *node;

	avl_insert(internal->uval.children, &tmp);

	treenode_destroy(node);
}

void internal_remove(struct treenode *internal, char *key) {
	assert(internal->type == TREENODE_TYPE_INTERNAL);

	avl_remove(internal->uval.children, &key);
}

struct treenode *internal_add_new_number(struct treenode *internal, char *key, double number) {
	struct internal_node_element tmp;

	assert(internal->type == TREENODE_TYPE_INTERNAL);

	tmp.key = key;
	tmp.node.type = TREENODE_TYPE_LEAF_NUMBER;
	tmp.node.uval.number = number;

	return &((struct internal_node_element*)avl_insert(internal->uval.children, &tmp))->node;
}

struct treenode *internal_add_new_bool(struct treenode *internal, char *key, enum treenode_bool bool) {
	struct internal_node_element tmp;

	assert(internal->type == TREENODE_TYPE_INTERNAL);

	tmp.key = key;
	tmp.node.type = TREENODE_TYPE_LEAF_BOOL;
	tmp.node.uval.bool = bool;

	return &((struct internal_node_element*)avl_insert(internal->uval.children, &tmp))->node;
}

struct treenode *internal_add_new_string(struct treenode *internal, char *key, char *string) {
	struct internal_node_element tmp;

	assert(internal->type == TREENODE_TYPE_INTERNAL);

	tmp.key = key;
	tmp.node.type = TREENODE_TYPE_LEAF_STRING;
	tmp.node.uval.str = string;

	return &((struct internal_node_element*)avl_insert(internal->uval.children, &tmp))->node;
}

struct treenode *internal_add_new_null(struct treenode *internal, char *key) {
	struct internal_node_element tmp;

	assert(internal->type == TREENODE_TYPE_INTERNAL);

	tmp.key = key;
	tmp.node.type = TREENODE_TYPE_LEAF_NULL;
	tmp.node.uval.null = NULL;

	return &((struct internal_node_element*)avl_insert(internal->uval.children, &tmp))->node;
}

struct treenode *internal_add_new_internal(struct treenode *internal, char *key) {
	struct internal_node_element tmp;

	assert(internal->type == TREENODE_TYPE_INTERNAL);

	tmp.key = key;
	tmp.node.type = TREENODE_TYPE_INTERNAL;
	tmp.node.uval.children = NULL;

	return &((struct internal_node_element*)avl_insert(internal->uval.children, &tmp))->node;
}

struct treenode *treenode_new(enum treenode_type type, union treenode_value uval) {
	struct treenode *ret;

	ret = calloc(1,
			(type == TREENODE_TYPE_LEAF_BOOL || type == TREENODE_TYPE_LEAF_NULL)
			? sizeof(*ret)
			: sizeof(*ret) + sizeof (treenode_hash_t));
	ret->type = type;
	ret->uval = uval;

	return ret;
}

struct treenode *treenode_new_number(double number) {
	struct treenode *ret;

	ret = calloc(1, sizeof(*ret) + sizeof (treenode_hash_t));
	ret->type = TREENODE_TYPE_LEAF_NUMBER;
	ret->uval.number = number;

	return ret;
}

struct treenode *treenode_new_bool(enum treenode_bool bool) {
	struct treenode *ret;

	ret = calloc(1, sizeof(*ret));
	ret->type = TREENODE_TYPE_LEAF_BOOL;
	ret->uval.bool = bool;

	return ret;
}

struct treenode *treenode_new_string(char *string) {
	struct treenode *ret;

	ret = calloc(1, sizeof(*ret) + sizeof (treenode_hash_t));
	ret->type = TREENODE_TYPE_LEAF_STRING;
	ret->uval.str = strdup(string);

	return ret;
}

struct treenode *treenode_new_null() {
	struct treenode *ret;

	ret = calloc(1, sizeof(*ret));
	ret->type = TREENODE_TYPE_LEAF_NULL;
	ret->uval.null = NULL;

	return ret;
}

struct treenode *treenode_new_internal() {
	struct treenode *ret;

	ret = calloc(1, sizeof(*ret) + sizeof (treenode_hash_t));
	ret->type = TREENODE_TYPE_INTERNAL;
	ret->uval.children = avl_new(internal_element_key_cmp, internal_element_copy, internal_element_size, internal_element_data_cleanup);

	return ret;
}

void treenode_cleanup(struct treenode *node) {
	if (node->type == TREENODE_TYPE_INTERNAL) {
		avl_destroy(node->uval.children);
	} else if (node->type == TREENODE_TYPE_LEAF_STRING) {
		free(node->uval.str);
	}
}

void treenode_destroy(struct treenode *node) {
	if (node != NULL) {
		treenode_cleanup(node);
		free(node);
	}
}


static treenode_hash_t bool_hash[] = {
		[TN_FALSE].bytes = {
				'a','S','S','N','o','q','c','S','4','o','Q','w','J','2',
				'x','x','H','2','0','r','v','p','p','3','z','P','0','='
		},
		[TN_TRUE ].bytes = {
				'E','5','z','6','1','Q','M','0','l','N','/','U','2','W',
				's','O','n','u','s','s','z','C','T','k','R','8','M','='
		},
};

#define _U (const unsigned char *)

void treenode_hash(struct treenode *n) {
	wc_SHA1_CTX ctx;
	char hexnum[17];
	unsigned char digest[20];
	treenode_hash_t *child_hash;
	struct avl_it it;
	struct internal_node_element *p;

	union {
		double d;
		uint64_t i;
	} conv;

	wc_SHA1Init(&ctx);

	switch (n->type) {
	case TREENODE_TYPE_LEAF_NUMBER:
		conv.d = n->uval.number;
		snprintf(hexnum, 17, "%016"PRIx64, conv.i);
		wc_SHA1Update(&ctx, _U "number:", 7);
		wc_SHA1Update(&ctx, _U hexnum, 16);
		break;
	case TREENODE_TYPE_LEAF_STRING:
		wc_SHA1Update(&ctx, _U "string:", 7);
		wc_SHA1Update(&ctx, _U n->uval.str, strlen(n->uval.str));
		break;
	case TREENODE_TYPE_INTERNAL:
		avl_it_start(&it, n->uval.children);

		while ((p = avl_it_next(&it)) != NULL) {
			if (p->node.type != TREENODE_TYPE_LEAF_NULL) {
				child_hash = treenode_hash_get(&p->node);
				wc_SHA1Update(&ctx, _U":", 1);
				wc_SHA1Update(&ctx, _U p->key, strlen(p->key));
				wc_SHA1Update(&ctx, _U":", 1);
				wc_SHA1Update(&ctx, _U child_hash->bytes, 28);
			}
		}

		break;
	default:
		break;
	}

	wc_SHA1Final(digest, &ctx);

	base64_enc_20(digest, n->hash);
}

treenode_hash_t *treenode_hash_get(struct treenode *n) {
	treenode_hash_t *ret = NULL;

	if (n == NULL) goto end;

	switch (n->type) {
	case TREENODE_TYPE_LEAF_NULL:
		/* let the function return NULL */
		break;
	case TREENODE_TYPE_LEAF_BOOL:
		ret = &bool_hash[n->uval.bool];
		break;
	case TREENODE_TYPE_LEAF_NUMBER:
	case TREENODE_TYPE_LEAF_STRING:
	case TREENODE_TYPE_INTERNAL:
		if (n->hash_cached == 0) {
			treenode_hash(n);
			n->hash_cached = 1;
		}
		ret = (treenode_hash_t *)n->hash;
		break;
	}
end:
	return ret;
}

int treenode_to_json_len(struct treenode *n) {
	int ret = 0, non_null;
	struct avl_it it;
	struct internal_node_element *e;

	if (n == NULL) {
		ret = sizeof("null") - 1;
	} else {
		switch (n->type) {
		case TREENODE_TYPE_LEAF_BOOL:
			ret = (n->uval.bool == TN_TRUE) ? sizeof("true") - 1 : sizeof("false") - 1;
			break;
		case TREENODE_TYPE_LEAF_NULL:
			ret = sizeof("null") - 1;
			break;
		case TREENODE_TYPE_LEAF_NUMBER:
			ret = snprintf(NULL, 0, "%.16g", n->uval.number);
			break;
		case TREENODE_TYPE_LEAF_STRING:
			ret = json_escaped_str_len(n->uval.str);
			break;
		case TREENODE_TYPE_INTERNAL:
			avl_it_start(&it, n->uval.children);

			non_null = 0;

			while ((e = avl_it_next(&it)) != NULL) {
				if (e->node.type != TREENODE_TYPE_LEAF_NULL) {
					ret += json_escaped_str_len(e->key) + 1 /* : */ + treenode_to_json_len(&e->node);
				}
				non_null++;
			}

			if (non_null > 0) {
				ret += non_null - 1; /* separating commas */
			}

			ret += 2; /* enclosing { ... } */
			break;
		}
	}

	return ret;
}

int treenode_to_json(struct treenode *n, char *json) {
	char *p = json;
	struct avl_it it;
	struct internal_node_element *e;

	if (n == NULL) {
		*p++ = 'n'; *p++ = 'u'; *p++ = 'l'; *p++ = 'l';
	} else {
		switch (n->type) {
		case TREENODE_TYPE_LEAF_BOOL:
			if (n->uval.bool == TN_TRUE) {
				*p++ = 't'; *p++ = 'r'; *p++ = 'u'; *p++ = 'e';
			} else {
				*p++ = 'f'; *p++ = 'a'; *p++ = 'l'; *p++ = 's'; *p++ = 'e';
			}
			break;
		case TREENODE_TYPE_LEAF_NULL:
			*p++ = 'n'; *p++ = 'u'; *p++ = 'l'; *p++ = 'l';
			break;
		case TREENODE_TYPE_LEAF_NUMBER:
			p += sprintf(p, "%.16g", n->uval.number);
			break;
		case TREENODE_TYPE_LEAF_STRING:
			p += json_escape_str(n->uval.str, p);
			break;
		case TREENODE_TYPE_INTERNAL:
			*p++ = '{';

			avl_it_start(&it, n->uval.children);
			while ((e = avl_it_next(&it)) != NULL) {
				if (e->node.type != TREENODE_TYPE_LEAF_NULL) {
					p += json_escape_str(e->key, p);
					*p++ = ':';
					p += treenode_to_json(&e->node, p);
					if (avl_it_has_next(&it)) {
						*p++ = ',';
					}
				}
			}

			*p++ = '}';

			break;
		}
	}
	*p = 0;
	return p - json;
}

void ftreenode_to_json(struct treenode *n, FILE *stream) {
	struct avl_it it;
	struct internal_node_element *e;

	if (n == NULL) {
		fputs("null", stream);
	} else {
		switch (n->type) {
		case TREENODE_TYPE_LEAF_BOOL:
			if (n->uval.bool == TN_TRUE) {
				fputs("true", stream);
			} else {
				fputs("false", stream);
			}
			break;
		case TREENODE_TYPE_LEAF_NULL:
			fputs("null", stream);
			break;
		case TREENODE_TYPE_LEAF_NUMBER:
			fprintf(stream, "%.16g", n->uval.number);
			break;
		case TREENODE_TYPE_LEAF_STRING:
			fjson_escape_str(n->uval.str, stream);
			break;
		case TREENODE_TYPE_INTERNAL:
			fputc('{', stream);

			avl_it_start(&it, n->uval.children);
			while ((e = avl_it_next(&it)) != NULL) {
				if (e->node.type != TREENODE_TYPE_LEAF_NULL) {
					fjson_escape_str(e->key, stream);
					fputc(':', stream);
					ftreenode_to_json(&e->node, stream);
					if (avl_it_has_next(&it)) {
						fputc(',', stream);
					}
				}
			}

			fputc('}', stream);

			break;
		}
	}
}

int treenode_hash_eq(treenode_hash_t *h1, treenode_hash_t *h2) {
	if (h1 == h2) {
		return 1;
	}

	if (h1 == NULL || h2 == NULL) {
		return 0;
	}

	return memcmp(h1, h2, sizeof(*h1)) == 0;
}

void treenode_hash_copy(treenode_hash_t *from, treenode_hash_t *to) {
	if (from == NULL) {
		*to = (treenode_hash_t) {};
	} else {
		*to = *from;
	}
}

struct treenode *treenode_from_json_r(json_object *j) {
	struct treenode *ret = NULL;
	int i;
	char sidx[32];

	sidx[31] = 0;

	switch (json_object_get_type(j)) {
	case json_type_array:
		ret = treenode_new_internal();

		for (i = 0 ; i < (int)json_object_array_length(j) ; i++) {
			snprintf(sidx, sizeof(sidx) - 1, "%d", i);
			internal_add(ret, sidx, treenode_from_json_r(json_object_array_get_idx(j, i)));
		}
		break;
	case json_type_boolean:
		ret = treenode_new_null();
		break;
	case json_type_double:
		ret = treenode_new_number(json_object_get_double(j));
		break;
	case json_type_int:
		ret = treenode_new_number((double)json_object_get_int(j));
		break;
	case json_type_null:
		ret = treenode_new_null();
		break;
	case json_type_object:
		ret = treenode_new_internal();
		json_object_object_foreach(j, obj_key, obj_val) {
			internal_add(ret, obj_key, treenode_from_json_r(obj_val));
		}
		break;
	case json_type_string:
		ret = treenode_new_string((char *)json_object_get_string(j));
		break;
	}

	return ret;
}

struct treenode *treenode_from_json(char *json) {
	struct treenode *ret;
	json_object *j = json_tokener_parse(json);

	ret = treenode_from_json_r(j);

	json_object_put(j);

	return ret;
}
