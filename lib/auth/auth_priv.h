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

#ifndef LIB_AUTH_AUTH_PRIV_H_
#define LIB_AUTH_AUTH_PRIV_H_

#include <curl/curl.h>
#include <json-c/json.h>

struct wc_auth_context {
	struct wc_context *webcom;
	char *auth_url;
	CURL *auth_curl_handle;
	CURLM *auth_curl_multi_handle;
	json_tokener *auth_parser;
	char auth_error[CURL_ERROR_SIZE];
	char *auth_form_data;
};

#endif /* LIB_AUTH_AUTH_PRIV_H_ */
