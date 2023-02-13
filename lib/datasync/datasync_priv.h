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

#include "../compat.h"

#include <stdint.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <sys/time.h>
#include <json-c/json.h>


#include "webcom-c/webcom.h"
#include "cache/treenode_cache.h"
#include "on/on_registry.h"

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
#define DATA_ROUTES_HASH_FACTOR 8
typedef struct wc_action_trans wc_action_trans_t;
typedef struct wc_datasync_data_route wc_datasync_data_route_t;

struct wc_datasync_context {
	struct wc_context *webcom;
	struct lws_client_connect_info lws_cci;
	struct lws *lws_conn;
	wc_cnx_state_t state;
	wc_parser_t *parser;
	char rxbuf[WC_RX_BUF_LEN];
	struct pushid_state pids;
	int64_t time_offset;
	int64_t last_req;
	wc_action_trans_t *pending_req_table[1 << PENDING_ACTION_HASH_FACTOR];
	wc_datasync_data_route_t *data_routes[1 << DATA_ROUTES_HASH_FACTOR];
	unsigned ws_next_reconnect_timer;
	struct on_registry *on_reg;
	data_cache_t *cache;
	struct listen_registry *listen_reg;
	unsigned stamp;
};

static inline int64_t wc_datasync_now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline int64_t wc_datasync_server_now(struct wc_datasync_context *dsctx) {
	return wc_datasync_now() - dsctx->time_offset;
}

static inline int64_t wc_datasync_next_reqnum(struct wc_datasync_context *dsctx) {
	return ++dsctx->last_req;
}

wc_action_trans_t *wc_datasync_req_get_pending(wc_context_t *dsctx, int64_t id);
void wc_datasync_free_pending_trans(wc_action_trans_t **table);
void wc_datasync_req_response_dispatch(wc_context_t *dsctx, wc_response_t *response);

void wc_datasync_push_id(struct pushid_state *s, int64_t time, char* buf) ;
void wc_datasync_dispatch_data(wc_context_t *dsctx, wc_push_t *push);
void wc_datasync_cleanup_data_routes(wc_datasync_data_route_t **table);
int wc_auth_service(wc_context_t *ctx, int fd);
void _wc_datasync_connect(wc_context_t *ctx);
void wc_datasync_service_socket(wc_context_t *ctx, struct wc_pollargs *pa);

/**
 * de-initializes the datasync service for a Webcom context
 *
 * @param ctx the Webcom context
 */
void wc_datasync_context_cleanup(struct wc_datasync_context *ds_ctx);

/**
 * Sends a datasync message to the webcom server.
 *
 * @param ctx the context
 * @param msg the webcom message to send
 * @return the number of bytes written, otherwise < 0 is returned in case of
 * failure.
 */
int wc_datasync_send_msg(wc_context_t *ctx, wc_msg_t *msg);

#endif /* SRC_WEBCOM_PRIV_H_ */
