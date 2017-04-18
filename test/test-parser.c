#include <webcom-c/webcom.h>

#include "stfu.h"

int main(void) {
	wc_msg_t msg1, msg2, msg3;

	char *str1 = "Good morning, that's a nice tnetennba";
	char *str2 = "{\"t\":\"c\",\"d\":{\"t\":\"h\",\"d\":{\"ts\":1492191239182,\"h\":\"\\/test\\/foo?bar=baz\",\"v\":\"5\"}}}";
	char *str3 = "{\"t\":\"c\",\"d\":{\"t\":\"s\",\"d\":\"the truth is out there\"}}";

	STFU_INIT();

	STFU_TRUE	("Parse non JSON", wc_parse_msg(str1, &msg1) == 0);
	STFU_TRUE	("Parse valid handshake", wc_parse_msg(str2, &msg2) == 1);
	STFU_STR_EQ	("Handshake server path string", msg2.u.ctrl.u.handshake.server, "/test/foo?bar=baz");
	STFU_TRUE	("Parse valid shutdown message", wc_parse_msg(str3, &msg3) == 1);
	STFU_STR_EQ	("Shutdown reason string", msg3.u.ctrl.u.shutdown_reason, "the truth is out there");

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
