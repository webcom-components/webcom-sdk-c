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

#define LOCAL_LOG_FACILITY WC_LOG_AUTH

#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#include "webcom_priv.h"

#define WEBCOM_AUTH_API_PATH_BEGIN "auth/v2/"
#define WEBCOM_AUTH_API_PATH_END            "/password/signin"

/* libcurl init */
__attribute__((constructor))
static void libwebcom_curl_init() {
   curl_global_init(CURL_GLOBAL_DEFAULT);
}

static void _wc_parse_auth_json(json_object *root, struct wc_auth_info *pai) {
	json_object *tmp, *tmp2;

	/* is this an error response? */
	if (json_object_object_get_ex(root, "success", &tmp)
			&& json_object_is_type(tmp, json_type_boolean)
			&& json_object_get_boolean(tmp) == FALSE)
	{
		if (json_object_object_get_ex(root, "error", &tmp)
				&& json_object_is_type(tmp, json_type_object)
				&& json_object_object_get_ex(tmp, "message", &tmp)
				&& json_object_is_type(tmp, json_type_string))
		{
			pai->error = strdup(json_object_get_string(tmp));
		} else {
			pai->error = strdup("authentication failure");
		}
		WL_INFO("the auth server replied with an error: %s", pai->error);
		return;
	}

	/* is it a success response? */
	if (json_object_object_get_ex(root, "token", &tmp)
			&& json_object_is_type(tmp, json_type_string))
	{
		pai->token = strdup(json_object_get_string(tmp));

		if (json_object_object_get_ex(root, "user", &tmp)
				&& json_object_is_type(tmp, json_type_object))
		{
			if (json_object_object_get_ex(tmp, "expires", &tmp2)
					&& json_object_is_type(tmp2, json_type_int))
			{
				pai->expires = json_object_get_int64(tmp2);
			}

			if (json_object_object_get_ex(tmp, "providerUid", &tmp2)
					&& json_object_is_type(tmp2, json_type_string))
			{
				pai->provider_uid = strdup(json_object_get_string(tmp2));
			}

			if (json_object_object_get_ex(tmp, "uid", &tmp2)
					&& json_object_is_type(tmp2, json_type_string))
			{
				pai->uid = strdup(json_object_get_string(tmp2));
			}

			if (json_object_object_get_ex(tmp, "provider", &tmp2)
					&& json_object_is_type(tmp2, json_type_string))
			{
				pai->provider = strdup(json_object_get_string(tmp2));
			}

			if (json_object_object_get_ex(tmp, "providerProfile", &tmp2)
					&& json_object_is_type(tmp2, json_type_object))
			{
				pai->provider_profile = strdup(json_object_to_json_string_ext(tmp2, JSON_C_TO_STRING_PLAIN));
			}
		}
		WL_INFO("the auth server replied with success, token: %.7s...", pai->token);
		return;
	}

	/* then it's garbage */
	pai->error = strdup("invalid server response");
}

#define FREE_IF_NOT_NULL(p) do{if(p)free(p);}while(0)
static void _wc_free_auth_members(struct wc_auth_info *pai) {
	FREE_IF_NOT_NULL(pai->error);
	FREE_IF_NOT_NULL(pai->provider_uid);
	FREE_IF_NOT_NULL(pai->uid);
	FREE_IF_NOT_NULL(pai->token);
	FREE_IF_NOT_NULL(pai->provider);
	FREE_IF_NOT_NULL(pai->provider_profile);
}

static int _wc_parse_auth_body(wc_context_t *ctx, char *data, size_t len, struct wc_auth_info *pai) {
	enum json_tokener_error jte;
	json_object* jroot;

	if (ctx->auth_parser == NULL) {
		ctx->auth_parser = json_tokener_new();
	}

	jroot = json_tokener_parse_ex(ctx->auth_parser, data, len);
	jte = json_tokener_get_error(ctx->auth_parser);
	switch (jte) {
		case json_tokener_success:
			memset(pai, 0, sizeof(*pai));
			_wc_parse_auth_json(jroot, pai);
			json_tokener_free(ctx->auth_parser);
			json_object_put(jroot);
			return 1;
			break;
		case json_tokener_continue:
			return 0;
			break;
		default: /* json_tokener_error_* */
			pai->error = (char *)json_tokener_error_desc(jte);
			WL_ERR("malformed auth json response, %s", pai->error);
			return -1;
			break;
	}
}

static int curl_debug_cb(UNUSED_PARAM(CURL *handle),
                   curl_infotype type,
                   char *data,
                   size_t size,
				   UNUSED_PARAM(void *userptr))
{
	switch (type) {
	case CURLINFO_TEXT:
		WL_LOG(WC_LOG_DEBUG, "[cURL] %.*s", (int )size - 1, data);
		break;
	case CURLINFO_HEADER_IN:
		WL_LOG(WC_LOG_DEBUG, "[cURL, header in] %.*s", (int )size - 1, data);
		break;
	case CURLINFO_HEADER_OUT:
		WL_LOG(WC_LOG_DEBUG, "[cURL, header out] %.*s", (int )size - 1, data);
		break;
	case CURLINFO_DATA_IN:
	case CURLINFO_DATA_OUT:
	case CURLINFO_SSL_DATA_OUT:
	case CURLINFO_SSL_DATA_IN:
	default:
		break;
	}
	return 0;
}


static size_t curl_write_cb(void *ptr, size_t size, size_t nmemb, void *data) {
	WL_DBG("incoming data for auth request: %.*s", (int)(size * nmemb), (char*)ptr);
	struct wc_auth_info ai;
	wc_context_t *ctx = data;
	if (_wc_parse_auth_body(ctx, ptr, size * nmemb, &ai)) {
		ctx->callback(WC_AUTH_ON_AUTH_REPLY, ctx, &ai, sizeof(ai));
		_wc_free_auth_members(&ai);
	}
	return size * nmemb;
}

static int curl_sock_cb(CURL *e, curl_socket_t s, int what, void *data, void *sockp) {
	WL_DBG("socket handling request for %p: fd %d, events %d", e, s, what);
	struct wc_pollargs wcpa;
	wc_context_t *ctx = data;

	wcpa.src = WC_POLL_AUTH;
	wcpa.fd = s;

	wc_event_t event = ((intptr_t)sockp) ? WC_EVENT_MODIFY_FD : WC_EVENT_ADD_FD;

	curl_multi_assign(ctx->auth_curl_multi_handle, s, (void*)1);

	switch (what) {
	case CURL_POLL_IN:
		wcpa.events = POLLIN;
		ctx->callback(event, ctx, &wcpa, 0);
		break;
	case CURL_POLL_OUT:
		wcpa.events = POLLOUT;
		ctx->callback(event, ctx, &wcpa, 0);
		break;
	case CURL_POLL_INOUT:
		wcpa.events = POLLIN|POLLOUT;
		ctx->callback(event, ctx, &wcpa, 0);
		break;
	case CURL_POLL_REMOVE:
		ctx->callback(WC_EVENT_DEL_FD, ctx, &wcpa, 0);
		wcpa.events = 0;
		break;
	}

	return 0;
}

static int curl_timer_cb(CURLM *multi, long timeout_ms, void *userp) {
	WL_DBG("timer request on %p: %ld ms", multi, timeout_ms);
	int handles;
	struct wc_timerargs wcta;
	wc_context_t *ctx = userp;

	if (timeout_ms == 0) {
		curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, &handles);
	} else if (timeout_ms > 0) {
		wcta.ms = timeout_ms;
		wcta.repeat = 0;
		wcta.timer = WC_TIMER_AUTH;
		ctx->callback(WC_EVENT_SET_TIMER, ctx, &wcta, 0);
	} else if (timeout_ms == -1) {
		wcta.timer = WC_TIMER_AUTH;
		ctx->callback(WC_EVENT_DEL_TIMER, ctx, &wcta.timer, 0);
	}
	return 0;
}

int wc_auth_with_password(wc_context_t *ctx, const char *email, const char *password) {
	size_t api_path_l;
	char *urle_email, *urle_password;
	size_t form_data_l;

	if (ctx->auth_curl_handle != NULL) {
		return 0;
	}

	if (ctx->auth_parser != NULL) {
		json_tokener_free(ctx->auth_parser);
		ctx->auth_parser = NULL;
	}


	if (ctx->auth_curl_multi_handle == NULL) {
		ctx->auth_curl_multi_handle = curl_multi_init();

		curl_multi_setopt(ctx->auth_curl_multi_handle, CURLMOPT_SOCKETFUNCTION, curl_sock_cb);
		curl_multi_setopt(ctx->auth_curl_multi_handle, CURLMOPT_SOCKETDATA, ctx);
		curl_multi_setopt(ctx->auth_curl_multi_handle, CURLMOPT_TIMERFUNCTION, curl_timer_cb);
		curl_multi_setopt(ctx->auth_curl_multi_handle, CURLMOPT_TIMERDATA, ctx);
		api_path_l =
				9 /* 'https://' */
				+ strlen(ctx->host)
				+ 7 /* ':' + port + '/' */
				+ sizeof(WEBCOM_AUTH_API_PATH_BEGIN) - 1
				+ strlen(ctx->app_name) + sizeof(WEBCOM_AUTH_API_PATH_END) - 1
				+ 1;
		ctx->auth_url = malloc(api_path_l);
		if (ctx->auth_url == NULL) {
			return 0;
		}
		snprintf((char*) ctx->auth_url, api_path_l,
				"https://%s:%"PRIu16"/"WEBCOM_AUTH_API_PATH_BEGIN"%s"WEBCOM_AUTH_API_PATH_END,
				ctx->host,
				ctx->port,
				ctx->app_name);
	}

	ctx->auth_curl_handle = curl_easy_init();

	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_URL, ctx->auth_url);
	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_WRITEFUNCTION, curl_write_cb);
	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_WRITEDATA, ctx);
	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_DEBUGFUNCTION, curl_debug_cb);
	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_ERRORBUFFER, ctx->auth_error);
	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_PRIVATE, ctx);
	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_TIMEOUT, 10);

	urle_email = curl_easy_escape(ctx->auth_curl_handle, email, strlen(email));
	urle_password = curl_easy_escape(ctx->auth_curl_handle, password, strlen(password));
	form_data_l = snprintf(NULL, 0, "email=%s&password=%s", urle_email, urle_password);
	ctx->auth_form_data = malloc(form_data_l + 1);
	snprintf(ctx->auth_form_data, form_data_l + 1, "email=%s&password=%s", urle_email, urle_password);

	curl_easy_setopt(ctx->auth_curl_handle, CURLOPT_POSTFIELDS, ctx->auth_form_data);

	curl_multi_add_handle(ctx->auth_curl_multi_handle, ctx->auth_curl_handle);

	return 1;
}

void wc_auth_event_action(wc_context_t *ctx, int fd) {
	int curl_still_running;

	curl_multi_socket_action(ctx->auth_curl_multi_handle, fd, 0, &curl_still_running);
	if (curl_still_running == 0) {
		curl_multi_remove_handle(ctx->auth_curl_multi_handle, ctx->auth_curl_handle);
		curl_easy_cleanup(ctx->auth_curl_handle);
	}
}
