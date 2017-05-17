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

int wc_push_json_data(wc_cnx_t *cnx, char *path, char *json);

#endif /* INCLUDE_WEBCOM_C_WEBCOM_H_ */
