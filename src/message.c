#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>

#include "webcom-c/webcom-msg.h"

#define IF_NOT_NULL_DO(_func, _p) do {if ((_p) != NULL) _func((_p));} while (0)

static inline void _wc_free_action(wc_action_t *msg) {
	switch (msg->type) {
	case WC_ACTION_AUTHENTICATE:
		IF_NOT_NULL_DO(free, msg->u.auth.cred);
		break;
	case WC_ACTION_LISTEN:
		IF_NOT_NULL_DO(free, msg->u.listen.path);
		break;
	case WC_ACTION_MERGE:
		IF_NOT_NULL_DO(free, msg->u.merge.path);
		IF_NOT_NULL_DO(json_object_put, msg->u.merge.data);
		break;
	case WC_ACTION_ON_DISCONNECT_CANCEL:
		IF_NOT_NULL_DO(free, msg->u.on_disc_cancel.path);
		break;
	case WC_ACTION_ON_DISCONNECT_MERGE:
		IF_NOT_NULL_DO(free, msg->u.on_disc_merge.path);
		IF_NOT_NULL_DO(json_object_put, msg->u.on_disc_merge.data);
		break;
	case WC_ACTION_ON_DISCONNECT_PUT:
		IF_NOT_NULL_DO(free, msg->u.on_disc_put.path);
		IF_NOT_NULL_DO(json_object_put, msg->u.on_disc_put.data);
		break;
	case WC_ACTION_PUT:
		IF_NOT_NULL_DO(free, msg->u.put.path);
		IF_NOT_NULL_DO(free, msg->u.put.hash);
		IF_NOT_NULL_DO(json_object_put, msg->u.put.data);
		break;
	case WC_ACTION_UNAUTHENTICATE:
		break;
	case WC_ACTION_UNLISTEN:
		IF_NOT_NULL_DO(free, msg->u.unlisten.path);
		break;
	}
}

static inline void _wc_free_push(wc_push_t *msg) {
	switch (msg->type) {
	case WC_PUSH_AUTH_REVOKED:
		IF_NOT_NULL_DO(free, msg->u.auth_revoked.reason);
		IF_NOT_NULL_DO(free, msg->u.auth_revoked.status);
		break;
	case WC_PUSH_LISTEN_REVOKED:
		IF_NOT_NULL_DO(free, msg->u.listen_revoked.path);
		break;
	case WC_PUSH_DATA_UPDATE_PUT:
		IF_NOT_NULL_DO(free, msg->u.update_put.path);
		IF_NOT_NULL_DO(json_object_put, msg->u.update_put.data);
		break;
	case WC_PUSH_DATA_UPDATE_MERGE:
		IF_NOT_NULL_DO(free, msg->u.update_merge.path);
		IF_NOT_NULL_DO(json_object_put, msg->u.update_put.data);
		break;
	}
}

static inline void _wc_free_response(wc_response_t *msg) {
	IF_NOT_NULL_DO(free, msg->status);
	IF_NOT_NULL_DO(json_object_put, msg->data);
}

static inline void _wc_free_data_msg(wc_data_msg_t *msg) {
	switch (msg->type) {
	case WC_DATA_MSG_ACTION:
		_wc_free_action(&msg->u.action);
		break;
	case WC_DATA_MSG_PUSH:
		_wc_free_push(&msg->u.push);
		break;
	case WC_DATA_MSG_RESPONSE:
		_wc_free_response(&msg->u.response);
		break;
	}
}

static inline void _wc_free_ctrl_msg(wc_ctrl_msg_t *msg) {
	switch (msg->type) {
	case WC_CTRL_MSG_HANDSHAKE:
		IF_NOT_NULL_DO(free, msg->u.handshake.server);
		IF_NOT_NULL_DO(free, msg->u.handshake.version);
		break;
	case WC_CTRL_MSG_CONNECTION_SHUTDOWN:
		IF_NOT_NULL_DO(free, msg->u.shutdown_reason);
		break;
	}
}

void wc_msg_free(wc_msg_t *msg) {
	switch (msg->type) {
	case WC_MSG_DATA:
		_wc_free_data_msg(&msg->u.data);
		break;
	case WC_MSG_CTRL:
		_wc_free_ctrl_msg(&msg->u.ctrl);
		break;
	}
}

void wc_msg_init(wc_msg_t *msg) {
	memset(msg, 0, sizeof(wc_msg_t));
}

static inline json_object* _wc_put_msg_to_json(wc_action_put_t *put) {
	json_object *jroot;

	jroot = json_object_new_object();
	json_object_object_add(jroot, "p", json_object_new_string(put->path));
	json_object_object_add(jroot, "d", put->data);
	if (put->hash) {
		json_object_object_add(jroot, "h", json_object_new_string(put->hash));
	}

	return jroot;
}
static inline json_object* _wc_data_msg_to_json(wc_data_msg_t *data) {
	json_object *jroot;
	static int64_t ref_cnt = 1;

	jroot = json_object_new_object();

	switch (data->type) {
	case WC_DATA_MSG_ACTION:
		json_object_object_add(jroot, "r", json_object_new_int64(data->u.action.r ? data->u.action.r : ref_cnt++));
		switch (data->u.action.type) {
		case WC_ACTION_PUT:
			json_object_object_add(jroot, "a", json_object_new_string("p"));
			json_object_object_add(jroot, "b", _wc_put_msg_to_json(&data->u.action.u.put));
			break;
		}
		break;
	case WC_DATA_MSG_PUSH:
		break;
	case WC_DATA_MSG_RESPONSE:
		break;
	}

	return jroot;
}

static json_object* _wc_msg_to_json(wc_msg_t *msg) {
	json_object *jroot;

	jroot = json_object_new_object();

	switch (msg->type) {
	case WC_MSG_DATA:
		json_object_object_add(jroot, "t", json_object_new_string("d"));
		json_object_object_add(jroot, "d", _wc_data_msg_to_json(&msg->u.data));
		break;
	case WC_MSG_CTRL:
		json_object_object_add(jroot, "t", json_object_new_string("c"));
		//json_object_object_add(jroot, "d", _wc_ctrl_msg_to_json(&msg->u.ctrl));
		break;
	}

	return jroot;
}

char *wc_msg_to_json_str(wc_msg_t *msg) {
	json_object *jroot;
	char *res;

	jroot = _wc_msg_to_json(msg);

	res = strdup(json_object_to_json_string_ext(jroot, JSON_C_TO_STRING_PLAIN));

	json_object_put(jroot);

	return res;
}
