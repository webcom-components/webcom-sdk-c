#ifndef INCLUDE_WEBCOM_C_WEBCOM_REQ_H_
#define INCLUDE_WEBCOM_C_WEBCOM_REQ_H_

#include "webcom-cnx.h"
#include "webcom-msg.h"

typedef enum {WC_REQ_OK, WC_REQ_ERROR} wc_req_pending_result_t;

typedef void (*wc_on_req_result_t) (wc_cnx_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason);

int64_t wc_req_put(wc_cnx_t *cnx, char *path, char *json, wc_on_req_result_t callback);

#endif /* INCLUDE_WEBCOM_C_WEBCOM_REQ_H_ */
