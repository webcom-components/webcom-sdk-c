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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_ON_H_
#define INCLUDE_WEBCOM_C_WEBCOM_ON_H_

#include "webcom-base.h"

typedef void (*on_callback_f)(wc_context_t *ctx, char * data, char *current_key, char *previous_key);

#define ON_EVENT_TYPE_COUNT (4)
enum on_event_type {
	ON_CHILD_ADDED,
	ON_CHILD_REMOVED,
	ON_CHILD_CHANGED,
	ON_VALUE,
};

void wc_datasync_on_value(wc_context_t *ctx, char *path, on_callback_f callback);
void wc_datasync_on_child_added(wc_context_t *ctx, char *path, on_callback_f callback);
void wc_datasync_on_child_changed(wc_context_t *ctx, char *path, on_callback_f callback);
void wc_datasync_on_child_removed(wc_context_t *ctx, char *path, on_callback_f callback);

void wc_datasync_off_path(wc_context_t *ctx, char *path);
void wc_datasync_off_path_type(wc_context_t *ctx, char *path, enum on_event_type type);
void wc_datasync_off_path_type_cb(wc_context_t *ctx, char *path, enum on_event_type type, on_callback_f cb);

#endif /* INCLUDE_WEBCOM_C_WEBCOM_ON_H_ */