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

#include "../lib/datasync/request.c"

#include <inttypes.h>

#include "stfu.h"

/* stubs */
int wc_datasync_send_msg(wc_context_t *cnx, wc_msg_t *msg) {
	wc_response_t resp;
	if (msg->type == WC_MSG_DATA && msg->u.data.type == WC_DATA_MSG_ACTION) {
		resp.r = msg->u.data.u.action.r;
		resp.status = "ok";
		resp.data = "{\"foo\": \"bar\"";
		wc_datasync_req_response_dispatch(cnx, &resp);
	}
	return 42;
}

wc_datasync_context_t *wc_get_datasync(wc_context_t *ctx) {
	wc_datasync_context_t *ret = NULL;
	if (ctx->datasync_init) {
		ret = &ctx->datasync;
	}
	return ret;
}

void wc_datasync_push_id(UNUSED_PARAM(struct pushid_state *s), UNUSED_PARAM(int64_t time), UNUSED_PARAM(char* buf)) {}
void wc_datasync_msg_init(UNUSED_PARAM(wc_msg_t *msg)) {}
/* end stubs */

int listen_response = 0, put_response = 0, auth_response = 0;

void cb_listen(wc_context_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data, void *user) {
	printf("\t[%p:%d:%"PRId64"] %s \"%s\"\n", cnx, type, id, status == WC_REQ_OK ? "WC_REQ_OK" : "WC_REQ_ERROR", reason);
	if(type == WC_ACTION_LISTEN) {
		listen_response = 1;
	}
}

void cb_put(wc_context_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data, void *user) {
	printf("\t[%p:%d:%"PRId64"] %s \"%s\"\n", cnx, type, id, status == WC_REQ_OK ? "WC_REQ_OK" : "WC_REQ_ERROR", reason);
	if(type == WC_ACTION_PUT) {
		put_response = 1;
	}
}

void cb_auth(wc_context_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data, void *user) {
	printf("\t[%p:%d:%"PRId64"] %s \"%s\"\n", cnx, type, id, status == WC_REQ_OK ? "WC_REQ_OK" : "WC_REQ_ERROR", reason);
	if(type == WC_ACTION_AUTHENTICATE) {
		auth_response = 1;
	}
}

int main(void)
{
	wc_context_t cnx;
	uint64_t id;
	int c;
	wc_action_trans_t *p;
	size_t i;

	memset(&cnx, 0, sizeof(cnx));

	cnx.datasync_init = 1;

	for (i = 0 ; i < 4096 ; i++) {
		wc_req_store_pending(&cnx, wc_datasync_next_reqnum(&cnx.datasync), WC_ACTION_LISTEN, NULL, NULL);
	}
	c = 0;
	for (i = 0 ; i < (1 << PENDING_ACTION_HASH_FACTOR) ; i++) {
		p = cnx.datasync.pending_req_table[i];
		while(p) {
			c++;
			p = p->next;
		}
	}

	STFU_TRUE("Store 4096 pending requests", c == 4096);

	for (id = 1 ; id <= 4096 ; id++) {
		free(wc_datasync_req_get_pending(&cnx, id));
	}

	c = 0;
	for (i = 0 ; i < (1 << PENDING_ACTION_HASH_FACTOR) ; i++) {
		p = cnx.datasync.pending_req_table[i];
		while(p) {
			c++;
			p = p->next;
		}
	}

	STFU_TRUE("Requests table is empty after all getting all entries", c == 0);

	wc_datasync_listen(&cnx, "/foo/", cb_listen, NULL);
	STFU_TRUE("Callback was called for listen request", listen_response);

	wc_datasync_put(&cnx, "/bar/", "{\"foo\": 1337}", cb_put, NULL);
	STFU_TRUE("Callback was called for put request", put_response);

	wc_datasync_auth(&cnx, "S3CRET", cb_auth, NULL);
	STFU_TRUE("Callback was called for auth request", auth_response);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
