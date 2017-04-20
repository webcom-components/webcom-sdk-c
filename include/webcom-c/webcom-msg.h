#ifndef WEBCOM_MSG_H_
#define WEBCOM_MSG_H_

#include <stdint.h>
#include <json-c/json.h>

/* Data message data types */

typedef enum {
	WC_DATA_MSG_ACTION,
	WC_DATA_MSG_PUSH,
	WC_DATA_MSG_RESPONSE
} wc_data_msg_type_t;

typedef enum {
	WC_ACTION_PUT,
	WC_ACTION_MERGE,
	WC_ACTION_LISTEN,
	WC_ACTION_UNLISTEN,
	WC_ACTION_AUTHENTICATE,
	WC_ACTION_UNAUTHENTICATE,
	WC_ACTION_ON_DISCONNECT_PUT,
	WC_ACTION_ON_DISCONNECT_MERGE,
	WC_ACTION_ON_DISCONNECT_CANCEL,
} wc_action_type_t;

typedef enum {
	WC_ACTION_AUTH_REVOKED,
	WC_ACTION_LISTEN_REVOKED,
	WC_ACTION_DATA_UPDATE_PUT,
	WC_ACTION_DATA_UPDATE_MERGE
} wc_push_type_t;

typedef struct {
	char *path;
	json_object *data;
	char *hash;
} wc_action_put_t;

typedef struct {
	char *path;
	json_object *data;
} wc_action_merge_t;

typedef struct {
	char *path;
	void *query_ids;
} wc_action_listen_t;

typedef struct {
	char *path;
} wc_action_unlisten_t;

typedef struct {
	char *cred;
} wc_action_auth_t;

typedef struct {
	char *path;
	json_object *data;
} wc_action_on_disc_put_t;

typedef struct {
	char *path;
	json_object *data;
} wc_action_on_disc_merge_t;

typedef struct {
	char *path;
} wc_action_on_disc_cancel_t;

typedef struct {
	wc_action_type_t type;
	int64_t r;
	union {
		wc_action_put_t put;
		wc_action_merge_t merge;
		wc_action_listen_t listen;
		wc_action_unlisten_t unlisten;
		wc_action_auth_t auth;
		wc_action_on_disc_put_t on_disc_put;
		wc_action_on_disc_merge_t on_disc_merge;
		wc_action_on_disc_cancel_t on_disc_cancel;
	} u;
} wc_action_t;

typedef struct {
	wc_push_type_t type;
	int64_t r;
	void *data; /* TODO */
} wc_response_t;

typedef struct {
	char *status;
	void *data; /* TODO */
} wc_push_t;

typedef struct {
	wc_data_msg_type_t type;
	union {
		wc_action_t action;
		wc_response_t response;
		wc_push_t push;
	} u;
} wc_data_msg_t;

/* Control message data types */

typedef enum {
	WC_CTRL_MSG_HANDSHAKE,
	WC_CTRL_MSG_CONNECTION_SHUTDOWN
} wc_ctrl_msg_type_t;

typedef struct {
	int64_t ts;
	char *server;
	char *version;
} wc_handshake_t;

typedef struct {
	wc_ctrl_msg_type_t type;
	union {
		wc_handshake_t handshake;
		char *shutdown_reason;
	} u;
} wc_ctrl_msg_t;

/* Level 1 data types */

typedef enum {
	WC_MSG_CTRL,
	WC_MSG_DATA
} wc_msg_type_t;

typedef struct {
	wc_msg_type_t type;
	union {
		wc_ctrl_msg_t ctrl;
		wc_data_msg_t data;
	} u;
} wc_msg_t;


#endif /* WEBCOM_MSG_H_ */
