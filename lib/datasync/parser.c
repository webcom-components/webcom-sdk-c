/*
 * Webcom C SDK
 *
 * Copyright 2017 Orange
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

#include <stdio.h>
#include <sys/time.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>

#include "webcom-c/webcom-msg.h"
#include "webcom-c/webcom-parser.h"

#include "path.h"

#define WCPM_CALL_CHILD_PARSER(key, child_parser_name, child_res) \
	do { \
		json_object *_wcpm_cp_jtmp; \
		if (json_object_object_get_ex(jroot, (key), &_wcpm_cp_jtmp)) { \
			return child_parser_name(_wcpm_cp_jtmp, (child_res)); \
		} \
	} while (0)

#define WCPM_CALL_DATA_CHILD_PARSER(child_parser_name, child_res) \
	WCPM_CALL_CHILD_PARSER("d", child_parser_name, child_res)

#define WCPM_CALL_BODY_CHILD_PARSER(child_parser_name, child_res) \
	WCPM_CALL_CHILD_PARSER("b", child_parser_name, child_res)

typedef struct wc_parser {
	json_tokener* jtok;
	const char *error;
} wc_parser_t;

const char *wc_parse_err_not_wc = "not a valid webcom message";
const char *wc_parse_err_parser_null = "parser is NULL";

__attribute__((always_inline))
static inline int _wc_hlp_get_string(json_object *j, char *key, char **s) {
	json_object *jtmp;

	if (json_object_object_get_ex(j, key, &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_string) {
			*s = strdup(json_object_get_string(jtmp));
			return 1;
		}
	}
	return 0;
}

__attribute__((always_inline))
static inline int _wc_hlp_get_json_str(json_object *j, char *key, char **s) {
	json_object *jtmp;

	if (json_object_object_get_ex(j, key, &jtmp)) {
		*s = strdup(json_object_to_json_string_ext(jtmp, JSON_C_TO_STRING_PLAIN));
		return 1;
	}
	return 0;
}

struct _wc_hlp_json_keyval {
	char *key;
	json_object *val;
};

static int cmp_json_keys(const void *a, const void *b) {
	return wc_datasync_key_cmp(
			((struct _wc_hlp_json_keyval *)a)->key,
			((struct _wc_hlp_json_keyval *)b)->key);
}

static int _wc_hlp_get_level1_sorted_json_str(json_object *j, char *key, char **s) {
	json_object *jtmp;
	struct lh_table *t;
	struct lh_entry *it;

	if (json_object_object_get_ex(j, key, &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_object) {
			struct _wc_hlp_json_keyval *sorted_keys;
			int i;

			t = json_object_get_object(jtmp);
			sorted_keys = malloc(t->count * sizeof(struct _wc_hlp_json_keyval));

			if (sorted_keys == NULL) {
				return 0;
			}

			it = t->head;
			for (i = 0; i < t->count ; i++) {
				sorted_keys[i].key = (char *)it->k;
				sorted_keys[i].val = (struct json_object*)it->v;
				it = it->next;
			}

			qsort(sorted_keys, t->count, sizeof(struct _wc_hlp_json_keyval), cmp_json_keys);
			/* now the sorted_keys name is relevant */

			jtmp = json_object_new_object();

			for (i = 0 ; i < t->count ; i++) {
				json_object_get(sorted_keys[i].val);
				json_object_object_add(jtmp, sorted_keys[i].key, sorted_keys[i].val);
			}

			*s = strdup(json_object_to_json_string_ext(jtmp, JSON_C_TO_STRING_PLAIN));
			json_object_put(jtmp);

			free(sorted_keys);
			return 1;
		} else {
			*s = strdup(json_object_to_json_string_ext(jtmp, JSON_C_TO_STRING_PLAIN));
			return 1;
		}
	}
	return 0;
}

__attribute__((always_inline))
static inline int _wc_hlp_get_long(json_object *j, const char *key, int64_t *l) {
	json_object *jtmp;

	if (json_object_object_get_ex(j, key, &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_int) {
			*l = json_object_get_int64(jtmp);
			return 1;
		}
	}
	return 0;
}

static int wc_parse_handshake(json_object *jroot, wc_handshake_t *res) {
	return (
			_wc_hlp_get_long(jroot, "ts", &res->ts)
			&& _wc_hlp_get_string(jroot, "h", &res->server)
			&& _wc_hlp_get_string(jroot, "v", &res->version)
		);
}

static int wc_parse_ctrl_msg(json_object *jroot, wc_ctrl_msg_t *res) {
	json_object *jtmp;
	if (json_object_object_get_ex(jroot, "t", &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_string) {
			const char *s = json_object_get_string(jtmp);
			if (strcmp("h", s) == 0) {
				res->type = WC_CTRL_MSG_HANDSHAKE;
				WCPM_CALL_DATA_CHILD_PARSER(wc_parse_handshake, &res->u.handshake);
			} else if (strcmp("s", s) == 0) {
				res->type = WC_CTRL_MSG_CONNECTION_SHUTDOWN;
				return _wc_hlp_get_string(jroot, "d", &res->u.shutdown_reason);
			}
		}
	}

	return 0;
}

static int wc_parse_action_put(json_object *jroot, wc_action_put_t *res) {
	if (!_wc_hlp_get_string(jroot, "h", &res->hash))
	{
		res->hash = NULL;
	}
	return _wc_hlp_get_string(jroot, "p", &res->path) && _wc_hlp_get_json_str(jroot, "d", &res->data);
}

static int wc_parse_action_merge(json_object *jroot, wc_action_merge_t *res) {
	return _wc_hlp_get_string(jroot, "p", &res->path)
			&& _wc_hlp_get_json_str(jroot, "d", &res->data);
}

static int wc_parse_action_listen(json_object *jroot, wc_action_listen_t *res) {
	res->query_ids = NULL;
	return _wc_hlp_get_string(jroot, "p", &res->path);
}

static int wc_parse_action_unlisten(json_object *jroot, wc_action_unlisten_t *res) {
	return _wc_hlp_get_string(jroot, "p", &res->path);
}

static int wc_parse_action_auth(json_object *jroot, wc_action_auth_t *res) {
	return _wc_hlp_get_string(jroot, "cred", &res->cred);
}

static int wc_parse_action_on_disc_put(json_object *jroot, wc_action_on_disc_put_t *res) {
	return _wc_hlp_get_string(jroot, "p", &res->path)
			&& _wc_hlp_get_json_str(jroot, "d", &res->data);;
}

static int wc_parse_action_on_disc_merge(json_object *jroot, wc_action_on_disc_merge_t *res) {
	return _wc_hlp_get_string(jroot, "p", &res->path)
			&& _wc_hlp_get_json_str(jroot, "d", &res->data);;
}

static int wc_parse_action_on_disc_cancel(json_object *jroot, wc_action_on_disc_cancel_t *res) {
	return _wc_hlp_get_string(jroot, "p", &res->path);
}

static int wc_parse_action(json_object *jroot, wc_action_t *res) {
	json_object *jtmp;

	if (!_wc_hlp_get_long(jroot, "r", &res->r)) {
		return 0;
	}

	if (json_object_object_get_ex(jroot, "a", &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_string) {
			const char *s = json_object_get_string(jtmp);
			if (strcmp("p", s) == 0) {
				res->type = WC_ACTION_PUT;
				WCPM_CALL_BODY_CHILD_PARSER(wc_parse_action_put, &res->u.put);
			} else if (strcmp("m", s) == 0) {
				res->type = WC_ACTION_MERGE;
				return wc_parse_action_merge(jroot, &res->u.merge);
			} else if (strcmp("l", s) == 0) {
				res->type = WC_ACTION_LISTEN;
				return wc_parse_action_listen(jroot, &res->u.listen);
			} else if (strcmp("u", s) == 0) {
				res->type = WC_ACTION_UNLISTEN;
				return wc_parse_action_unlisten(jroot, &res->u.unlisten);
			} else if (strcmp("auth", s) == 0) {
				res->type = WC_ACTION_AUTHENTICATE;
				return wc_parse_action_auth(jroot, &res->u.auth);
			} else if (strcmp("unauth", s) == 0) {
				res->type = WC_ACTION_UNAUTHENTICATE;
				return 1;
			} else if (strcmp("o", s) == 0) {
				res->type = WC_ACTION_ON_DISCONNECT_PUT;
				return wc_parse_action_on_disc_put(jroot, &res->u.on_disc_put);
			} else if (strcmp("om", s) == 0) {
				res->type = WC_ACTION_ON_DISCONNECT_MERGE;
				return wc_parse_action_on_disc_merge(jroot, &res->u.on_disc_merge);
			} else if (strcmp("oc", s) == 0) {
				res->type = WC_ACTION_ON_DISCONNECT_CANCEL;
				return wc_parse_action_on_disc_cancel(jroot, &res->u.on_disc_cancel);
			}
		}
	}

	return 0;
}

static int wc_parse_response(json_object *jroot, wc_response_t *res) {
	json_object *jtmp;
	if (_wc_hlp_get_long(jroot, "r", &res->r)) {
		if (json_object_object_get_ex(jroot, "b", &jtmp)) {
			return _wc_hlp_get_string(jtmp, "s", &res->status)
					&& _wc_hlp_get_json_str(jtmp, "d", &res->data);
		}
	}
	return 0;
}

static int wc_parse_push_auth_revoked(json_object *jroot, wc_push_auth_revoked_t *res) {
	return _wc_hlp_get_string(jroot, "s", &res->status)
			&& _wc_hlp_get_string(jroot, "d", &res->reason);
}

static int wc_parse_push_listen_revoked(json_object *jroot, wc_push_listen_revoked_t *res) {
	return _wc_hlp_get_string(jroot, "p", &res->path);
}

static int wc_parse_push_update_put(json_object *jroot, wc_push_data_update_put_t *res) {
	return _wc_hlp_get_string(jroot, "p", &res->path)
			&& _wc_hlp_get_level1_sorted_json_str(jroot, "d", &res->data);
}

static int wc_parse_push_update_merge(json_object *jroot, wc_push_data_update_merge_t *res) {
	return _wc_hlp_get_string(jroot, "p", &res->path)
			&& _wc_hlp_get_level1_sorted_json_str(jroot, "d", &res->data);
}

static int wc_parse_push(json_object *jroot, wc_push_t *res) {
	json_object *jtmp;

	if (json_object_object_get_ex(jroot, "a", &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_string) {
			const char *s = json_object_get_string(jtmp);
			if (strcmp("ac", s) == 0) {
				res->type = WC_PUSH_AUTH_REVOKED;
				WCPM_CALL_BODY_CHILD_PARSER(wc_parse_push_auth_revoked, &res->u.auth_revoked);
			} else if (strcmp("c", s) == 0) {
				res->type = WC_PUSH_LISTEN_REVOKED;
				WCPM_CALL_BODY_CHILD_PARSER(wc_parse_push_listen_revoked, &res->u.listen_revoked);
			} else if (strcmp("d", s) == 0) {
				res->type = WC_PUSH_DATA_UPDATE_PUT;
				WCPM_CALL_BODY_CHILD_PARSER(wc_parse_push_update_put, &res->u.update_put);
			} else if (strcmp("m", s) == 0) {
				res->type = WC_PUSH_DATA_UPDATE_MERGE;
				WCPM_CALL_BODY_CHILD_PARSER(wc_parse_push_update_merge, &res->u.update_merge);
			}
		}
	}
	return 0;
}

static int wc_parse_data_msg(json_object *jroot, wc_data_msg_t *res) {
	json_object *jtmp;
	if (json_object_object_get_ex(jroot, "a", &jtmp)
			&& json_object_object_get_ex(jroot, "r", &jtmp)
			&& json_object_object_get_ex(jroot, "b", &jtmp)) {
		res->type = WC_DATA_MSG_ACTION;
		return wc_parse_action(jroot, &res->u.action);
	} else if (!json_object_object_get_ex(jroot, "a", &jtmp)
			&& json_object_object_get_ex(jroot, "r", &jtmp)) {
		res->type = WC_DATA_MSG_RESPONSE;
		return wc_parse_response(jroot, &res->u.response);
	} else if (json_object_object_get_ex(jroot, "a", &jtmp)
			&& !json_object_object_get_ex(jroot, "r", &jtmp)) {
		res->type = WC_DATA_MSG_PUSH;
		return wc_parse_push(jroot, &res->u.push);
	}
	return 0;
}

static int wc_parse_msg_json(json_object *jroot, wc_msg_t *res) {
	json_object *jtmp;
	if (json_object_object_get_ex(jroot, "t", &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_string) {
			const char *s = json_object_get_string(jtmp);
			if (strcmp("c", s) == 0) {
				res->type = WC_MSG_CTRL;
				WCPM_CALL_DATA_CHILD_PARSER(wc_parse_ctrl_msg, &res->u.ctrl);
			} else if (strcmp("d", s) == 0) {
				res->type = WC_MSG_DATA;
				WCPM_CALL_DATA_CHILD_PARSER(wc_parse_data_msg, &res->u.data);
			}
		}
	}

	return 0;
}

const char *wc_datasync_parser_get_error(wc_parser_t *parser) {
	return parser ? parser->error : wc_parse_err_parser_null;
}

wc_parser_t *wc_datasync_parser_new() {
	wc_parser_t * parser = malloc(sizeof(wc_parser_t));
	if (parser == NULL) {
		return NULL;
	}

	memset(parser, 0, sizeof(wc_parser_t));

	parser->jtok = json_tokener_new();

	if (parser->jtok == NULL) {
		free(parser);
		return NULL;
	}

	return parser;
}

void wc_datasync_parser_free(wc_parser_t *parser) {
	if (parser != NULL) {
		if (parser->jtok != NULL) {
			json_tokener_free(parser->jtok);
		}
		free(parser);
	}
}

wc_parser_result_t wc_datasync_parse_msg_ex(wc_parser_t *parser, char *buf, size_t len, wc_msg_t *res) {
	enum json_tokener_error jte;
	int ret;
	json_object* jroot;

	if (parser == NULL) {
		return WC_PARSER_ERROR;
	}

	jroot = json_tokener_parse_ex(parser->jtok, (char *)buf, len);
	jte = json_tokener_get_error(parser->jtok);

	switch (jte) {
	case json_tokener_success:
		memset(res, 0, sizeof(wc_msg_t));
		ret = wc_parse_msg_json(jroot, res);
		json_object_put(jroot);
		if (ret) {
			return WC_PARSER_OK;
		} else {
			parser->error = wc_parse_err_not_wc;
			return WC_PARSER_ERROR;
		}
		break;
	case json_tokener_continue:
		return WC_PARSER_CONTINUE;
		break;
	default:
		parser->error = json_tokener_error_desc(jte);
		return WC_PARSER_ERROR;
		break;
	}
}

int wc_datasync_parse_msg(char *str, wc_msg_t *res) {
	wc_parser_t *parser;
	int ret;

	parser = wc_datasync_parser_new();
	ret = wc_datasync_parse_msg_ex(parser, str, strlen(str), res);
	wc_datasync_parser_free(parser);

	return ret == WC_PARSER_OK;
}

