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

#ifndef INCLUDE_WEBCOM_C_WEBCOM_LOG_H_
#define INCLUDE_WEBCOM_C_WEBCOM_LOG_H_

#include "webcom-config.h"

#ifdef WITH_SYSLOG
#	include <syslog.h>
#endif

enum wc_log_facility {
	WC_LOG_WEBSOCKET,
	WC_LOG_PARSER,
	WC_LOG_CONNECTION,
	WC_LOG_MESSAGE,
	WC_LOG_GENERAL,
	WC_LOG_APPLICATION,

	WC_LOG_ALL /* must be the last one */
};

enum wc_log_level {
	WC_LOG_EMERG,
	WC_LOG_ALERT,
	WC_LOG_CRIT,
	WC_LOG_ERR,
	WC_LOG_WARNING,
	WC_LOG_NOTICE,
	WC_LOG_INFO,
	WC_LOG_DEBUG,
	WC_LOG_EXTRADEBUG,
	WC_LOG_DISABLED,
};

void wc_set_log_level(enum wc_log_facility f, enum wc_log_level l);
void wc_log_use_syslog(const char *ident, int option, int facility);
void wc_log_use_journald(void);
void wc_log_use_stderr(void);
void wc_log(enum wc_log_facility f, enum wc_log_level l, const char *file, const char *func, int line, const char *fmt, ...);

extern enum wc_log_level wc_log_levels[WC_LOG_ALL];

#define W_LOG(_facility, _level, _fmt, ...) \
	wc_log((_facility), (_level), \
			__FILE__, __func__, __LINE__, \
			_fmt"\n", ## __VA_ARGS__); \

#define W_EXDBG(_facility, _fmt, ...)	W_LOG((_facility), WC_LOG_EXTRADEBUG, _fmt, ## __VA_ARGS__)
#define W_DBG(  _facility, _fmt, ...)	W_LOG((_facility), WC_LOG_DEBUG,      _fmt, ## __VA_ARGS__)
#define W_INFO( _facility, _fmt, ...)	W_LOG((_facility), WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define W_NOT(  _facility, _fmt, ...)	W_LOG((_facility), WC_LOG_NOTICE,     _fmt, ## __VA_ARGS__)
#define W_WARN( _facility, _fmt, ...)	W_LOG((_facility), WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define W_ERR(  _facility, _fmt, ...)	W_LOG((_facility), WC_LOG_ERR,        _fmt, ## __VA_ARGS__)
#define W_CRIT( _facility, _fmt, ...)	W_LOG((_facility), WC_LOG_CRIT,       _fmt, ## __VA_ARGS__)
#define W_ALRT( _facility, _fmt, ...)	W_LOG((_facility), WC_LOG_ALERT,      _fmt, ## __VA_ARGS__)
#define W_EMRG( _facility, _fmt, ...)	W_LOG((_facility), WC_LOG_EMERG,      _fmt, ## __VA_ARGS__)


#ifndef LOCAL_LOG_FACILITY
#	define LOCAL_LOG_FACILITY WC_LOG_GENERAL
#endif

#define WL_EXDBG(_fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_EXTRADEBUG, _fmt, ## __VA_ARGS__)
#define WL_DBG(  _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_DEBUG,      _fmt, ## __VA_ARGS__)
#define WL_INFO( _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define WL_NOT(  _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_NOTICE,     _fmt, ## __VA_ARGS__)
#define WL_WARN( _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define WL_ERR(  _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_ERR,        _fmt, ## __VA_ARGS__)
#define WL_CRIT( _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_CRIT,       _fmt, ## __VA_ARGS__)
#define WL_ALRT( _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_ALERT,      _fmt, ## __VA_ARGS__)
#define WL_EMRG( _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, WC_LOG_EMERG,      _fmt, ## __VA_ARGS__)


#define APP_LOG(_level, _fmt, ...) W_LOG(WC_LOG_APPLICATION, (_level), _fmt, ## __VA_ARGS__)

#define APP_EXDBG(_fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_EXTRADEBUG, _fmt, ## __VA_ARGS__)
#define APP_DBG(  _fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_DEBUG,      _fmt, ## __VA_ARGS__)
#define APP_INFO( _fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define APP_NOT(  _fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_NOTICE,     _fmt, ## __VA_ARGS__)
#define APP_WARN( _fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define APP_ERR(  _fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_ERR,        _fmt, ## __VA_ARGS__)
#define APP_CRIT( _fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_CRIT,       _fmt, ## __VA_ARGS__)
#define APP_ALRT( _fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_ALERT,      _fmt, ## __VA_ARGS__)
#define APP_EMRG( _fmt, ...)	W_LOG(WC_LOG_APPLICATION, WC_LOG_EMERG,      _fmt, ## __VA_ARGS__)

#endif /* INCLUDE_WEBCOM_C_WEBCOM_LOG_H_ */
