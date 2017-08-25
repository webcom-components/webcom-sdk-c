/*
 * Webcom C SDK
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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_UTILS_H_
#define INCLUDE_WEBCOM_C_WEBCOM_UTILS_H_

/**
 * @ingroup webcom-utils
 * @{
 */

/**
 * marks a function parameter unused, and avoids compiler warnings
 * @param expr the parameter definition
 *
 * **Example:**
 *
 * the compiler won't complain about y not being used:
 *
 *			int square(int x, UNUSED_PARAM(int y)) {
 *				return x * x;
 *			}
 */
#define UNUSED_PARAM(expr) expr __attribute__ ((unused))

/**
 * marks a variable unused, and avoids compiler warnings
 * @param name the variable name
 *
 * **Example:**
 *
 * the compiler won't complain about y not being used:
 *
 *			int square(int x) {
 *				int y;
 *				UNUSED_VAR(y);
 *				return x * x;
 *			}
 */
#define UNUSED_VAR(name) (void)(name)

#define WC_INLINE __attribute__((always_inline)) inline

/**
 * @}
 */
#endif /* INCLUDE_WEBCOM_C_WEBCOM_UTILS_H_ */
