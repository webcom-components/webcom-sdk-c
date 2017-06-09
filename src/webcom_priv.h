#ifndef SRC_WEBCOM_PRIV_H_
#define SRC_WEBCOM_PRIV_H_

#include <stdint.h>
#include <nopoll.h>
#include <sys/time.h>

#include "webcom-c/webcom-cnx.h"
#include "webcom-c/webcom-parser.h"

typedef enum {
	WC_CNX_STATE_INIT = 0,
	WC_CNX_STATE_CREATED,
	WC_CNX_STATE_READY,
	WC_CNX_STATE_CLOSED,
} wc_cnx_state_t;

struct pushid_state {
	int64_t last_time;
	uint64_t lastrand_low;
	uint16_t lastrand_hi;
};

#define WC_RX_BUF_LEN	(1 << 12)

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

#endif /* SRC_WEBCOM_PRIV_H_ */
