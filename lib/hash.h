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

#ifndef LIB_HASH_H_
#define LIB_HASH_H_

typedef unsigned long wc_hash_t;


void wc_djb2_hash_update(char *str, wc_hash_t *hash);
wc_hash_t wc_djb2_hash(char *str);
void wc_ap_hash_update(char* str, wc_hash_t *hash);
wc_hash_t wc_ap_hash(char* str);
wc_hash_t wc_str_path_hash_update(unsigned char **str, wc_hash_t hash);
wc_hash_t wc_str_path_hash(char *path);

#endif /* LIB_HASH_H_ */
