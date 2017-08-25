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

#ifndef WEBCOM_MSG_H_
#define WEBCOM_MSG_H_

#include <stdint.h>

/**
 * @ingroup webcom-messages
 * @{
 */

typedef enum {
	WC_DATA_MSG_ACTION = 1,
	WC_DATA_MSG_PUSH,
	WC_DATA_MSG_RESPONSE
} wc_data_msg_type_t;

typedef enum {
	WC_ACTION_PUT = 1,
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
	WC_PUSH_AUTH_REVOKED = 1,
	WC_PUSH_LISTEN_REVOKED,
	WC_PUSH_DATA_UPDATE_PUT,
	WC_PUSH_DATA_UPDATE_MERGE
} wc_push_type_t;

typedef struct {
	char *path;
	char *data;
	char *hash;
} wc_action_put_t;

typedef struct {
	char *path;
	char *data;
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

typedef void *wc_action_unauth_t;

typedef struct {
	char *path;
	char *data;
} wc_action_on_disc_put_t;

typedef struct {
	char *path;
	char *data;
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
		wc_action_unauth_t unauth;
		wc_action_on_disc_put_t on_disc_put;
		wc_action_on_disc_merge_t on_disc_merge;
		wc_action_on_disc_cancel_t on_disc_cancel;
	} u;
} wc_action_t;

typedef struct {
	int64_t r;
	char *status;
	char *data;
} wc_response_t;

typedef struct {
	char *status;
	char *reason;
} wc_push_auth_revoked_t;

typedef struct {
	char *path;
} wc_push_listen_revoked_t;

typedef struct {
	char *path;
	char *data;
} wc_push_data_update_put_t;

typedef struct {
	char *path;
	char *data;
} wc_push_data_update_merge_t;

typedef struct {
	wc_push_type_t type;
	char *status;
	union {
		wc_push_auth_revoked_t auth_revoked;
		wc_push_listen_revoked_t listen_revoked;
		wc_push_data_update_put_t update_put;
		wc_push_data_update_merge_t update_merge;
	} u;
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
	WC_CTRL_MSG_HANDSHAKE = 1,
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
	WC_MSG_CTRL = 1,
	WC_MSG_DATA
} wc_msg_type_t;

typedef struct {
	wc_msg_type_t type;
	union {
		wc_ctrl_msg_t ctrl;
		wc_data_msg_t data;
	} u;
} wc_msg_t;

/**
 * does the internal initialization of a message structure
 *
 * @param msg pointer to the message to initialize
 */
void wc_msg_init(wc_msg_t *msg);

/**
 * frees the memory allocated for a message structure
 *
 * This function frees every chunk of memory used by this message (strings,
 * JSON fragments, ...). Note: this does not free the space used by the
 * wc_msg_t structure itself, you have to manually free() it after using
 * this function if if was malloc()'ed.
 *
 * @param msg the message structure to be freed
 */
void wc_msg_free(wc_msg_t *msg);


/**
 * makes a JSON string representation of a Webcom message
 *
 * @param msg the webcom message
 *
 * @return a newly malloc'd JSON string (must be manually free'd after use);
 */
char *wc_msg_to_json_str(wc_msg_t *msg);

/**
 * @}
 */

#endif /* WEBCOM_MSG_H_ */
