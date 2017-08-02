/**
 * @mainpage Webcom C SDK Index Page
 *
 * This SDK offers an easy access to the Orange Flexible Datasync service
 * to C/C++ programmer.
 */

/**
 * @defgroup hilev		Higher level functions
 * @{
 * 	@defgroup webcom		Webcom general functions
 * 	@defgroup connection 	Connect a client to a Webcom server
 * 	@defgroup requests		Send Webcom client to server request
 *	@defgroup event			Register callback on Webcom events
 * @}
 * @defgroup lolev		Lower level functions (you may not need them)
 * @{
 * 	@defgroup messages	Webcom protocol data types, messages manipulation
 * 	@defgroup parser 	Webcom protocol parsing functions
 * 	@defgroup utils 	Some utility macros, functions, ...
 * @}
 */

#ifndef INCLUDE_WEBCOM_C_WEBCOM_H_
#define INCLUDE_WEBCOM_C_WEBCOM_H_

#include "webcom-msg.h"
#include "webcom-parser.h"
#include "webcom-cnx.h"
#include "webcom-req.h"
#include "webcom-utils.h"
#include "webcom-event.h"

/**
 * @ingroup webcom
 * @{
 */

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
 * builds a webcom push id
 *
 * This function will write a new push id in the buffer pointed by `result`.
 *
 * A push id is a 20-bytes string that guarantees several properties:
 * - all SDKs across all platform generate it using the exact same method
 * - when sorted in lexicographical order, they are also sorted by
 *   chronological order of their creation, regardless of what webcom client
 *   node has generated it (the SDKs does so by computing a local to server
 *   clock offset during the webcom protocolar handshake)
 *
 * **Example:**
 *
 *			foo(wc_cnx_t *cnx) {
 *				char buf[20];
 *				...
 *				wc_get_push_id(cnx, buf);
 *				printf("the id is: %20s\n", buf);
 *				...
 *			}
 *
 * @note the result buffer **will not** be nul-terminated
 *
 * @param cnx the webcom connection (it **MUST** have received the handshake
 * from the server, and it may be currently connected or disconnected)
 * @param result the address of a (minimum) 20-bytes buffer that will receive
 * the newly created push id
 */
void wc_get_push_id(wc_cnx_t *cnx, char *result);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_H_ */
