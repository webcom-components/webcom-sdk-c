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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_AUTH_H_
#define INCLUDE_WEBCOM_C_WEBCOM_AUTH_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @addtogroup webcom-auth
 * @{
 */

/**
 * Informations about an authentication response.
 *
 * If the authentication is successful, **error** will be NULL, otherwise it
 * points to a string describing the error.
 */
struct wc_auth_info {
	char *token; /**< the webcom token */
	uint64_t expires; /**< the token expiration timestamp */
	char *provider_uid; /**< the User ID according to the provider */
	char *uid; /**< the User ID according to the Webcom platform */
	char *error; /**< not NULL f an error occurred */
	char *provider; /**< the name of the provider */
	char *provider_profile;	/**< some optional JSON string containing profile
							     data from the provider, NULL if none */
};

/**
 * Request an authentication token with email/password
 *
 * Requests a token to the webcom server of the given context with the plain
 * email/password method.
 *
 * This function is asynchronous and returns immediately. Once the reply from
 * the server is received, the main callback of the context will be called with
 * the **WC_AUTH_ON_AUTH_REPLY** event, and a pointer to a **struct
 * wc_auth_info** object as data.
 *
 * @param ctx the webcom context
 * @param email the email
 * @param password the password
 * @return 1 on success, 0 on failure (e.g. out of memory)
 */
int wc_auth_with_password(wc_context_t *ctx, const char *email, const char *password);

/**
 * @}
 */

#endif /* INCLUDE_WEBCOM_C_WEBCOM_AUTH_H_ */
