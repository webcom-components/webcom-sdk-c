/*
 * webom-sdk-c
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

#ifndef SRC_PATH_H_
#define SRC_PATH_H_

#include <stdint.h>

#include "../hash.h"

#define WC_DS_MAX_DEPTH	32
#define WC_DS_MAX_PATH_LEN	256



typedef struct wc_ds_path {
	char *_buf;
	unsigned nparts;
	uint16_t offsets[];
} wc_ds_path_t;

#define PATH_STRUCT_MAX_FLEXIBLE_SIZE (WC_DS_MAX_DEPTH * sizeof(uint16_t))
#define PATH_STRUCT_MAX_SIZE (sizeof(struct wc_ds_path) + PATH_STRUCT_MAX_FLEXIBLE_SIZE)

wc_ds_path_t *wc_datasync_path_new(const char *path);
int wc_datasync_path_parse(const char *path, struct wc_ds_path *parsed);

void wc_datasync_path_cleanup(wc_ds_path_t *path);
void wc_datasync_path_destroy(wc_ds_path_t *path);
unsigned wc_datasync_path_get_part_count(wc_ds_path_t *path);
char *wc_datasync_path_get_part(wc_ds_path_t *path, unsigned part);
int wc_datasync_path_cmp(wc_ds_path_t *a, wc_ds_path_t *b);
int wc_datasync_key_cmp(const char *sa, const char *sb);
wc_hash_t wc_datasync_path_hash(wc_ds_path_t *path);
int wc_datasync_path_starts_with(wc_ds_path_t *path, wc_ds_path_t *prefix);

extern wc_ds_path_t path_root;

#endif /* SRC_PATH_H_ */
