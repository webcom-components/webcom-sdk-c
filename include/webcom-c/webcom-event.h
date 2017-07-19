#ifndef INCLUDE_WEBCOM_C_WEBCOM_EVENT_H_
#define INCLUDE_WEBCOM_C_WEBCOM_EVENT_H_

#include "webcom-cnx.h"

/**
 * @defgroup event XXX
 * @{
 */

typedef void (*wc_on_data_callback_t)(wc_cnx_t *cnx, int put_or_merge, char *path, char *json_data, void *param);

void wc_on_data(wc_cnx_t *cnx, char *path, wc_on_data_callback_t callback, void *user);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_EVENT_H_ */
