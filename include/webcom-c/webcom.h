/**
 * @mainpage Webcom C SDK Index Page
 *
 * This SDK offers an easy access to the Orange Flexible Datasync service
 * to C/C++ programmer.
 */

#ifndef INCLUDE_WEBCOM_C_WEBCOM_H_
#define INCLUDE_WEBCOM_C_WEBCOM_H_

#include "webcom-msg.h"
#include "webcom-parser.h"
#include "webcom-cnx.h"
#include "webcom-utils.h"

/**
 * @defgroup webcom Webcom "high-level" functions
 * @{
 */

/**
 * sends a data put request to the webcom server
 *
 * This function builds and sends a data put request to the webcom server.
 *
 * @param cnx the webcom connection
 * @param path a string representing the path of the data
 * @param json a string containing the JSON-encoded data to store on the server
 * 	             at the given path
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 *
 */
int64_t wc_put_json_data(wc_cnx_t *cnx, char *path, char *json);

/**
 * sends a data push request to the webcom server
 *
 * This function builds and sends a data push request to the webcom server. It
 * slightly differs from the put request: in the push case, the SDK adds a
 * unique, time-ordered, partly random key to the path and sends a put request
 * to this new path.
 *
 * @param cnx the webcom connection
 * @param path a string representing the path of the data
 * @param json a string containing the JSON-encoded data to push
 *
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_push_json_data(wc_cnx_t *cnx, char *path, char *json);

/**
 * sends a listen request to the webcom server
 *
 * This function builds and sends a listen request to the webcom server.
 *
 * @param cnx the webcom connection
 * @param path a string representing the path to listen to
 *
 * @return the put request id (>0) if it was sent successfully, -1 otherwise
 */
int64_t wc_listen(wc_cnx_t *cnx, char *path);


/**
 * gets the server time in milliseconds since 1970/1/1
 *
 * This function returns the (estimated) time on the server, in milliseconds.
 * It is achieved by the SDK by computing and memorizing the clock offset
 * between the local machine and the server when establishing the connection to
 * the Webcom server.
 *
 * @param cnx the webcom connection
 * @return the estimated server time in milliseconds since 1970/1/1
 */
int64_t wc_get_server_time(wc_cnx_t *cnx);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_H_ */
