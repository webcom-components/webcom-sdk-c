/*
 * Webcom C SDK
 *
 * Copyright 2017 Orange
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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_EVENT_H_
#define INCLUDE_WEBCOM_C_WEBCOM_EVENT_H_

#include "webcom-cnx.h"

/**
 * @ingroup webcom-event
 * @{
 */

typedef enum {
	WC_ON_DATA_PUT, /**< callback for a update put event */
	WC_ON_DATA_MERGE /**< callback for a update merge event */
} ws_on_data_event_t;
/**
 * callback for data update events
 *
 * @param cnx the webcom connection
 * @param event the event type (either WC_ON_DATA_PUT or WC_ON_DATA_MERGE)
 * @param path the path of the data event
 * @param json_data a string containing the json encoded data of the event
 * @param param the optional user data passed to wc_on_data()
 */
typedef void (*wc_on_data_callback_t)(wc_cnx_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param);

/**
 * registers a callback for data update
 *
 * This function will register a function to be called whenever the server
 * sends a **WC_PUSH_DATA_UPDATE_PUT** or **WC_PUSH_DATA_UPDATE_MERGE** message
 * on a given connection, and its path is a superset of the given path
 * parameter. E.g.: if `"/foo/bar/"` is provided in the path parameter, the
 * given callback will be called for data updates on `"/foo/bar/qux/"`,
 * `"/foo/bar/"`, `"/foo/bar/quux/corge/"`, ..., but not `"/foo/baz/"`.
 *
 * @remark If a data event path matches several paths registered through this
 * function, they will all be called, in the shortest to the longest path
 * order.
 *
 * @param cnx the webcom connection
 * @param path the "root" path of the events to register
 * @param callback the callback to call if an event matches
 * @param user some optional user data pointer that will be passed to the
 * callback
 */
void wc_on_data(wc_cnx_t *cnx, char *path, wc_on_data_callback_t callback, void *user);


/**
 * unregister callbacks for data update
 *
 * This function will unregister a given callback that was previously
 * registered with wc_on_data() for a given root path, or all the callbacks
 * for the given path.
 * @param cnx the webcom connection
 * @param path the path
 * @param callback the callback to unregister; pass NULL to unregister all the
 * callbacks for the given path
 */
void wc_off_data(wc_cnx_t *cnx, char *path, wc_on_data_callback_t callback);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_EVENT_H_ */
