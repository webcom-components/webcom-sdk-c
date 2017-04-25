#include <stdio.h>
#include <string.h>
#include <webcom-c/webcom.h>

#include "stfu.h"

int stop1 = 0;
int stop2 = 0;

void test_cb(wc_event_t event, wc_cnx_t *cnx, void *data, size_t len, void *user) {
	wc_msg_t *msg = (wc_msg_t*) data;
	switch (event) {
		case WC_EVENT_ON_CNX_ESTABLISHED:
			puts("ESTABLISHED");
			break;
		case WC_EVENT_ON_MSG_RECEIVED:
			STFU_TRUE	("Receive handshake", msg->type == WC_MSG_CTRL && msg->u.ctrl.type == WC_CTRL_MSG_HANDSHAKE);
			STFU_TRUE	("Handshake path exists", msg->u.ctrl.u.handshake.server != NULL);
			printf("\t%s\n", msg->u.ctrl.u.handshake.server);

			stop1 = 1;
			break;
		case WC_EVENT_ON_CNX_CLOSED:
			puts("CNX CLOSED");
			stop2 = 1;
			break;
		default:
			break;
	}
}

int main(void) {
	wc_cnx_t *cnx1;

	STFU_TRUE	("Create new wc_cnx_t object", (cnx1 = wc_cnx_new("io.datasync.orange.com", 443, "/_wss/.ws?v=5&ns=legorange", test_cb, (void *)0xDEADBEEF)) != NULL);

	wc_cnx_connect(cnx1);

	do {
		wc_service(cnx1);
	} while (!stop1);

	wc_cnx_close(cnx1);

	do {
		wc_service(cnx1);
	} while (!stop2);

	wc_cnx_free(cnx1);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
