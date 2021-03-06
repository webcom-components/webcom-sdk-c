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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "webcom-c/webcom.h"
#include "../webcom_base_priv.h"

/* lrand48 is documented to return a long int in the [0, 2^31[ range */
#define RANDOM_BYTES_PER_LRAND48 3

static void rand_bytes(unsigned char *buf, size_t num, struct drand48_data *rand_buffer) {
	size_t i;
	size_t j;
	long int rnd;
	for (i = 0 ; i < num / RANDOM_BYTES_PER_LRAND48 ; i++) {
		lrand48_r(rand_buffer, &rnd);
		for (j = 0 ; j < RANDOM_BYTES_PER_LRAND48 ; j++) {
			*buf++ = (unsigned char)rnd;
			rnd >>= 8;
		}
	}
	if (num % RANDOM_BYTES_PER_LRAND48 > 0) {
		lrand48_r(rand_buffer, &rnd);
		for (j = 0 ; j < num % RANDOM_BYTES_PER_LRAND48 ; j++) {
			*buf++ = (unsigned char)rnd;
			rnd >>= 8;
		}
	}
}

/* base64-ish encoding used to encode the timestamp and random bits for the push id */
inline static void wc_b64ish_encode(char *out, unsigned char *in, size_t n) {
	static const char base[] = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
	uint32_t tmp;
	size_t ndiv = n / 3;
	const size_t nmod = n % 3;

	in += n;
	out += ((n - nmod) * 8) / 6 + (nmod ? 1 + nmod: 0);
	while (ndiv--) {
		/* 3 input bytes produce 4 output characters */
		tmp = (uint32_t)*--in;
		tmp += ((uint32_t)*--in) << 8;
		tmp += ((uint32_t)*--in) << 16;

		*--out = base[tmp & 0x3f];
		*--out = base[(tmp >> 6) & 0x3f];
		*--out = base[(tmp >> 12) & 0x3f];
		*--out = base[(tmp >> 18) & 0x3f];
	}

	switch (nmod) {
	case 2:
		/* 2 trailing bytes (16 bits) produce 3 output characters (18 bits) */
		tmp = (uint32_t)*--in;
		tmp += ((uint32_t)*--in) << 8;
		*--out = base[tmp & 0x3f];
		*--out = base[(tmp >> 6) & 0x3f];
		*--out = base[(tmp >> 18) & 0x3f];
		break;
	case 1:
		/* 1 trailing byte (8 bits) produce 2 output characters (12 bits) */
		tmp = (uint32_t)*--in;
		*--out = base[tmp & 0x3f];
		*--out = base[(tmp >> 6) & 0x3f];
		break;
	}
}

void wc_datasync_push_id(struct pushid_state *s, int64_t time, char* buf) {
	uint64_t time_be;

	time_be = htobe64((uint64_t)time);
	/* use the 48 low order bits (6 bytes) from the timestamp to make the first 8 chars */
	wc_b64ish_encode(buf, ((unsigned char*)&time_be) + 2, 6);

	if (s->last_time != time) {
		/* if the timestamp differs from the previous one, generate new random information */
		s->last_time = time;
		rand_bytes(s->lastrand, 9, &s->rand_buffer);
	} else {
		/* otherwise, increment the last rand array by one
		 *
		 * this array is 9 bytes long, therefore we split it as 64 bits low order bits
		 * plus 8 bits high order bits
		 *
		 * s->lastrand
		 *   |
		 *   v
		 * +---+---+---+---+---+---+---+---+---+
		 * | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
		 * +---+---+---+---+---+---+---+---+---+
		 *   |  \_____________________________/
		 *   v                 v
		 *  high              low
		 */

		uint64_t low;

		memcpy((void*)&low, s->lastrand + 1, 8);
		/* the value is stored in big endian notation in the array, fetch the host representation: */
		low = be64toh(low);
		low++;
		if (low == 0) {
			/* increment the hi byte */
			(*(s->lastrand))++;
		}
		/* bach to big endian */
		low = htobe64(low);
		memcpy(s->lastrand + 1, (void*)&low, 8);
	}

	/* write 12 more chars using the 9 random bytes */
	wc_b64ish_encode(buf + 8, s->lastrand, 9);
}

int64_t wc_datasync_server_time(wc_context_t *cnx) {
	return wc_datasync_server_now(wc_get_datasync(cnx));
}

void wc_datasync_gen_push_id(wc_context_t *cnx, char *result) {
	wc_datasync_push_id(&cnx->datasync.pids, (uint64_t)wc_datasync_server_now(wc_get_datasync(cnx)), result);
}
