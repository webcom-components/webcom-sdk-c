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

#ifndef LIB_DATASYNC_LISTEN_REGISTRY_H_
#define LIB_DATASYNC_LISTEN_REGISTRY_H_

#include <stdio.h>

#include "webcom-c/webcom.h"
#include "../path.h"
#include "../../collection/avl.h"

enum listen_status {
	LISTEN_UNREF=-1,
	LISTEN_REQUIRED,
	LISTEN_REMOVED,
	LISTEN_PENDING, /**< a listen request is sent, waiting for the result */
	LISTEN_ACTIVE, /**< the server accepted the  */
	LISTEN_FAILED,
	LISTEN_MASKED,
};

struct listen_item {
	enum listen_status status;
	unsigned stamp;
	unsigned ref;
	wc_ds_path_t path;
};

#define LISTEN_ITEM_STRUCT_MAX_SIZE (sizeof(struct listen_item) + PATH_STRUCT_MAX_FLEXIBLE_SIZE)

struct listen_registry {
	avl_t *list;
};

struct listen_registry* listen_registry_new();
void listen_registry_destroy(struct listen_registry* lr);

struct listen_item *listen_registry_add(struct listen_registry* lr, char *path);
struct listen_item *listen_registry_add_ex(struct listen_registry* lr, wc_ds_path_t *parsed_path);
enum listen_status listen_registry_remove(struct listen_registry* lr, char *path);
enum listen_status listen_registry_remove_ex(struct listen_registry* lr, wc_ds_path_t *parsed_path);

void wc_datasync_watch(wc_context_t *ctx, char *path);
void wc_datasync_watch_ex(wc_context_t *ctx, wc_ds_path_t *parsed_path);

void wc_datasync_unwatch_ex(wc_context_t *ctx, wc_ds_path_t *parsed_path, int ref_dec);
void dump_listen_registry(struct listen_registry* lr, FILE *f) ;

#endif /* LIB_DATASYNC_LISTEN_REGISTRY_H_ */
