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

#include "../lib/webcom.c"

#include "stfu.h"

#define getRandomNumber() (4)

int main(void)
{
	wc_context_t cnx;
	unsigned char rnd_dat[12];
	char rnd_str[16];
	char push_id[20][16];
	int i, sorted;

	printf("\twebcom version: \"%s\" maj: %d, min: %d, patch: %d, "
#ifdef WEBCOM_SDK_EXTRA

			"extra: "WEBCOM_SDK_EXTRA", "
#endif
			"full:0x%06x\n",
			wc_version(), WEBCOM_SDK_MAJOR, WEBCOM_SDK_MINOR, WEBCOM_SDK_PATCH, WEBCOM_SDK_VERSION);

	memset(&cnx, 0, sizeof(cnx));
	memset(rnd_dat, 0, sizeof(rnd_dat));
	memset(rnd_str, 0, sizeof(rnd_str));

	srand48_r(getRandomNumber(), &cnx.pids.rand_buffer);

	rand_bytes(rnd_dat, 1, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 2, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 3, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 4, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 9, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 10, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 12, &cnx.pids.rand_buffer);
	STFU_TRUE("The PRNG did not segfault (yet)", 1);

	wc_b64ish_encode(rnd_str, rnd_dat, 9);
	printf("\trandom string: %.12s\n", rnd_str);
	wc_b64ish_encode(rnd_str, rnd_dat, 10);
	printf("\trandom string: %.14s\n", rnd_str);
	wc_b64ish_encode(rnd_str, rnd_dat, 11);
	printf("\trandom string: %.15s\n", rnd_str);
	STFU_TRUE("The b64-ish encoder did not segfault (yet)", 1);

	wc_b64ish_encode(rnd_str, rnd_dat, 12);
	printf("\trandom string: %.16s\n", rnd_str);
	STFU_TRUE("We generate a nice-looking 16 chars random string from a 12 bytes random buffer",
		   rnd_str[0] && rnd_str[1] && rnd_str[2] && rnd_str[3]
		&& rnd_str[4] && rnd_str[5] && rnd_str[6] && rnd_str[7]
		&& rnd_str[8] && rnd_str[9] && rnd_str[10] && rnd_str[11]
		&& rnd_str[12] && rnd_str[13] && rnd_str[14] && rnd_str[15]);

	wc_push_id(&cnx.pids, 1496911743000UL, push_id[0]);
	STFU_TRUE("The push ID for the timestamp 1496911743000 begins with '-Km5t7VN'", strncmp(push_id[0], "-Km5t7VN", 8) == 0);

	wc_push_id(&cnx.pids, 1496911736000UL, push_id[0]);
	STFU_TRUE("The push ID for the timestamp 1496911736000 begins with '-Km5t5n-'", strncmp(push_id[0], "-Km5t5n-", 8) == 0);

	for (i = 0 ; i < 16 ; i++) {
		wc_push_id(&cnx.pids, 1496911743314UL, push_id[i]);
		printf("\t%.20s\n", push_id[i]);
	}

	sorted = 1;

	for (i = 1 ; i < 15 ; i++) {
		if (strncmp(push_id[i], push_id[i + 1], 20) > 0) {
			sorted = 0;
			break;
		}
	}
	STFU_TRUE("In case of time collision, the push ID are generated in ascending order", sorted);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
