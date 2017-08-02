#include "../src/request.c"

#include "stfu.h"

/* stubs */
int wc_cnx_send_msg(wc_cnx_t *cnx, wc_msg_t *msg) {
	wc_response_t resp;
	if (msg->type == WC_MSG_DATA && msg->u.data.type == WC_DATA_MSG_ACTION) {
		resp.r = msg->u.data.u.action.r;
		resp.status = "ok";
		resp.data = "fake reply";
		wc_req_response_dispatch(cnx, &resp);
	}
	return 42;
}

void wc_push_id(UNUSED_PARAM(struct pushid_state *s), UNUSED_PARAM(int64_t time), UNUSED_PARAM(char* buf)) {}
void wc_msg_init(UNUSED_PARAM(wc_msg_t *msg)) {}
/* end stubs */

int listen_response = 0, put_response = 0, auth_response = 0;

void cb_listen(wc_cnx_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason) {
	printf("\t[%p:%d:%lu] %s \"%s\"\n", cnx, type, id, status == WC_REQ_OK ? "WC_REQ_OK" : "WC_REQ_ERROR", reason);
	if(type == WC_ACTION_LISTEN) {
		listen_response = 1;
	}
}

void cb_put(wc_cnx_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason) {
	printf("\t[%p:%d:%lu] %s \"%s\"\n", cnx, type, id, status == WC_REQ_OK ? "WC_REQ_OK" : "WC_REQ_ERROR", reason);
	if(type == WC_ACTION_PUT) {
		put_response = 1;
	}
}

void cb_auth(wc_cnx_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason) {
	printf("\t[%p:%d:%lu] %s \"%s\"\n", cnx, type, id, status == WC_REQ_OK ? "WC_REQ_OK" : "WC_REQ_ERROR", reason);
	if(type == WC_ACTION_AUTHENTICATE) {
		auth_response = 1;
	}
}

int main(void)
{
	wc_cnx_t cnx;
	uint64_t id;
	int c;
	wc_action_trans_t *p;
	size_t i;

	memset(&cnx, 0, sizeof(cnx));

	for (i = 0 ; i < 4096 ; i++) {
		wc_req_store_pending(&cnx, wc_next_reqnum(&cnx), WC_ACTION_LISTEN, NULL);
	}
	c = 0;
	for (i = 0 ; i < (1 << PENDING_ACTION_HASH_FACTOR) ; i++) {
		p = cnx.pending_req_table[i];
		while(p) {
			c++;
			p = p->next;
		}
	}

	STFU_TRUE("Store 4096 pending requests", c == 4096);

	for (id = 1 ; id <= 4096 ; id++) {
		free(wc_req_get_pending(&cnx, id));
	}

	c = 0;
	for (i = 0 ; i < (1 << PENDING_ACTION_HASH_FACTOR) ; i++) {
		p = cnx.pending_req_table[i];
		while(p) {
			c++;
			p = p->next;
		}
	}

	STFU_TRUE("Requests table is empty after all getting all entries", c == 0);

	wc_req_listen(&cnx, cb_listen, "/foo/");
	STFU_TRUE("Callback was called for listen request", listen_response);

	wc_req_put(&cnx, cb_put, "/bar/", "{\"foo\": 1337}");
	STFU_TRUE("Callback was called for put request", put_response);

	wc_req_auth(&cnx, cb_auth, "S3CRET");
	STFU_TRUE("Callback was called for auth request", auth_response);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
