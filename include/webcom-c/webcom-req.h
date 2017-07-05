#ifndef INCLUDE_WEBCOM_C_WEBCOM_REQ_H_
#define INCLUDE_WEBCOM_C_WEBCOM_REQ_H_

#include "webcom-cnx.h"
#include "webcom-msg.h"

typedef enum {WC_REQ_OK, WC_REQ_ERROR} wc_req_pending_result_t;

typedef void (*wc_on_req_result_t) (wc_cnx_t *cnx, int64_t id, wc_action_type_t type, wc_req_pending_result_t status, char *reason);

#define DECLARE_REQ_FUNC(__name, ... /* args */)	\
	int64_t wc_req_ ## __name(wc_cnx_t *cnx, wc_on_req_result_t callback, __VA_ARGS__);


DECLARE_REQ_FUNC(auth, char *cred)
DECLARE_REQ_FUNC(unauth, ...)
DECLARE_REQ_FUNC(put, char *path, char *json)
DECLARE_REQ_FUNC(merge, char *path, char *json)
DECLARE_REQ_FUNC(push, char *path, char *json)
DECLARE_REQ_FUNC(listen, char *path)
DECLARE_REQ_FUNC(unlisten, char *path)

#endif /* INCLUDE_WEBCOM_C_WEBCOM_REQ_H_ */
