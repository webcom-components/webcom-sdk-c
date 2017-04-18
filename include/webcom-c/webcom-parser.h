#ifndef INCLUDE_WEBCOM_PARSER_H_
#define INCLUDE_WEBCOM_PARSER_H_

#include <stddef.h>

#include "webcom-msg.h"


typedef struct wc_parser wc_parser_t;
typedef enum {
	WC_PARSER_OK = 0,
	WC_PARSER_CONTINUE,
	WC_PARSER_ERROR
} wc_parser_result_t;

wc_parser_t *wc_parser_new();
wc_parser_result_t wc_parse_msg_ex(wc_parser_t *parser, char *buf, size_t len, wc_msg_t *res);
const char *wc_parser_get_error(wc_parser_t *parser);
int wc_parse_msg(char *str, wc_msg_t *res);
void wc_parser_free(wc_parser_t *parser);

#endif /* INCLUDE_WEBCOM_PARSER_H_ */
