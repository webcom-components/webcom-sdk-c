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

static enum wc_log_backend {
	be_stderr,
	be_custom,
#ifdef WITH_SYSLOG
	be_syslog,
#endif
#ifdef WITH_JOURNALD
	be_journald,
#endif
} backend = be_stderr;

static wc_log_f _log_custom;

char *wc_log_facility_names[] = {
	"LWS",
	"PRS",
	"CNX",
	"MSG",
	"GEN",
	"APP",
	"AUT",
};

char *wc_log_level_names[] = {
	" EMERG",
	" ALERT",
	"CRITIC",
	" ERROR",
	"WARNIN",
	"NOTICE",
	"  INFO",
	" DEBUG",
	"EXTDBG",
};

static inline const char *_basename(const char *p) {
	const char *ret = p;
	while (*p) {
		if (*p++ == '/') ret = p;
	}
	return ret;
}

static void _log_stderr(enum wc_log_facility f, enum wc_log_level l, const char *file, int line,  const char *message) {
	fprintf(stderr, "[%s] (%s:%d) %s: %s", wc_log_level_names[l], _basename(file), line, wc_log_facility_names[f], message);
}

static enum wc_log_level wc_log_levels[WC_LOG_ALL] = {
	[WC_LOG_WEBSOCKET]   = WC_LOG_ERR,
	[WC_LOG_PARSER]      = WC_LOG_ERR,
	[WC_LOG_CONNECTION]  = WC_LOG_ERR,
	[WC_LOG_MESSAGE]     = WC_LOG_ERR,
	[WC_LOG_GENERAL]     = WC_LOG_ERR,
	[WC_LOG_APPLICATION] = WC_LOG_WARNING,
};


void wc_lws_log_adapter(int level, const char *line) {
	enum wc_log_level l = WC_LOG_EXTRADEBUG;

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

	wc_log(WC_LOG_WEBSOCKET, l, "libwebsockets.so", "N/A", 0, "%s", line);
}

__attribute__ ((visibility ("hidden")))
void wc_init_log(void) {
	wc_set_log_level(WC_LOG_WEBSOCKET, wc_log_levels[WC_LOG_WEBSOCKET]);
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
				/* FALLTHRU */
			case WC_LOG_DEBUG:
				lws_level |= LLL_DEBUG | LLL_CLIENT;
				/* FALLTHRU */
			case WC_LOG_INFO:
				lws_level |= LLL_INFO;
				/* FALLTHRU */
			case WC_LOG_NOTICE:
				lws_level |= LLL_NOTICE;
				/* FALLTHRU */
			case WC_LOG_WARNING:
				lws_level |= LLL_WARN;
				/* FALLTHRU */
			case WC_LOG_ERR:
				lws_level |= LLL_ERR;
				/* FALLTHRU */
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
	backend = be_stderr;
}

void wc_log_use_custom(wc_log_f f) {
	backend = be_custom;
	_log_custom = f;
}

#ifdef WITH_SYSLOG
#include <syslog.h>
static void _log_syslog(enum wc_log_facility f, enum wc_log_level l, const char *file, int line,  const char *message) {
	syslog(l == WC_LOG_EXTRADEBUG ? WC_LOG_DEBUG : l,
			"(%s:%d) %s: %s", _basename(file), line, wc_log_facility_names[f], message);
}

void wc_log_use_syslog(const char *ident, int option, int facility) {
	openlog(ident, option, facility);
	backend = be_syslog;
}
#endif /* WITH_SYSLOG */

#ifdef WITH_JOURNALD
#define SD_JOURNAL_SUPPRESS_LOCATION
#include <systemd/sd-journal.h>
static void _log_journald(enum wc_log_facility f, enum wc_log_level l, const char *file, const char *func, int line,  const char *message) {
	sd_journal_send(
			"CODE_FILE=%s", file,
			"CODE_LINE=%d", line,
			"CODE_FUNC=%s", func,
			"MESSAGE=%s: %s", wc_log_facility_names[f], message,
			"PRIORITY=%d", l,
			"WC_FACILITY=%s", wc_log_facility_names[f],
			NULL);
}

void wc_log_use_journald(void) {
	backend = be_journald;
}
#endif /* WITH_JOURNALD */

void wc_log(enum wc_log_facility f, enum wc_log_level l, const char *file, const char *func, int line, const char *fmt, ...) {
	va_list args, _args;

	/* discard messages with too low priority */
	if (l > wc_log_levels[f]) return;

	va_start(args, fmt);
	va_copy(_args, args);
	int msg_len = vsnprintf(NULL, 0, fmt, args);
	char msg[msg_len + 1];
	vsnprintf(msg, msg_len + 1, fmt, _args);
	va_end(_args);
	va_end(args);

	switch (backend) {
	case be_stderr:
		_log_stderr(f, l, file, line, msg);
		break;
	case be_custom:
		_log_custom(f, l, file, func, line, msg);
		break;
#ifdef WITH_SYSLOG
	case be_syslog:
		_log_syslog(f, l, file, line, msg);
		break;
#endif
#ifdef WITH_JOURNALD
	case be_journald:
		_log_journald(f, l, file, func, line, msg);
		break;
#else
		UNUSED_VAR(func);
#endif
	}
}
