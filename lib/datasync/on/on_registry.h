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

#ifndef LIB_DATASYNC_ON_ON_REGISTRY_H_
#define LIB_DATASYNC_ON_ON_REGISTRY_H_

#include <stdio.h>

#include "webcom-c/webcom.h"

#include "on_subscription.h"

#include "../cache/treenode_cache.h"
#include "../path.h"

struct on_registry;

struct on_registry *on_registry_new();
on_handle_t on_registry_add(wc_context_t *ctx, enum on_event_type type, char *path, on_callback_f cb);
int on_registry_remove(wc_context_t *ctx, wc_ds_path_t *path, int type_mask, on_callback_f cb);

void on_registry_destroy(struct on_registry *);

void on_registry_dispatch_on_event(struct on_registry* reg, data_cache_t *cache, char *path);

void dump_on_registry(struct on_registry* reg, FILE *f);

#endif /* LIB_DATASYNC_ON_ON_REGISTRY_H_ */
