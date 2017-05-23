#include <string.h>
#include <webcom-c/webcom.h>

#include "stfu.h"

int main(void) {
	wc_msg_t msg1;

	char *str1 = "{\"t\":\"d\",\"d\":{\"r\":3,\"a\":\"p\",\"b\":{\"p\":\"\\/brick\\/23-32\",\"d\":{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}}}}";

	wc_msg_init(&msg1);

	char *res;

	msg1.type = WC_MSG_DATA;
	msg1.u.data.type = WC_DATA_MSG_ACTION;
	msg1.u.data.u.action.type = WC_ACTION_PUT;
	msg1.u.data.u.action.r = 3;
	msg1.u.data.u.action.u.put.path = strdup("/brick/23-32");
	msg1.u.data.u.action.u.put.data = strdup("{\"color\":\"white\",\"uid\":\"anonymous\",\"x\":23,\"y\":32}");

	res = wc_msg_to_json_str(&msg1);

	STFU_TRUE	("Message to JSON str not null", res != NULL);
	STFU_STR_EQ	("JSON document conforms to reference", str1, res);
	printf("\tgot: %s\n\tref: %s", res, str1);

	wc_msg_free(&msg1);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
