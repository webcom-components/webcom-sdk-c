#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <openssl/rand.h>

#include "webcom_priv.h"
#include "webcom-c/webcom.h"

inline static void write_base64(char *buf, uint64_t val, uint64_t n) {
	static const char base[] = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

	while (n-- > 0) {
		buf[n] = base[val & (UINT64_MAX >> 58 )];
		val >>= 6;
	}
}

void wc_push_id(struct pushid_state *s, int64_t time, char* buf) {
	/* use the 48 low order bits from the timestamp to make the first 8 chars */
	write_base64(buf, (uint64_t)time, 8);

	if (s->last_time != time) {
		/* if the timestamp differs from the previous one, generate new random information */
		s->last_time = time;
		RAND_pseudo_bytes((unsigned char *) &s->lastrand_low, 8);
		RAND_pseudo_bytes((unsigned char *) &s->lastrand_hi, 2);
	} else {
		/* in case of a time collision, increment the last random value by one */
		s->lastrand_low++;

		/* if the low order bits (60 bits) have overflowed, increment the high order ones */
		if (!(s->lastrand_low & (UINT64_MAX >> 4))) {
			s->lastrand_hi++;
		}
	}

	/* write 12 more chars using 12 bits from lastrand_hi and 60 bits from lastrand_low */
	write_base64(buf + 8, (uint64_t)s->lastrand_hi, 2);
	write_base64(buf + 10, s->lastrand_low, 10);
}


int64_t wc_push_json_data(wc_cnx_t *cnx, char *path, char *json) {
	char pushid[20];
	char *push_path;
	int64_t ret;

	wc_push_id(&cnx->pids, (uint64_t)wc_server_now(cnx), pushid);
	asprintf(&push_path, "%s/%.20s", path, pushid);

	ret = wc_put_json_data(cnx, push_path, json);

	free(push_path);

	return ret;
}

int64_t wc_put_json_data(wc_cnx_t *cnx, char *path, char *json) {
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

	ret = wc_cnx_send_msg(cnx, &msg);
	return ret > 0 ? reqnum : -1l;
}

int64_t wc_listen(wc_cnx_t *cnx, char *path) {
	wc_msg_t msg;
	int ret;
	int64_t reqnum = wc_next_reqnum(cnx);

	wc_msg_init(&msg);
	msg.type = WC_MSG_DATA;
	msg.u.data.type = WC_DATA_MSG_ACTION;
	msg.u.data.u.action.type = WC_ACTION_LISTEN;
	msg.u.data.u.action.r = reqnum;
	msg.u.data.u.action.u.listen.path = path;

	ret = wc_cnx_send_msg(cnx, &msg);
	return ret > 0 ? reqnum : -1l;
}

int64_t wc_get_server_time(wc_cnx_t *cnx) {
	return wc_server_now(cnx);
}
