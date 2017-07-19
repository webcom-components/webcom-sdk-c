#include <string.h>

#include "webcom-c/webcom.h"
#include "webcom_priv.h"

void wc_on_data(wc_cnx_t *cnx, char *path, wc_on_data_callback_t callback, void *user) {
	wc_on_data_handler_t *new_h;

	new_h = malloc(sizeof(wc_on_data_handler_t));

	new_h->path = strdup(path);
	new_h->path_len = strlen(path);
	new_h->callback = callback;
	new_h->user = user;
	new_h->next = cnx->handlers;

	cnx->handlers = new_h;
}

void wc_on_data_dispatch(wc_cnx_t *cnx, wc_push_t *push) {
	wc_on_data_handler_t *p;
	char *push_path;
	char *push_data;

	if (push->type == WC_PUSH_DATA_UPDATE_PUT) {
		push_path = push->u.update_put.path;
		push_data = push->u.update_put.data;
	} else if(push->type == WC_PUSH_DATA_UPDATE_MERGE) {
		push_path = push->u.update_merge.path;
		push_data = push->u.update_merge.data;
	} else {
		return;
	}


	for (p = cnx->handlers ; p ; p = p->next) {
		if (strncmp(push_path, p->path, p->path_len) == 0) {
			p->callback(cnx,
					push->type == WC_PUSH_DATA_UPDATE_PUT,
					push_path,
					push_data,
					p->user);
			break;
		}
	}
}
