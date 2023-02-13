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

#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "webcom-c/webcom-base.h"

#include "webcom_base_priv.h"

wc_context_t *wc_context_create(struct wc_context_options *options) {
	static int curl_initialized = 0;
	static int log_initialized = 0;

	wc_context_t *ret = NULL;

	if (log_initialized == 0) {
		wc_init_log();
		log_initialized = 1;
	}

	if (curl_initialized == 0) {
		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl_initialized = 1;
	}

	ret = calloc(1, sizeof (*ret));

	if (ret != NULL) {
		ret->app_name = strdup(options->app_name);
		ret->host = strdup(options->host);
		ret->port = options->port;
		ret->user = options->user_data;
		ret->callback = options->callback;
		ret->no_tls = !!options->no_tls;
	}

	return ret;
}

void wc_dispatch_fd_event(wc_context_t *ctx, struct wc_pollargs *pa) {
	if (pa->src == WC_POLL_DATASYNC) {
		wc_datasync_service_socket(ctx, pa);
	} else if (pa->src == WC_POLL_AUTH) {
		wc_auth_service(ctx, pa->fd);
	}
}

int wc_dispatch_timer_event(wc_context_t *ctx, enum wc_timersrc timer) {
	int release_timer = 0;
	
	switch (timer) {
	case WC_TIMER_DATASYNC_KEEPALIVE:
		wc_datasync_keepalive(ctx);
		break;
	case WC_TIMER_DATASYNC_RECONNECT:
		_wc_datasync_connect(ctx);
		break;
	case WC_TIMER_AUTH:
		release_timer = wc_auth_service(ctx, CURL_SOCKET_TIMEOUT);
		break;
	default:
		break;
	}
	
	return release_timer;
}


wc_datasync_context_t *wc_get_datasync(wc_context_t *ctx) {
	wc_datasync_context_t *ret = NULL;
	if (ctx->datasync_init) {
		ret = &ctx->datasync;
	}
	return ret;
}

wc_auth_context_t *wc_get_auth(wc_context_t *ctx) {
	wc_auth_context_t *ret = NULL;
	if (ctx->auth_init) {
		ret = &ctx->auth;
	}
	return ret;
}

void wc_context_destroy(wc_context_t * ctx) {
	if (ctx->datasync_init) {
		wc_datasync_context_cleanup(&ctx->datasync);
	}

	if (ctx->auth_init) {
		// TODO call destructor
	}

	free(ctx->app_name);
	free(ctx->host);

	free(ctx);
}

void *wc_context_get_user_data(wc_context_t *ctx) {
	return ctx->user;
}

const char* wc_version() {
	return WEBCOM_SDK_VERSION_STR;
}

