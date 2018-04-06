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

#include "treenode.h"
#include "treenode_sibs.h"

#include "../json.h"

#include "../../sha1.h"
#include "../../base64.h"

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

void treenode_cleanup(struct treenode *node) {
	if (node->type == TREENODE_TYPE_INTERNAL && node->uval.children != NULL) {
		treenode_sibs_destroy(node->uval.children);
	}
}

void treenode_destroy(struct treenode *node) {
	treenode_cleanup(node);
	free(node);
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

static void update_internal_hash(struct treenode_sibs *l, char *key, struct treenode *n, void *param) {
	SHA1_CTX *ctx = param;
	treenode_hash_t *child_hash;
	(void)l;
	if (n->type != TREENODE_TYPE_LEAF_NULL) {
		child_hash = treenode_hash_get(n);
		SHA1Update(ctx, _U":", 1);
		SHA1Update(ctx, _U key, strlen(key));
		SHA1Update(ctx, _U":", 1);
		SHA1Update(ctx, _U child_hash->bytes, 28);
	}
}

void treenode_hash(struct treenode *n) {
	SHA1_CTX ctx;
	char hexnum[17];
	unsigned char digest[20];

	union {
		double d;
		uint64_t i;
	} conv;

	SHA1Init(&ctx);

	switch (n->type) {
	case TREENODE_TYPE_LEAF_NUMBER:
		conv.d = n->uval.number;
		snprintf(hexnum, 17, "%016"PRIx64, conv.i);
		SHA1Update(&ctx, _U "number:", 7);
		SHA1Update(&ctx, _U hexnum, 16);
		break;
	case TREENODE_TYPE_LEAF_STRING:
		SHA1Update(&ctx, _U "string:", 7);
		SHA1Update(&ctx, _U n->uval.str, strlen(n->uval.str));
		break;
	case TREENODE_TYPE_INTERNAL:
		treenode_sibs_foreach(n->uval.children, TREENODE_SIBS_INORDER, update_internal_hash, &ctx);
		break;
	default:
		break;
	}

	SHA1Final(digest, &ctx);

	base64_enc_20(digest, n->hash);
}

treenode_hash_t *treenode_hash_get(struct treenode *n) {
	treenode_hash_t *ret = NULL;
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

	return ret;
}

struct tn2json {
	char *buf;
	int len;
	int count;
};

static void update_json_str_len(struct treenode_sibs *l, char *key, struct treenode *n, void *param) {
	struct tn2json *ctx = param;

	(void)l;

	if (n->type != TREENODE_TYPE_LEAF_NULL) {
		ctx->len += json_escaped_str_len(key) + 1 /* : */ + treenode_to_json_len(n);
	}
	if (ctx->count != 1) {
		ctx->len += 1; /* , */
	}

	ctx->count--;
}

int treenode_to_json_len(struct treenode *n) {
	int ret = 0;
	struct tn2json ctx;
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
			ctx.len = 0;
			ctx.count = treenode_sibs_count(n->uval.children);
			treenode_sibs_foreach(n->uval.children, TREENODE_SIBS_INORDER, update_json_str_len, (void*)&ctx);
			ret += ctx.len;
			ret += 2; /* enclosing { ... } */
			break;
		}
	}

	return ret;
}

static void update_json_str(struct treenode_sibs *l, char *key, struct treenode *n, void *param) {
	struct tn2json *ctx = param;
	char *p = ctx->buf;

	(void)l;

	if (n->type != TREENODE_TYPE_LEAF_NULL) {
		p += json_escape_str(key, ctx->buf);
		*p++ = ':';
		p += treenode_to_json(n, p);
		if (ctx->count != 1) {
			*p++ = ',';
		}
	}

	ctx->count--;
	ctx->len += p - ctx->buf;
	ctx->buf = p;
}

int treenode_to_json(struct treenode *n, char *json) {
	char *p = json;
	struct tn2json ctx;
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

			ctx.count = treenode_sibs_count(n->uval.children);
			ctx.buf = p;
			ctx.len = 0;

			treenode_sibs_foreach(n->uval.children, TREENODE_SIBS_INORDER, update_json_str, (void*)&ctx);

			p = ctx.buf;

			*p++ = '}';

			break;
		}
	}
	*p = 0;
	return p - json;
}
