#include <string.h>
#include <webcom-c/webcom.h>

#include "stfu.h"

int main(void) {
	wc_msg_t msg1, msg2, msg3, msg4, msg5, msg6, msg7;

	char *str1 = "Good morning, that's a nice tnetennba";
	char *str2 = "{\"t\":\"c\",\"d\":{\"t\":\"h\",\"d\":{\"ts\":1492191239182,\"h\":\"\\/test\\/foo?bar=baz\",\"v\":\"5\"}}}";
	char *str3 = "{\"t\":\"c\",\"d\":{\"t\":\"s\",\"d\":\"the truth is out there\"}}";
	char *str4 = "{\"t\":\"d\",\"d\":{\"r\":3,\"a\":\"p\",\"b\":{\"p\":\"/brick/23-32\",\"d\":{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}}}}";
	char *chunked_str5[] = {
			"{\"t\":\"d\",\"d\":{\"r\":2,\"a\":\"p",
			"\",\"b\":{\"p\":",
			"\"/brick/13-37\",\"d\":{\"color\":\"whi",
			"te\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}}}}",
	};
	char *str6 = "{\"t\":\"d\",\"d\":{\"r\":3,\"b\":{\"s\":\"ok\",\"d\":\"ok\"}}}";
	char *str7 = "{\"t\":\"d\",\"d\":{\"a\":\"d\",\"b\":{\"p\":\"/brick/23-32\",\"d\":{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}}}}";

	wc_parser_t *parser;
	int i;

	STFU_INIT();

	STFU_TRUE	("Parse non JSON", wc_parse_msg(str1, &msg1) == 0);
	STFU_TRUE	("Parse valid handshake", wc_parse_msg(str2, &msg2) == 1);
	STFU_STR_EQ	("Handshake server path string", msg2.u.ctrl.u.handshake.server, "/test/foo?bar=baz");
	STFU_TRUE	("Parse valid shutdown message", wc_parse_msg(str3, &msg3) == 1);
	STFU_STR_EQ	("Shutdown reason string", msg3.u.ctrl.u.shutdown_reason, "the truth is out there");
	STFU_TRUE	("Parse valid put action", wc_parse_msg(str4, &msg4) == 1);
	STFU_STR_EQ	("Put action path", msg4.u.data.u.action.u.put.path, "/brick/23-32");
	STFU_STR_EQ	(
			"Put action data",
			json_object_to_json_string_ext(msg4.u.data.u.action.u.put.data, JSON_C_TO_STRING_PLAIN),
			"{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}"
	);

	STFU_TRUE   ("Create Webcom msg parser", (parser = wc_parser_new()) != NULL);
	for (i = 0 ; i < 4 ; i++) {
		if (i != 3) {
			STFU_TRUE ("Parse a 4-chunks Webcom message", wc_parse_msg_ex(parser, chunked_str5[i], strlen(chunked_str5[i]), &msg5) ==  WC_PARSER_CONTINUE);
		} else {
			STFU_TRUE ("Parse the last chunk of a 4-chunks Webcom message", wc_parse_msg_ex(parser, chunked_str5[i], strlen(chunked_str5[i]), &msg5) ==  WC_PARSER_OK);
		}
	}
	STFU_STR_EQ	("Put action path", msg5.u.data.u.action.u.put.path, "/brick/13-37");

	wc_parser_free(parser);

	STFU_TRUE	("Parse valid put response", wc_parse_msg(str6, &msg6) == 1);
	STFU_TRUE	("Check put response message type is data", msg6.type == WC_MSG_DATA);
	STFU_TRUE	("Check put response data msg is response", msg6.u.data.type == WC_DATA_MSG_RESPONSE);
	STFU_TRUE	("Check put response id is 3", msg6.u.data.u.response.r == 3L);
	STFU_STR_EQ ("Check put response status is 'ok'", msg6.u.data.u.response.status, "ok");
	STFU_STR_EQ	(
				"Check put response data",
				json_object_to_json_string_ext(msg6.u.data.u.response.data, JSON_C_TO_STRING_PLAIN),
				"\"ok\""
	);
	STFU_TRUE	("Parse valid update put push", wc_parse_msg(str7, &msg7) == 1);
	STFU_STR_EQ	(
					"Check put update put push data",
					json_object_to_json_string_ext(msg7.u.data.u.push.u.update_put.data, JSON_C_TO_STRING_PLAIN),
					"{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}"
	);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
