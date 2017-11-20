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

#include <stdarg.h>
#include <stdio.h>
#include <libwebsockets.h>
#include <string.h>

#include "webcom-c/webcom.h"

char *wc_log_facility_names[] = {
	"websocket",
	"parser",
	"connection",
	"message",
	"general",
	"application",
};

char *wc_log_level_names[] = {
	"EMERG",
	"ALERT",
	"CRIT",
	"ERR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG",
	"EXTRADEBUG",
};

static void _log_stderr(enum wc_log_facility f, enum wc_log_level l, const char *fmt, va_list args) {
	fprintf(stderr, "[%s] %s: ", wc_log_level_names[l], wc_log_facility_names[f]);
	vfprintf(stderr, fmt, args);
}

static void (*log_f) (enum wc_log_facility f, enum wc_log_level l, const char *fmt, va_list args) = _log_stderr;

enum wc_log_level wc_log_levels[WC_LOG_ALL] = {
	[WC_LOG_WEBSOCKET]   = WC_LOG_ERR,
	[WC_LOG_PARSER]      = WC_LOG_ERR,
	[WC_LOG_CONNECTION]  = WC_LOG_ERR,
	[WC_LOG_MESSAGE]     = WC_LOG_ERR,
	[WC_LOG_GENERAL]     = WC_LOG_ERR,
	[WC_LOG_APPLICATION] = WC_LOG_WARNING,
};

void wc_lws_log_adapter(int level, const char *line) {
	enum wc_log_level l;

	switch(level) {
	case LLL_ERR:
		l = WC_LOG_ERR;
		break;
	case LLL_WARN:
		l = WC_LOG_WARNING;
		break;
	case LLL_NOTICE:
		l = WC_LOG_NOTICE;
		break;
	case LLL_INFO:
		l = WC_LOG_INFO;
		break;
	case LLL_DEBUG:
	case LLL_CLIENT:
		l = WC_LOG_DEBUG;
		break;
	case LLL_PARSER:
	case LLL_HEADER:
	case LLL_EXT:
	case LLL_LATENCY:
		l = WC_LOG_EXTRADEBUG;
		break;
	}

	wc_log(WC_LOG_WEBSOCKET, l, "%s", line);
}

void wc_set_log_level(enum wc_log_facility f, enum wc_log_level l) {
	if (f == WC_LOG_ALL) {
		int i = 0;
		for (i = 0 ; i < WC_LOG_ALL ; i++) {
			wc_set_log_level(i, l);
		}
	} else {
		if (f == WC_LOG_WEBSOCKET) {
			int lws_level = 0;
			switch (l) {
			case WC_LOG_EXTRADEBUG:
				lws_level |= LLL_EXT | LLL_PARSER | LLL_LATENCY;
				/* no break */
			case WC_LOG_DEBUG:
				lws_level |= LLL_DEBUG | LLL_CLIENT;
				/* no break */
			case WC_LOG_INFO:
				lws_level |= LLL_INFO;
				/* no break */
			case WC_LOG_NOTICE:
				lws_level |= LLL_NOTICE;
				/* no break */
			case WC_LOG_WARNING:
				lws_level |= LLL_WARN;
				/* no break */
			case WC_LOG_ERR:
				lws_level |= LLL_ERR;
				/* no break */
			case WC_LOG_CRIT:
			case WC_LOG_ALERT:
			case WC_LOG_EMERG:
			case WC_LOG_DISABLED:
				break;
			}
			lws_set_log_level(lws_level, wc_lws_log_adapter);
		}
		wc_log_levels[f] = l;
	}
}

void wc_log_use_stderr(void) {
	wc_set_log_level(WC_LOG_WEBSOCKET, wc_log_levels[WC_LOG_WEBSOCKET]);
}

#ifdef WITH_SYSLOG
#include <syslog.h>
static void _log_syslog(enum wc_log_facility f, enum wc_log_level l, const char *fmt, va_list args) {
	va_list _args;
	va_copy(_args, args);
	int msg_len = vsnprintf(NULL, 0, fmt, args);
	char msg[msg_len + 1];
	vsnprintf(msg, msg_len + 1, fmt, _args);
	va_end(_args);
	syslog(l == WC_LOG_EXTRADEBUG ? WC_LOG_DEBUG : l,
			"%s: %s", wc_log_facility_names[f], msg);
}

void wc_log_use_syslog(const char *ident, int option, int facility) {
	openlog(ident, option, facility);
	log_f = _log_syslog;
	wc_set_log_level(WC_LOG_WEBSOCKET, wc_log_levels[WC_LOG_WEBSOCKET]);
}
#endif /* WITH_SYSLOG */

#ifdef WITH_JOURNALD
#include <systemd/sd-journal.h>
static void _log_journald(enum wc_log_facility f, enum wc_log_level l, const char *fmt, va_list args) {
	va_list _args;
	va_copy(_args, args);
	int msg_len = vsnprintf(NULL, 0, fmt, args);
	char msg[msg_len + 1];
	vsnprintf(msg, msg_len + 1, fmt, _args);
	va_end(_args);

	sd_journal_send(
			"MESSAGE=%s: %s", wc_log_facility_names[f], msg,
			"PRIORITY=%d", l,
			"WC_FACILITY=%s", wc_log_facility_names[f],
			NULL);
}

void wc_log_use_journald(void) {
	log_f = _log_journald;
	wc_set_log_level(WC_LOG_WEBSOCKET, wc_log_levels[WC_LOG_WEBSOCKET]);
}
#endif /* WITH_JOURNALD */

inline void wc_log(enum wc_log_facility f, enum wc_log_level l, const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	log_f(f, l, fmt, args);
	va_end(args);
}
