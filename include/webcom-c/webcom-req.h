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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_REQ_H_
#define INCLUDE_WEBCOM_C_WEBCOM_REQ_H_

#include "webcom-datasync.h"
#include "webcom-msg.h"

typedef enum {WC_REQ_OK, WC_REQ_ERROR} wc_req_pending_result_t;

/**
 * @ingroup webcom-requests
 * @{
 */

/**
 * callback type for request status notification
 *
 * @param cnx the webcomm connection
 * @param id the request id
 * @param type what kind of action (wc_action_type_t) is this notification
 * about
 * @param status did the request succeed (WC_REQ_OK) or failed (WC_REQ_ERROR)
 * @param reason the success or error reason string
 * @param data some optional JSON data from the server
 */
typedef void (*wc_on_req_result_t) (wc_context_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason, char *data, void *user);

/**
 * sends a data put request to the webcom server and get notified of the status
 *
 * This function builds and sends a data put request to the webcom server.
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param path a string representing the path of the data
 * @param json a string containing the JSON-encoded data to store on the server
 * 	             at the given path
 * @param user a custom pointer that will be passed to the callback
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_put(wc_context_t *cnx, char *path, char *json, wc_on_req_result_t callback, void *user);

/**
 * sends a data merge request to the webcom server and get notified of the
 * status
 *
 * This function builds and sends a data merge request to the webcom server.
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param path a string representing the path of the data
 * @param json a string containing the JSON-encoded data to merge on the server
 * 	             at the given path
 * @param user a custom pointer that will be passed to the callback
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_merge(wc_context_t *cnx, char *path, char *json, wc_on_req_result_t callback, void *user);

/**
 * sends a data push request to the webcom server and get notified of the status
 *
 * This function builds and sends a data push request to the webcom server. It
 * slightly differs from the put request: in the push case, the SDK adds a
 * unique, time-ordered, partly random key to the path and sends a put request
 * to this new path.
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param path a string representing the path of the data
 * @param json a string containing the JSON-encoded data to push
 * @param user a custom pointer that will be passed to the callback
 *
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_push(wc_context_t *cnx, char *path, char *json, wc_on_req_result_t callback, void *user);

/**
 * sends a listen request to the webcom server and get notified of the status
 *
 * This function builds and sends a listen request to the webcom server.
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param path a string representing the path to listen to
 * @param user a custom pointer that will be passed to the callback
 *
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_listen(wc_context_t *cnx, char *path, wc_on_req_result_t callback, void *user);

/**
 * sends a un-listen request to the webcom server and get notified of the status
 *
 * This function builds and sends a un-listen request to the webcom server, to
 * stop listening a given path.
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param path a string representing the path
 * @param user a custom pointer that will be passed to the callback
 *
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_unlisten(wc_context_t *cnx, char *path, wc_on_req_result_t callback, void *user);

/**
 * sends an authentication request to the webcom server and get notified of the
 * status
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param cred a string containing the secret token
 * @param user a custom pointer that will be passed to the callback
 *
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 *
 * @see wc_auth_with_password()
 */
int64_t wc_datasync_auth(wc_context_t *cnx, char *cred, wc_on_req_result_t callback, void *user);

/**
 * sends a un-authentication request to the webcom server and get notified of
 * the status
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param user a custom pointer that will be passed to the callback
 *
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_unauth(wc_context_t *cnx, wc_on_req_result_t callback, void *user);

/**
 * sends an on-disconnect-put request to the webcom server and get notified of
 * the status
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param path a string representing the path of the data
 * @param json a string containing the JSON-encoded data to store on the server
 * 	             at the given path on disconnection
 * @param user a custom pointer that will be passed to the callback
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_on_disc_put(wc_context_t *cnx, char *path, char *json, wc_on_req_result_t callback, void *user);

/**
 * sends an on-disconnect-merge request to the webcom server and get notified of
 * the status
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param path a string representing the path of the data
 * @param json a string containing the JSON-encoded data to merge on the server
 * 	             at the given path on disconnection
 * @param user a custom pointer that will be passed to the callback
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_on_disc_merge(wc_context_t *cnx, char *path, char *json, wc_on_req_result_t callback, void *user);

/**
 * sends an on-disconnect-cancel request to the webcom server and get notified
 * of the status
 *
 * @param cnx the webcom connection
 * @param callback callback that will be called when the status of the request
 * is sent back from the server
 * @param path a string representing the path of the data
 * @param user a custom pointer that will be passed to the callback
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_datasync_on_disc_cancel(wc_context_t *cnx, char *path, wc_on_req_result_t callback, void *user);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_REQ_H_ */
