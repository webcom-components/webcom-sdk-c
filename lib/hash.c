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

#include "hash.h"

void wc_djb2_hash_update(char *str, wc_hash_t *hash) {
	int c;

	while ((c = (unsigned char)*str++))
		*hash = ((*hash << 5) + *hash) + c;
}

wc_hash_t wc_djb2_hash(char *str) {
	unsigned long hash = 5381;

	wc_djb2_hash_update(str, &hash);

	return hash;
}

/* Arash Partow hash
 * see http://www.partow.net/programming/hashfunctions/index.html */
void wc_ap_hash_update(char* str, wc_hash_t *hash) {
	wc_hash_t c;

	while ((c = (unsigned char)*str++)) { /* process two elements per loop */
		*hash ^= (*hash << 7) ^ c * (*hash >> 3);
		if((c = (unsigned char)*str++)) {
			*hash ^= ~((*hash << 11) + (c ^ (*hash >> 5)));
		} else {
			break;
		}
	}
}

wc_hash_t wc_ap_hash(char* str) {
	wc_hash_t hash = (wc_hash_t)0xaaaaaaaaaaaaaaaa;

	wc_ap_hash_update(str, &hash);
	return hash;
}



/* djb2 hash */
wc_hash_t wc_str_path_hash_update(unsigned char **str, wc_hash_t hash) {
	while (**str != '/' && **str != '\0') {
		hash = ((hash << 5) + hash) + **str;
		(*str)++;
	}

	return ((hash << 5) + hash) + '/';
}

wc_hash_t wc_str_path_hash(char *path) {
	wc_hash_t hash = 177620; /* '/' hashed through djb2 */
	while (*path) {
		while (*path == '/') {
			path++;
		}
		if (*path) {
			hash = wc_str_path_hash_update((unsigned char **)&path, hash);
		}
	}
	return hash;
}
