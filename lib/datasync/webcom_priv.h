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

#ifndef SRC_WEBCOM_PRIV_H_
#define SRC_WEBCOM_PRIV_H_

#include "compat.h"

#include <stdint.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <sys/time.h>
#include <json-c/json.h>
#include <curl/curl.h>

#include "webcom-c/webcom.h"

typedef enum {
	WC_CNX_STATE_DISCONNECTED = 0,
	WC_CNX_STATE_CONNECTING,
	WC_CNX_STATE_CONNECTED,
	WC_CNX_STATE_DISCONNECTING,
} wc_cnx_state_t;

struct pushid_state {
	int64_t last_time;
	unsigned char lastrand[9];
	struct drand48_data rand_buffer;
};




#define WC_RX_BUF_LEN	(1 << 12)
#define PENDING_ACTION_HASH_FACTOR 8
#define DATA_HANDLERS_HASH_FACTOR 8
typedef struct wc_action_trans wc_action_trans_t;
typedef struct wc_on_data_handler wc_on_data_handler_t;

typedef struct wc_context {
	struct lws_client_connect_info lws_cci;
	struct lws *lws_conn;
	wc_on_event_cb_t callback;
	void *user;
	wc_cnx_state_t state;
	wc_parser_t *parser;
	char rxbuf[WC_RX_BUF_LEN];
	struct pushid_state pids;
	int64_t time_offset;
	int64_t last_req;
	wc_action_trans_t *pending_req_table[1 << PENDING_ACTION_HASH_FACTOR];
	wc_on_data_handler_t *handlers[1 << DATA_HANDLERS_HASH_FACTOR];
	char *app_name;
	char *host;
	uint16_t port;
	unsigned ws_next_reconnect_timer;
	char *auth_url;
	CURL *auth_curl_handle;
	CURLM *auth_curl_multi_handle;
	json_tokener *auth_parser;
	char auth_error[CURL_ERROR_SIZE];
	char *auth_form_data;
} wc_context_t;

static inline int64_t wc_now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline int64_t wc_server_now(wc_context_t *cnx) {
	return wc_now() - cnx->time_offset;
}

static inline int64_t wc_next_reqnum(wc_context_t *cnx) {
	return ++cnx->last_req;
}

wc_action_trans_t *wc_req_get_pending(wc_context_t *cnx, int64_t id);
void wc_free_pending_trans(wc_action_trans_t **table);
void wc_req_response_dispatch(wc_context_t *cnx, wc_response_t *response);

void wc_push_id(struct pushid_state *s, int64_t time, char* buf) ;
void wc_on_data_dispatch(wc_context_t *cnx, wc_push_t *push);
void wc_free_on_data_handlers(wc_on_data_handler_t **table);
void wc_auth_event_action(wc_context_t *ctx, int fd);

#endif /* SRC_WEBCOM_PRIV_H_ */
