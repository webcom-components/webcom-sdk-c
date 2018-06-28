/*
 * webcom-sdk-c
 *
 * Copyright 2018 Orange
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

#include "base64.h"

static const char base[] = {
	'A','B','C','D','E','F','G','H',
	'I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X',
	'Y','Z','a','b','c','d','e','f',
	'g','h','i','j','k','l','m','n',
	'o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3',
	'4','5','6','7','8','9','+','/'
};

#define ______XX 0x03
#define ____XXXX 0x0f
#define __XXXXXX 0x3f
#define XX______ 0xc0
#define XXXX____ 0xf0

void base64_enc(const unsigned char *in, char *out, size_t in_len) {
	const unsigned char *const in_end = in + in_len;

	/*
	 * Encode the input bytes, 3 by 3
	 *
	 * Input:
	 *              0           1           2
	 *  -  -  +-----------+-----------+-----------+  -  -
	 *        | aaaaaa.bb | bbbb.cccc | cc.dddddd |
	 *  -  -  +-----------+-----------+-----------+  -  -
	 *         <--A--> <-- B --> <-- C --> <--D-->
	 *             \        \__       \____    \_______
	 * Output:      |          \           \           \
	 *              0           1           2           3
	 *  -  -  +-----------+-----------+-----------+-----------+  -  -
	 *        |  base[A]  |  base[B]  |  base[C]  |  base[D]  |
	 *  -  -  +-----------+-----------+-----------+-----------+  -  -
	 */
	for ( ; in < in_end - 2 ; in += 3, out += 4) {
		out[0] = base[  in[0] >> 2];
		out[1] = base[((in[0] & ______XX) << 4) | ((int) (in[1] & XXXX____) >> 4)];
		out[2] = base[((in[1] & ____XXXX) << 2) | ((int) (in[2] & XX______) >> 6)];
		out[3] = base[  in[2] & __XXXXXX];
	}

	/*
	 * there may be 1 or 2 input bytes left to encode
	 */
	switch (in_end - in) {
	case 1: /* in: | aaaaaa.bb | */
		out[0] = base[  in[0] >> 2];
		out[1] = base[((in[0] & ______XX) << 4)];
		out[2] = '='; /* padding */
		out[3] = '='; /* (twice) */
		break;
	case 2: /* in: | aaaaaa.bb | bbbb.cccc | */
		out[0] = base[  in[0] >> 2];
		out[1] = base[((in[0] & ______XX) << 4) | ((int) (in[1] & XXXX____) >> 4)];
		out[2] = base[((in[1] & ____XXXX) << 2)];
		out[3] = '='; /* padding */
		break;
	}
}

/* unrollable version for SHA1 to b64 encoded string */
void base64_enc_20(const unsigned char in[20], char out[28]) {
	const unsigned char *const in_end_minus_2 = in + 18;

	for ( ; in < in_end_minus_2 ; in += 3, out += 4) {
		out[0] = base[  in[0] >> 2];
		out[1] = base[((in[0] & ______XX) << 4) | ((int) (in[1] & XXXX____) >> 4)];
		out[2] = base[((in[1] & ____XXXX) << 2) | ((int) (in[2] & XX______) >> 6)];
		out[3] = base[  in[2] & __XXXXXX];
	}

	/* in: | aaaaaa.bb | bbbb.cccc | */
	out[0] = base[  in[0] >> 2];
	out[1] = base[((in[0] & ______XX) << 4) | ((int) (in[1] & XXXX____) >> 4)];
	out[2] = base[((in[1] & ____XXXX) << 2)];
	out[3] = '='; /* padding */
}
