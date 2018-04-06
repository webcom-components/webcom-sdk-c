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

#include <stdio.h>

int json_escaped_str_len(char *str) {
	int c;
	int ret = 0;
	while ((c = *str++)) {
		switch (c) {
		case '"':
		case '\\':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
			ret += 2; /* '\_' */
			break;
		default:
			if ('\x00' <= c && c <= '\x1f') {
				ret +=  7; /* '\u____x' */
			} else {
				ret +=1;
			}
		}
	}
	ret +=2; /* enclosing double quotes */
	return ret;
}

int json_escape_str(char *raw, char *escaped) {
	int c;
	char *p = escaped;
	*p++ = '"';

	while((c = *raw++)) {
		switch (c) {
		case '"':
			*p++ = '\\';
			*p++ = '"';
			break;
		case '\\':
			*p++ = '\\';
			*p++ = '\\';
			break;
		case '\b':
			*p++ = '\\';
			*p++ = 'b';
			break;
		case '\f':
			*p++ = '\\';
			*p++ = 'f';
			break;
		case '\n':
			*p++ = '\\';
			*p++ = 'n';
			break;
		case '\r':
			*p++ = '\\';
			*p++ = 'r';
			break;
		case '\t':
			*p++ = '\\';
			*p++ = 't';
			break;
		default:
			if ('\x00' <= c && c <= '\x1f') {
				p += sprintf(p, "\\u%04x", c);
			} else {
				*p++ = c;
			}
		}
	}

	*p++ = '"';

	return p - escaped;
}
