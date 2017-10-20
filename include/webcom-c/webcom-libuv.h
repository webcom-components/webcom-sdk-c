/*
 * webcom-sdk-c
 *
 * Copyright 2017 Orange
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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_LIBUV_H_
#define INCLUDE_WEBCOM_C_WEBCOM_LIBUV_H_

#include "webcom-config.h"

#ifdef WITH_LIBUV

#include "webcom-cnx.h"

#include <uv.h>

struct wc_libuv_integration {
	uv_loop_t *loop;
	void (*on_connected)(wc_context_t *ctx, int initial_connection);
	void (*on_disconnected)(wc_context_t *ctx);
};

wc_context_t *wc_context_new_with_libuv(char *host, uint16_t port, char *application, struct wc_libuv_integration *eli);

#endif /* WITH_LIBUV */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_LIBUV_H_ */
