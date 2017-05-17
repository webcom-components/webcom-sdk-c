#include "webcom-c/webcom.h"

int wc_push_json_data(wc_cnx_t *cnx, char *path, char *json) {
	wc_msg_t msg;

	wc_msg_init(&msg);
	msg.type = WC_MSG_DATA;
	msg.u.data.type = WC_DATA_MSG_ACTION;
	msg.u.data.u.action.type = WC_ACTION_PUT;
	msg.u.data.u.action.r = 0;
	msg.u.data.u.action.u.put.path = path;
	msg.u.data.u.action.u.put.data = json;

	return wc_cnx_send_msg(cnx, &msg) > 0;
}
