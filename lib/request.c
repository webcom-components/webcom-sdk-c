#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

#include "webcom-c/webcom.h"
#include "webcom_priv.h"

typedef struct wc_action_trans {
	int64_t id;
	wc_action_type_t type;
	wc_on_req_result_t callback;
	struct wc_action_trans *next;
} wc_action_trans_t;

inline static size_t pending_req_hash(int64_t id) {
	return ((uint64_t)id) % (1 << PENDING_ACTION_HASH_FACTOR);
}

static void wc_req_store_pending(
		wc_cnx_t *cnx,
		int64_t id,
		wc_action_type_t type,
		wc_on_req_result_t callback)
{
	wc_action_trans_t *trans;
	size_t slot;

	trans = malloc(sizeof(wc_action_trans_t));
	trans->id = id;
	trans->type = type;
	trans->callback = callback;

	slot = pending_req_hash(id);

	trans->next = cnx->pending_req_table[slot];
	cnx->pending_req_table[slot] = trans;
}

wc_action_trans_t *wc_req_get_pending(wc_cnx_t *cnx, int64_t id) {
	wc_action_trans_t *cur, **prev;

	prev = &cnx->pending_req_table[pending_req_hash(id)];
	cur = *prev;

	while (cur != NULL) {
		if (cur->id == id) {
			*prev = cur->next;
			break;
		}
		prev = &(cur->next);
		cur = cur->next;
	}
	return cur;
}

void wc_req_response_dispatch(wc_cnx_t *cnx, wc_response_t *response) {
	wc_action_trans_t *trans;

	if ((trans = wc_req_get_pending(cnx, response->r)) != NULL) {
		if (trans->callback != NULL) {
			trans->callback(
					cnx,
					trans->id,
					trans->type,
					strcmp(response->status, "ok") == 0 ? WC_REQ_OK : WC_REQ_ERROR,
					response->status);
		}
		free(trans);
	}
}

void wc_free_pending_trans(wc_action_trans_t **table) {
	wc_action_trans_t *p, *q;
	size_t i;

	for (i = 0 ; i < (1 << PENDING_ACTION_HASH_FACTOR) ; i++) {
		p = table[i];
		while(p) {
			q = p->next;
			free(p);
			p = q;
		}
	}
}

#define DEFINE_REQ_FUNC(__name, __type, ... /* args */)						\
	int64_t wc_req_ ## __name(wc_cnx_t *cnx, wc_on_req_result_t callback,	\
		## __VA_ARGS__) {													\
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

DEFINE_REQ_FUNC (unauth, WC_ACTION_UNAUTHENTICATE)
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

DEFINE_REQ_FUNC (on_disc_put, WC_ACTION_ON_DISCONNECT_PUT, char *path, char *json)
	req->path = path;
	req->data = json;
END_DEFINE_REQ_FUNC

DEFINE_REQ_FUNC (on_disc_merge, WC_ACTION_ON_DISCONNECT_MERGE, char *path, char *json)
	req->path = path;
	req->data = json;
END_DEFINE_REQ_FUNC

DEFINE_REQ_FUNC (on_disc_cancel, WC_ACTION_ON_DISCONNECT_CANCEL, char *path)
	req->path = path;
END_DEFINE_REQ_FUNC

int64_t wc_req_push(wc_cnx_t *cnx, wc_on_req_result_t callback, char *path, char *json) {
	char *push_path;
	int64_t ret;
	size_t path_l;

	path_l = strlen(path);
	push_path = alloca(path_l + 22);

	memcpy(push_path, path, path_l);
	push_path[path_l] = '/';
	wc_push_id(&cnx->pids, (uint64_t)wc_server_now(cnx), push_path + path_l + 1);
	push_path[path_l + 21] = '\0';

	ret = wc_req_put(cnx, callback, push_path, json);

	return ret;
}
