#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include "webcom-c/webcom.h"
#include "webcom_priv.h"

#define PENDING_HACTION_HASH_FACTOR 8

inline static size_t pending_req_hash(int64_t id) {
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

	slot = pending_req_hash(id);

	trans->next = pending_req_table[slot];
	pending_req_table[slot] = trans;
}

wc_action_trans_t *wc_req_get_pending(int64_t id) {
	wc_action_trans_t *cur, *prev;

	prev = NULL;
	cur = pending_req_table[pending_req_hash(id)];
	while (cur != NULL) {
		if (cur->id == id) {
			if (prev != NULL) {
				prev->next = cur->next;
			}
			break;
		}
		prev = cur;
		cur = cur->next;
	}
	return cur;
}

#define DEFINE_REQ_FUNC(__name, __type, ... /* args */)						\
	int64_t wc_req_ ## __name(wc_cnx_t *cnx, wc_on_req_result_t callback,	\
			__VA_ARGS__) {													\
		wc_msg_t msg;														\
		wc_action_ ## __name ## _t *req;									\
		int ret;															\
																			\
		int64_t reqnum = wc_next_reqnum(cnx);								\
		wc_msg_init(&msg);													\
		msg.type = WC_MSG_DATA;												\
		msg.u.data.type = WC_DATA_MSG_ACTION;								\
		msg.u.data.u.action.r = reqnum;										\
		req = &msg.u.data.u.action.u.__name;								\
		msg.u.data.u.action.type = (__type);								\
		wc_req_store_pending(cnx, reqnum, (__type), callback);
#define END_DEFINE_REQ_FUNC													\
		ret = wc_cnx_send_msg(cnx, &msg);									\
		return ret > 0 ? reqnum : -1l;										\
	}

DEFINE_REQ_FUNC (auth, WC_ACTION_AUTHENTICATE, char *cred)
	req->cred = cred;
END_DEFINE_REQ_FUNC

DEFINE_REQ_FUNC (unauth, WC_ACTION_UNAUTHENTICATE, ...)
	UNUSED_VAR(req);
END_DEFINE_REQ_FUNC

DEFINE_REQ_FUNC (listen, WC_ACTION_LISTEN, char *path)
	req->path = path;
END_DEFINE_REQ_FUNC

DEFINE_REQ_FUNC (unlisten, WC_ACTION_UNLISTEN, char *path)
	req->path = path;
END_DEFINE_REQ_FUNC

DEFINE_REQ_FUNC (put, WC_ACTION_PUT, char *path, char *json)
	req->path = path;
	req->data = json;
END_DEFINE_REQ_FUNC

DEFINE_REQ_FUNC (merge, WC_ACTION_MERGE, char *path, char *json)
	req->path = path;
	req->data = json;
END_DEFINE_REQ_FUNC

int64_t wc_req_push(wc_cnx_t *cnx, wc_on_req_result_t callback, char *path, char *json) {
	char pushid[20];
	char *push_path;
	int64_t ret;

	wc_push_id(&cnx->pids, (uint64_t)wc_server_now(cnx), pushid);
	asprintf(&push_path, "%s/%.20s", path, pushid);

	ret = wc_req_put(cnx, callback, push_path, json);
	wc_req_unauth(cnx, callback);
	free(push_path);

	return ret;
}
