#ifndef SRC_WEBCOM_PRIV_H_
#define SRC_WEBCOM_PRIV_H_

#include <stdint.h>
#include <stdlib.h>
#include <nopoll.h>
#include <sys/time.h>

#include "webcom-c/webcom.h"

typedef enum {
	WC_CNX_STATE_INIT = 0,
	WC_CNX_STATE_CREATED,
	WC_CNX_STATE_READY,
	WC_CNX_STATE_CLOSED,
} wc_cnx_state_t;

struct pushid_state {
	int64_t last_time;
	unsigned char lastrand[9];
	struct drand48_data rand_buffer;
};


typedef struct wc_on_data_handler {
	char *path;
	size_t path_len;
	wc_on_data_callback_t callback;
	void *user;
	struct wc_on_data_handler *next;
} wc_on_data_handler_t;

#define WC_RX_BUF_LEN	(1 << 12)
#define PENDING_ACTION_HASH_FACTOR 8
typedef struct wc_action_trans wc_action_trans_t;

typedef struct wc_cnx {
	noPollCtx *np_ctx;
	noPollConn *np_conn;
	wc_on_event_cb_t callback;
	void *user;
	int fd;
	wc_cnx_state_t state;
	wc_parser_t *parser;
	char rxbuf[WC_RX_BUF_LEN];
	struct pushid_state pids;
	int64_t time_offset;
	int64_t last_req;
	wc_action_trans_t* pending_req_table[1 << PENDING_ACTION_HASH_FACTOR];
	wc_on_data_handler_t *handlers;
} wc_cnx_t;

static inline int64_t wc_now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline int64_t wc_server_now(wc_cnx_t *cnx) {
	return wc_now() - cnx->time_offset;
}

static inline int64_t wc_next_reqnum(wc_cnx_t *cnx) {
	return ++cnx->last_req;
}



typedef struct wc_action_trans {
	int64_t id;
	wc_action_type_t type;
	wc_on_req_result_t callback;
	struct wc_action_trans *next;
} wc_action_trans_t;

wc_action_trans_t *wc_req_get_pending(wc_cnx_t *cnx, int64_t id);
void wc_push_id(struct pushid_state *s, int64_t time, char* buf) ;
void wc_on_data_dispatch(wc_cnx_t *cnx, wc_push_t *push);

#endif /* SRC_WEBCOM_PRIV_H_ */
