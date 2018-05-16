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

#ifndef LIB_DATASYNC_ON_ON_API_H_
#define LIB_DATASYNC_ON_ON_API_H_

#include "webcom-c/webcom.h"
#include "on_subscription.h"

void on_server_update_put(wc_context_t *ctx, char *path, json_object *data);

void wc_datasync_on_value(wc_context_t *ctx, char *path, on_value_f callback);
void wc_datasync_on_child_added(wc_context_t *ctx, char *path, on_child_added_f callback);
void wc_datasync_on_child_changed(wc_context_t *ctx, char *path, on_child_changed_f callback);
void wc_datasync_on_child_removed(wc_context_t *ctx, char *path, on_child_changed_f callback);

void wc_datasync_off_value(wc_context_t *ctx, char *path);
void wc_datasync_off_child_added(wc_context_t *ctx, char *path);
void wc_datasync_off_child_changed(wc_context_t *ctx, char *path);
void wc_datasync_off_child_removed(wc_context_t *ctx, char *path);


#endif /* LIB_DATASYNC_ON_ON_API_H_ */
