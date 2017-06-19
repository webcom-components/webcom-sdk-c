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
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_H_ */
