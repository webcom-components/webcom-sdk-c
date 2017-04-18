#include <stdio.h>
#include <sys/time.h>
#include <stddef.h>
#include <string.h>
#include <json-c/json.h>

#include "webcom-c/webcom-msg.h"
#include "webcom-c/webcom-parser.h"

#define WCPM_CALL_CHILD_PARSER(key, child_parser_name, child_res) \
	do { \
		json_object *_wcpm_cp_jtmp; \
		if (json_object_object_get_ex(jroot, (key), &_wcpm_cp_jtmp)) { \
			return child_parser_name(_wcpm_cp_jtmp, (child_res)); \
		} \
	} while (0)

#define WCPM_CALL_DATA_CHILD_PARSER(child_parser_name, child_res) \
	WCPM_CALL_CHILD_PARSER("d", child_parser_name, child_res)

#define WCPM_GET_STRING(key, dest) \
	WCPM_GET_STRING_BEGIN(key, dest) \
	WCPM_GET_STRING_END


typedef struct wc_parser {
	json_tokener* jtok;
	const char *error;
} wc_parser_t;

static const char *wc_parse_err_str = "Not a valid webcom message";

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
	const char *s;
	json_object *jtmp;
	if (json_object_object_get_ex(jroot, "t", &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_string) {
			s = json_object_get_string(jtmp);
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

static int wc_parse_data_msg(json_object *jroot, wc_data_msg_t *res) {
	return 0;
}

static int wc_parse_msg_json(json_object *jroot, wc_msg_t *res) {
	const char *s;
	json_object *jtmp;
	if (json_object_object_get_ex(jroot, "t", &jtmp)) {
		if (json_object_get_type(jtmp) == json_type_string) {
			s = json_object_get_string(jtmp);
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

const char *wc_parser_get_error(wc_parser_t *parser) {
	return parser ? parser->error : NULL;
}

wc_parser_t *wc_parser_new() {
	wc_parser_t * parser = malloc(sizeof(wc_parser_t));
	if (parser == NULL) {
		return NULL;
	}

	memset(parser, 0, sizeof(wc_parser_t));

	parser->jtok = json_tokener_new();

	if (parser->jtok == NULL) {
		return NULL;
	}

	return parser;
}

void wc_parser_free(wc_parser_t *parser) {
	if (parser != NULL) {
		if (parser->jtok != NULL) {
			json_tokener_free(parser->jtok);
		}
		free(parser);
	}
}

wc_parser_result_t wc_parse_msg_ex(wc_parser_t *parser, char *buf, size_t len, wc_msg_t *res) {
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
		ret = wc_parse_msg_json(jroot, res);
		json_object_put(jroot);
		if (ret) {
			return WC_PARSER_OK;
		} else {
			parser->error = wc_parse_err_str;
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


int wc_parse_msg(char *str, wc_msg_t *res) {
	wc_parser_t *parser;
	int ret;

	parser = wc_parser_new();
	ret = wc_parse_msg_ex(parser, str, strlen(str), res);
	wc_parser_free(parser);

	return ret == WC_PARSER_OK;
}
