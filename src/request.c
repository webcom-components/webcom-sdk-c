#include <stdlib.h>

#include "webcom-c/webcom.h"
#include "webcom_priv.h"

#define PENDING_HACTION_HASH_FACTOR 8

inline static size_t pending_action_hash(int64_t id) {
	return ((uint64_t)id) % (1 << PENDING_HACTION_HASH_FACTOR);
}

static wc_action_trans_t* pending_req_table[1 << PENDING_HACTION_HASH_FACTOR] = {NULL};

static void wc_req_store_pending(
		wc_cnx_t *cnx,
		int64_t id,
		wc_action_type_t type,
		wc_on_req_result_t callback)
{
	wc_action_trans_t *trans;
	size_t slot;

	trans = malloc(sizeof(wc_action_trans_t));
	trans->cnx = cnx;
	trans->id = id;
	trans->type = type;
	trans->callback = callback;

	slot = pending_action_hash(id);

	if (pending_req_table[slot] == NULL) {
		trans->next = NULL;
		pending_req_table[slot] = trans;
	} else {
		trans->next = pending_req_table[slot];
		pending_req_table[slot] = trans;
	}
}

wc_action_trans_t *wc_req_get_pending(int64_t id) {
	wc_action_trans_t *cur, *prev;
	size_t slot;

	slot = pending_action_hash(id);

	if (pending_req_table[slot] == NULL) {
		return NULL;
	} else {
		prev = NULL;
		cur = pending_req_table[slot];
		do {
			if (cur->id == id) {
				if (prev != NULL) {
					prev->next = cur->next;
				}
				return cur;
			}
			prev = cur;
			cur = cur->next;
		} while (cur != NULL);
		return NULL;
	}
}

int64_t wc_req_put(wc_cnx_t *cnx, char *path, char *json, wc_on_req_result_t callback) {
	wc_msg_t msg;
	int ret;
	int64_t reqnum = wc_next_reqnum(cnx);

	wc_msg_init(&msg);
	msg.type = WC_MSG_DATA;
	msg.u.data.type = WC_DATA_MSG_ACTION;
	msg.u.data.u.action.type = WC_ACTION_PUT;
	msg.u.data.u.action.r = reqnum;
	msg.u.data.u.action.u.put.path = path;
	msg.u.data.u.action.u.put.data = json;

	wc_req_store_pending(cnx, reqnum, WC_ACTION_PUT, callback);

	ret = wc_cnx_send_msg(cnx, &msg);

	return ret > 0 ? reqnum : -1l;;
}
