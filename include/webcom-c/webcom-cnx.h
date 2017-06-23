#ifndef INCLUDE_WEBCOM_C_WEBCOM_CNX_H_
#define INCLUDE_WEBCOM_C_WEBCOM_CNX_H_

#include <stddef.h>

#include "webcom-msg.h"

typedef struct wc_cnx wc_cnx_t;

/**
 * @defgroup connection Webcom client to server connection management
 * @{
 */

typedef enum {
	/**
	 * This event indicates that the connection passed as cnx to the callback
	 * was closed, either because the server has closed it, or because
	 * wc_cnx_close() was called.
	 */
	WC_EVENT_ON_CNX_CLOSED,
	/**
	 * This event indicates that the websocket connection to the webcom server,
	 * passed as cnx to the callback, was successfully established.
	 *
	 * Note: this does not mean the server handshake was received yet.
	 */
	WC_EVENT_ON_CNX_ESTABLISHED,
	/**
	 * This event indicates that the webcom server has sent its handshake
	 * message. From this point, it's safe to start sending requests to the
	 * webcom server through this connection.
	 *
	 * Note: this does not mean the server handshake was received yet.
	 */
	WC_EVENT_ON_SERVER_HANDSHAKE,
	/**
	 * This event indicates that a valid webcom message was received on the
	 * given connection. The parsed message pointer (wc_msg_t *) is passed as
	 * (void *)data to the callback.
	 */
	WC_EVENT_ON_MSG_RECEIVED,
} wc_event_t;

/**
 * This defines the callback type that must be passed to wc_cnx_new() or
 * wc_cnx_new_with_proxy(). It will be called for the events listed in the
 * wc_event_t enum. When triggered, the callback will receive these parameters:
 *
 * @param event (mandatory) the event type that was triggered
 * @param cnx (mandatory) the connection on which the event occurred
 * @param data (optional) some additional data about the event
 * @param len (optional) the size of the additional data
 * @param user (optional) some user-defined data, set in wc_cnx_new() or
 * wc_cnx_new_with_proxy()
 */
typedef void (*wc_on_event_cb_t) (wc_event_t event, wc_cnx_t *cnx, void *data, size_t len, void *user);

/**
 * Establishes a direct connection to a webcom server.
 *
 * This function establishes a new direct connection (no proxy) to a webcom
 * server host.
 *
 * Note: this function is synchronous, it will return once the connection has
 * been established or once it failed to be established.
 *
 * @param host the webcom server host name
 *
 * @param port the webcom server port
 * @param application the name of the webcom application to tie to (e.g.
 * "legorange", "chat", ...)
 * @param callback the user-defined callback that will be called by the SDK
 * when one of the events listed in the wc_event_t enum occurs
 * @param user some optional user-data that will be passed to the callback for
 * every event occurring on this connection
 * @return a pointer to the newly created connection on success, NULL on
 * failure
 */
wc_cnx_t *wc_cnx_new(char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user);

/**
 * Establishes a connection to a webcom server through a HTTP proxy.
 *
 * Notes:
 *
 * - the only supported proxy method is CONNECT over unencrypted HTTP (the
 * webcom connection will then be TLS encrypted in this tunnel)
 * - this function is synchronous, it will return once the connection has
 * been established or once it failed to be established.
 *
 * @param proxy_host the HTTP proxy server host name
 * @param proxy_port the HTTP proxy server port
 * @param host the webcom server host name
 * @param port the webcom server port
 * @param application the name of the webcom application to tie to (e.g.
 * "legorange", "chat", ...)
 * @param callback the user-defined callback that will be called by the SDK
 * when one of the events listed in the wc_event_t enum occurs
 * @param user some optional user-data that will be passed to the callback for
 * every event occurring on this connection
 * @return a pointer to the newly created connection on success, NULL on
 * failure
 */
wc_cnx_t *wc_cnx_new_with_proxy(char *proxy_host, uint16_t proxy_port, char *host, uint16_t port, char *application, wc_on_event_cb_t callback, void *user);

/**
 * Frees the resources used by a wc_cnx_t object.
 *
 * This function is to be called once a wc_cnx_t object becomes useless.
 *
 * @param cnx the wc_cnx_t object to free.
 */
void wc_cnx_free(wc_cnx_t *cnx);

/**
 * Sends a message to the webcom server.
 *
 * @param cnx the connection to the server
 * @param msg the webcom message to send
 * @return the number of bytes written, otherwise < 0 is returned in case of
 * failure.
 */
int wc_cnx_send_msg(wc_cnx_t *cnx, wc_msg_t *msg);

/**
 * Gets the system-level file descriptor of a successfully established
 * connection.
 *
 * This is useful to integrate the SDK in some pre-existing or user-defined
 * event-loop (epoll, poll, ..., or abstractions such as libevent, libev, ...).
 *
 * @param cnx the connection
 * @return the file descriptor
 */
int wc_cnx_get_fd(wc_cnx_t *cnx);

/**
 * This function allows the webcom SDK to process the incoming data on a webcom
 * connection.
 *
 * This function must be called either frequently and periodically (not
 * ideal), or systematically when there is data to read on the file descriptor
 * obtained with wc_cnx_get_fd().
 *
 * If there is a webcom-level event related to the read and processed data,
 * calling this function will automatically trigger call(s) to the user-defined
 * wc_on_event_cb_t callback.
 *
 * @param cnx the connection whose incoming data should be processed
 * @return (not used)
 */
int wc_cnx_on_readable(wc_cnx_t *cnx);

/**
 * Gracefully closes a connection to a webcom server.
 *
 * Note: the user-defined wc_on_event_cb_t callback will be triggered with a
 * WC_EVENT_ON_CNX_CLOSED event.
 *
 * @param cnx tke connection
 */
void wc_cnx_close(wc_cnx_t *cnx);

/**
 * Function that keeps the connection to the webcom server alive.
 *
 * The server will singlehandledly close the connection if there is no activity
 * from the client in the last 60 seconds. Call this function periodically to
 * avoid this.
 *
 * @param cnx the connection
 */
int wc_cnx_keepalive(wc_cnx_t *cnx);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_CNX_H_ */
