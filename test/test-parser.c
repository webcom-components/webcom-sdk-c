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

#include <string.h>
#include <webcom-c/webcom.h>

#include "stfu.h"

int main(void) {
	wc_msg_t msg1, msg2, msg3, msg4, msg5, msg6, msg7, msg8;

	STFU_TRUE	("Key order: '123456' < '111foo'", wc_datasync_key_cmp("123456", "111foo") < 0);
	STFU_TRUE	("Key order: '123text' > '0123'", wc_datasync_key_cmp("123text", "122") > 0);
	STFU_TRUE	("Key order: '-456789' > '-567890'", wc_datasync_key_cmp("-456789", "-567890") > 0);
	STFU_TRUE	("Key order: '45678' == '45678'", wc_datasync_key_cmp("45678", "45678") == 0);
	STFU_TRUE	("Key order: '42' < '43'", wc_datasync_key_cmp("42", "43") < 0);
	STFU_TRUE	("Key order: 'FOO' == 'FOO'", wc_datasync_key_cmp("FOO", "FOO") == 0);
	STFU_TRUE	("Key order: 'FOO' < 'FOQ'", wc_datasync_key_cmp("FOO", "FOQ") < 0);
	STFU_TRUE	("Key order: 'FOR' > 'FOQ'", wc_datasync_key_cmp("FOR", "FOQ") > 0);

	char *str1 = "Good morning, that's a nice tnetennba";
	char *str2 = "{\"t\":\"c\",\"d\":{\"t\":\"h\",\"d\":{\"ts\":1492191239182,\"h\":\"\\/test\\/foo?bar=baz\",\"v\":\"5\"}}}";
	char *str3 = "{\"t\":\"c\",\"d\":{\"t\":\"s\",\"d\":\"the truth is out there\"}}";
	char *str4 = "{\"t\":\"d\",\"d\":{\"r\":3,\"a\":\"p\",\"b\":{\"p\":\"\\/brick\\/23-32\",\"d\":{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}}}}";
	char *chunked_str5[] = {
			"{\"t\":\"d\",\"d\":{\"r\":2,\"a\":\"p",
			"\",\"b\":{\"p\":",
			"\"/brick/13-37\",\"d\":{\"color\":\"whi",
			"te\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}}}}",
	};
	char *str6 = "{\"t\":\"d\",\"d\":{\"r\":3,\"b\":{\"s\":\"ok\",\"d\":\"ok\"}}}";
	char *str7 = "{\"t\":\"d\",\"d\":{\"a\":\"d\",\"b\":{\"p\":\"/brick/23-32\",\"d\":{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}}}}";
	char *str8 = "{\"t\":\"x\",\"d\":{\"r\":3,\"b\":{\"s\":\"ok\",\"d\":\"ok\"}}}";

	wc_parser_t *parser;
	int i;

	wc_datasync_msg_init(&msg1);
	wc_datasync_msg_init(&msg2);
	wc_datasync_msg_init(&msg3);
	wc_datasync_msg_init(&msg4);
	wc_datasync_msg_init(&msg5);
	wc_datasync_msg_init(&msg6);
	wc_datasync_msg_init(&msg7);
	wc_datasync_msg_init(&msg8);

	STFU_TRUE	("Parse non JSON", wc_datasync_parse_msg(str1, &msg1) == 0);

	parser = wc_datasync_parser_new();
	wc_datasync_parse_msg_ex(parser, str1, strlen(str1), &msg1);
	STFU_TRUE	("Parser error string", wc_datasync_parser_get_error(parser) != NULL);
	printf("\t%s\n", wc_datasync_parser_get_error(parser));
	wc_datasync_parser_free(parser);

	STFU_TRUE	("Parse valid handshake", wc_datasync_parse_msg(str2, &msg2) == 1);
	STFU_STR_EQ	("Handshake server path string", msg2.u.ctrl.u.handshake.server, "/test/foo?bar=baz");
	STFU_TRUE	("Parse valid shutdown message", wc_datasync_parse_msg(str3, &msg3) == 1);
	STFU_STR_EQ	("Shutdown reason string", msg3.u.ctrl.u.shutdown_reason, "the truth is out there");
	STFU_TRUE	("Parse valid put action", wc_datasync_parse_msg(str4, &msg4) == 1);
	STFU_STR_EQ	("Put action path", msg4.u.data.u.action.u.put.path, "/brick/23-32");
	STFU_STR_EQ	(
			"Put action data",
			msg4.u.data.u.action.u.put.data,
			"{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}"
	);

	STFU_TRUE   ("Create Webcom msg parser", (parser = wc_datasync_parser_new()) != NULL);
	for (i = 0 ; i < 4 ; i++) {
		if (i != 3) {
			STFU_TRUE ("Parse a 4-chunks Webcom message", wc_datasync_parse_msg_ex(parser, chunked_str5[i], strlen(chunked_str5[i]), &msg5) ==  WC_PARSER_CONTINUE);
		} else {
			STFU_TRUE ("Parse the last chunk of a 4-chunks Webcom message", wc_datasync_parse_msg_ex(parser, chunked_str5[i], strlen(chunked_str5[i]), &msg5) ==  WC_PARSER_OK);
		}
	}
	STFU_STR_EQ	("Put action path", msg5.u.data.u.action.u.put.path, "/brick/13-37");

	wc_datasync_parser_free(parser);

	STFU_TRUE	("Parse valid put response", wc_datasync_parse_msg(str6, &msg6) == 1);
	STFU_TRUE	("Check put response message type is data", msg6.type == WC_MSG_DATA);
	STFU_TRUE	("Check put response data msg is response", msg6.u.data.type == WC_DATA_MSG_RESPONSE);
	STFU_TRUE	("Check put response id is 3", msg6.u.data.u.response.r == 3L);
	STFU_STR_EQ ("Check put response status is 'ok'", msg6.u.data.u.response.status, "ok");
	STFU_STR_EQ	(
				"Check put response data",
				msg6.u.data.u.response.data,
				"\"ok\""
	);
	STFU_TRUE	("Parse valid update put push", wc_datasync_parse_msg(str7, &msg7) == 1);
	STFU_STR_EQ	(
					"Check put update put push data",
					msg7.u.data.u.push.u.update_put.data,
					"{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}"
	);

	parser = wc_datasync_parser_new();
	wc_datasync_parse_msg_ex(parser, str1, strlen(str8), &msg8);
	STFU_TRUE	("Parse valid JSON invalid webcom message", wc_datasync_parse_msg_ex(parser, str8, strlen(str8), &msg8));
	STFU_STR_EQ	("Valid JSON invalid webcom message error string", wc_datasync_parser_get_error(parser), "not a valid webcom message");
	printf("\t%s\n", wc_datasync_parser_get_error(parser));
	wc_datasync_parser_free(parser);

	wc_datasync_free(&msg1);
	wc_datasync_free(&msg2);
	wc_datasync_free(&msg3);
	wc_datasync_free(&msg4);
	wc_datasync_free(&msg5);
	wc_datasync_free(&msg6);
	wc_datasync_free(&msg7);
	wc_datasync_free(&msg8);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
