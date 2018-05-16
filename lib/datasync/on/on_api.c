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


#include <json-c/json.h>

#include "../../webcom_base_priv.h"
#include "../path.h"
#include "../cache/treenode_cache.h"
#include "on_registry.h"

#include "on_api.h"

void wc_datasync_on_value(wc_context_t *ctx, char *path, on_value_f callback) {
	on_registry_add_on_value(ctx->datasync.on_reg, ctx->datasync.cache, path, callback);
}


void on_server_update_put(wc_context_t *ctx, char *path, json_object *data) {
	wc_ds_path_t *parsed_path;

	parsed_path = wc_datasync_path_new(path);
	data_cache_set_ex(ctx->datasync.cache, parsed_path, data);
	on_registry_dispatch_on_value_ex(ctx->datasync.on_reg, ctx->datasync.cache, parsed_path);
	wc_datasync_path_destroy(parsed_path);

}


#if 0
void wc_datasync_on_child_added(wc_context_t *ctx, char *path, on_child_added_f callback) {
	struct on_sub *sub;

	sub = on_sub_new(ON_CHILD_ADDED, path, (union on_callback) callback);

	on_registry_attach(ctx->datasync.on_reg, sub);
}

void wc_datasync_on_child_changed(wc_context_t *ctx, char *path, on_child_changed_f callback) {
	struct on_sub *sub;

	sub = on_sub_new(ON_CHILD_CHANGED, path, (union on_callback) callback);

	on_registry_attach(ctx->datasync.on_reg, sub);
}

void wc_datasync_on_child_removed(wc_context_t *ctx, char *path, on_child_changed_f callback) {
	struct on_sub *sub;

	sub = on_sub_new(ON_CHILD_REMOVED, path, (union on_callback) callback);

	on_registry_attach(ctx->datasync.on_reg, sub);
}

void wc_datasync_off_value(wc_context_t *ctx, char *path) {
	on_registry_detach(ctx->datasync.on_reg, ON_VALUE, path);
}

void wc_datasync_off_child_added(wc_context_t *ctx, char *path) {
	on_registry_detach(ctx->datasync.on_reg, ON_CHILD_ADDED, path);
}

void wc_datasync_off_child_changed(wc_context_t *ctx, char *path) {
	on_registry_detach(ctx->datasync.on_reg, ON_CHILD_CHANGED, path);
}

void wc_datasync_off_child_removed(wc_context_t *ctx, char *path) {
	on_registry_detach(ctx->datasync.on_reg, ON_CHILD_REMOVED, path);
}
#endif
