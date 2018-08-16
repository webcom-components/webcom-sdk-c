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

/**
 * @addtogroup webcom-on
 * @{
 * This set of functions is dedicated to registering callbacks on specific data
 * events occurring on the Webcom datasync tree:
 *
 * - a given path has changed,
 * - a child was added on a given path,
 * - a child was modified on a given path,
 * - a child was removed on a given path.
 *
 * The **wc_datasync_on_XXX()** functions all return an opaque handle that
 * represents the subscription. This handle can be used to cancel such a
 * subscription, or to get some informations associated to it such as the
 * data path it refers to, or the Webcom context it is associated to.
 */

/** Opaque type that represents a subscription */
typedef void *on_handle_t;

/**
 * Callback type for data events registered by the **wc_datasync_on_XXX()**
 * functions.
 * @param ctx the webcom context
 * @param handle the handle of the registration that triggered this callback
 * @param data a JSON string containing the current data at the registration's
 * path
 * @param current_key contains the name of the current key for a
 * **ON_CHILD_XXX** event, **NULL** for **ON_VALUE** events
 * @param previous_key contains the name of the previous sibling key for a
 * **ON_CHILD_XXX** event, **NULL** if it is the first sibling or for
 * **ON_VALUE** events
 * @return return 1 to keep this registration active, 0 to cancel it
 */
typedef int (*on_callback_f)(wc_context_t *ctx, on_handle_t handle, char * data, char *current_key, char *previous_key);

enum on_event_type {
	ON_CHILD_ADDED,
	ON_CHILD_REMOVED,
	ON_CHILD_CHANGED,
	ON_VALUE,
	/* update the ON_EVENT_TYPE_COUNT macro if you add new events */
};

/**
 * Registers a callback to monitor any change in the data under the given path.
 * When this method is called, the callback will be called once with the
 * current value of the database at the given path, and will be called again
 * each time a change occurs at this path.
 * @param ctx the Webcom context
 * @param path the path
 * @param callback the callback
 * @return the subscription handle
 */
on_handle_t wc_datasync_on_value(wc_context_t *ctx, char *path, on_callback_f callback);

/**
 * Registers a callback to monitor the apparition of a new key at the given
 * node (path).
 * When this method is called, the callback will be called once for every already
 * existing key at the given path, and will be called again each time a new
 * key is added at this path.
 * @param ctx the Webcom context
 * @param path the path
 * @param callback the callback
 * @return the subscription handle
 */
on_handle_t wc_datasync_on_child_added(wc_context_t *ctx, char *path, on_callback_f callback);

/**
 * Registers a callback to monitor the modification of a child element of the
 * given path.
 * The callback will be called each time a child node changes at the given
 * path, except when it's an addition or a deletion.
 * @param ctx the Webcom context
 * @param path the path
 * @param callback the callback
 * @return the subscription handle
 */
on_handle_t wc_datasync_on_child_changed(wc_context_t *ctx, char *path, on_callback_f callback);

/**
 * Registers a callback to monitor the deletion of a child element of the given
 * path.
 * The callback will be called each time a child node is removed at the given
 * path.
 * @param ctx the Webcom context
 * @param path the path
 * @param callback the callback
 * @return the subscription handle
 */
on_handle_t wc_datasync_on_child_removed(wc_context_t *ctx, char *path, on_callback_f callback);

/**
 * Gets the path associated to a subscription handle
 * @param h the handle
 * @return the path
 */
char *wc_datasync_on_handle_get_path(on_handle_t h);

/**
 * Gets the context associated to a subscription handle
 * @param h the handle
 * @return the context
 */
wc_context_t *wc_datasync_on_handle_get_ctx(on_handle_t h);

/**
 * Unsubscribes and deletes a subscription
 * @param h the subscription handle
 * @warning Do **not** use this function inside a **on_XXX** callback, it will
 * lead to undefined behavior and most likely a crash. Return 0 from the
 * callback if you wish to cancel this callback.
 */
void wc_datasync_off(on_handle_t h);

/**
 * Unsubscribes and deletes all subscriptions for a given path
 * @param ctx the webcom context
 * @param path the path
 * @warning Do **not** use this function inside a **on_XXX** callback, it will
 * lead to undefined behavior and most likely a crash.
 */
void wc_datasync_off_path(wc_context_t *ctx, char *path);

/**
 * Unsubscribes and deletes all subscriptions aof a given type for a given path
 * @param ctx the webcom context
 * @param path the path
 * @param type the type
 * @warning Do **not** use this function inside a **on_XXX** callback, it will
 * lead to undefined behavior and most likely a crash.
 */
void wc_datasync_off_path_type(wc_context_t *ctx, char *path, enum on_event_type type);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_ON_H_ */
