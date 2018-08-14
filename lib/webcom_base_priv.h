/*
 * webcom-sdk-c
 *
 * Copyright 2018 Orange
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

#ifndef LIB_WEBCOM_BASE_PRIV_H_
#define LIB_WEBCOM_BASE_PRIV_H_

#include "compat.h"

#include <stdint.h>

#include "webcom-c/webcom.h"

#include "auth/auth_priv.h"
#include "datasync/datasync_priv.h"


wc_datasync_context_t *wc_get_datasync(wc_context_t *);
wc_auth_context_t *wc_get_auth(wc_context_t *);

struct wc_context {
	wc_on_event_cb_t callback;
	void *user;
	char *app_name;
	char *host;
	uint16_t port;
	struct wc_datasync_context datasync;
	struct wc_auth_context auth;
	int no_tls:1;
	int datasync_init:1;
	int auth_init:1;
};

__attribute__ ((visibility ("hidden")))
void wc_init_log(void) ;

#endif /* LIB_WEBCOM_BASE_PRIV_H_ */
