#define _GNU_SOURCE
#include <stdio.h>
#include <stdio_ext.h>
#include <string.h>
#include <webcom-c/webcom.h>

void test_cb(wc_event_t event, wc_cnx_t *cnx, void *data, size_t len, void *user) {
	wc_msg_t *msg = (wc_msg_t*) data;
	switch (event) {
		case WC_EVENT_ON_CNX_ESTABLISHED:
			break;
		case WC_EVENT_ON_MSG_RECEIVED:;
			break;
		case WC_EVENT_ON_CNX_CLOSED:
			break;
		default:
			break;
	}
}

int main(void) {
	wc_cnx_t *cnx;
	wc_msg_t msg;
	int x, y, col;
	char *col_str, nl;
	json_object *put_data;

	cnx = wc_cnx_new("io.datasync.orange.com", 443, "/_wss/.ws?v=5&ns=legorange", test_cb, 0);
	sleep(1);
	wc_cnx_on_readable(cnx);

	while (!feof(stdin)) {
		printf("\"X Y COLOR\" (1:white, 0:black)> ");
		if(scanf("%d %d %d%c", &x, &y, &col, &nl) == 4) {
			col_str = col ? "white" : "black";

			wc_msg_init(&msg);
			msg.type = WC_MSG_DATA;
			msg.u.data.type = WC_DATA_MSG_ACTION;
			msg.u.data.u.action.type = WC_ACTION_PUT;
			msg.u.data.u.action.r = 0;
			asprintf(&msg.u.data.u.action.u.put.path, "/brick/%d-%d", x, y);

			put_data = json_object_new_object();
			json_object_object_add(put_data, "color", json_object_new_string(col_str));
			json_object_object_add(put_data, "uid", json_object_new_string("anonymous"));
			json_object_object_add(put_data, "x", json_object_new_int(x));
			json_object_object_add(put_data, "y", json_object_new_int(y));

			msg.u.data.u.action.u.put.data = put_data;

			printf("debug: sending %s\n", wc_msg_to_json_str(&msg));

			if (wc_cnx_send_msg(cnx, &msg) > 0) {
				puts("OK");
			} else {
				puts("ERROR");
			}

			free(msg.u.data.u.action.u.put.path);
			json_object_put(put_data);
			wc_cnx_on_readable(cnx);
		} else {
			puts("\nBYE...");
		}
	}

	return 0;
}
