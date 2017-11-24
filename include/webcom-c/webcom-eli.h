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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_ELI_H_
#define INCLUDE_WEBCOM_C_WEBCOM_ELI_H_

#include "webcom-cnx.h"

struct wc_eli_callbacks {
	/** callback when the connection to the server is up */
	void (*on_connected)(wc_context_t *ctx);

	/** callback when the connection to the server is closed
	 * @param ctx the context
	 * @param error NULL if no error occurred, an error string on error
	 * @return return anything but 0 to let the SDK reconnect automatically
	 */
	int (*on_disconnected)(wc_context_t *ctx);

	/** callback when the connection to the server is close on error
	 * @param ctx the context
	 * @param next_try delay, in seconds, before the next automatic
	 * reconnection attempt, 0 if no next attempt
	 * @param a string describing the error (may not be null terminated)
	 * @param error_len the error string length
	 * @return return anything but 0 to let the SDK reconnect automatically
	 */
	int (*on_error)(wc_context_t *ctx, unsigned next_try, const char *error, int error_len);
};

#endif /* INCLUDE_WEBCOM_C_WEBCOM_ELI_H_ */
