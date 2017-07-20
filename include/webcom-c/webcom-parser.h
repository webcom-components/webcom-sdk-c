#ifndef INCLUDE_WEBCOM_PARSER_H_
#define INCLUDE_WEBCOM_PARSER_H_

#include <stddef.h>

#include "webcom-msg.h"


typedef struct wc_parser wc_parser_t;

/**
 * @ingroup parser
 * @{
 */

typedef enum {
	WC_PARSER_OK = 0,
	WC_PARSER_CONTINUE,
	WC_PARSER_ERROR
} wc_parser_result_t;


/**
 * Creates a new wc_parser_t object.
 *
 * @return a pointer to the new object or NULL in case of failure
 */
wc_parser_t *wc_parser_new();

/**
 * Parses a JSON text buffer to populate a wc_msg_t object.
 *
 * If you have only partial buffers, you can call this function several times
 * with the following buffers and the same parser object. It will return
 * WC_PARSER_CONTINUE until the end of the JSON document has been parsed.
 *
 * @param parser    the parser created by wc_parser_new()
 * @param buf       the JSON text buffer to parse
 * @param len       the buffer length
 * @param[out] res  the wc_msg_t object to populate
 *
 * @return WC_PARSER_OK on success, WC_PARSER_CONTINUE if the JSON document is
 * not complete, WC_ERROR if a parsing error occurred (see
 * wc_parser_get_error() to get the error description)
 *
 */
wc_parser_result_t wc_parse_msg_ex(wc_parser_t *parser, char *buf, size_t len, wc_msg_t *res);

/**
 * Returns a string describing a parsing error.
 *
 * @return the description if an error occurred, NULL otherwise
 */
const char *wc_parser_get_error(wc_parser_t *parser);

/**
 * Parses a null-terminated webcom message string.
 *
 * This function parses a null-terminated string containing a webcom message
 * (JSON document) and populates the wc_msg_t object given as parameter.
 *
 * @param str       the message string to parse
 * @param[out] res  the wc_msg_t object to populate
 *
 * @return 1 on success, 0 on error
 */
int wc_parse_msg(char *str, wc_msg_t *res);

/**
 * Frees a wc_parser_t object previously allocated by wc_parser_new()
 *
 * @param parser  the wc_parser_t object to free
 */
void wc_parser_free(wc_parser_t *parser);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_PARSER_H_ */
