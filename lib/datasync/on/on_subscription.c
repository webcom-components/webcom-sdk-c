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

#include <stdlib.h>

#include "../path.h"

#include "on_subscription.h"

struct on_sub *on_sub_new(enum on_sub_type type, char *path, union on_callback cb) {
	struct on_sub *ret;
	wc_ds_path_t *parsed_path;

	ret = malloc(sizeof(*ret));
	parsed_path = wc_datasync_path_new(path);

	ret->cb = cb;
	ret->path = parsed_path;
	ret->status = ON_STATUS_PENDING;
	ret->type = type;
	ret->next = NULL;

	return ret;
}

void on_sub_destroy(struct on_sub *sub) {
	 wc_datasync_path_destroy(sub->path);
	 free(sub);
}
void on_sub_destroy_list(struct on_sub *sub) {
	struct on_sub *cur, *next;

	cur = sub;
	while (cur) {
		next = cur->next;
		on_sub_destroy(cur);
		cur = next;
	}
}
