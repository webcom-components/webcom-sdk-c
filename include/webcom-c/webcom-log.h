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

/**
 * @addtogroup webcom-log
 * @{
 */

/**
 * The SDK defines several facilities ("channels") in which logs can be
 * written. This enum allows to specify to which facility a log message is
 * targeted.
 */
enum wc_log_facility {
	WC_LOG_WEBSOCKET,   //!< libwebsockets logs
	WC_LOG_PARSER,      //!< Webcom protocol (JSON-based) parser logs
	WC_LOG_CONNECTION,  //!< Webcom server connection logs
	WC_LOG_MESSAGE,     //!< Webcom messages activity logs
	WC_LOG_GENERAL,     //!< Other logs
	WC_LOG_APPLICATION, //!< Reserved for the application's logs

	WC_LOG_ALL          //!< Special value used to set a priority on all facilities
};

/**
 * This emum defines the different possible priorities for a log message. They
 * mimic those found in syslog.h, plus two other special values
 */
enum wc_log_level {
	WC_LOG_EMERG,     //!< see syslog.h's LOG_EMERG
	WC_LOG_ALERT,     //!< see syslog.h's LOG_ALERT
	WC_LOG_CRIT,      //!< see syslog.h's LOG_CRIT
	WC_LOG_ERR,       //!< see syslog.h's LOG_ERR
	WC_LOG_WARNING,   //!< see syslog.h's LOG_WARNING
	WC_LOG_NOTICE,    //!< see syslog.h's LOG_NOTICE
	WC_LOG_INFO,      //!< see syslog.h's LOG_INFO
	WC_LOG_DEBUG,     //!< see syslog.h's LOG_DEBUG
	WC_LOG_EXTRADEBUG,/**< this extra debug level is for those extremely
	                       verbose and frequent messages that tend to make the
	                       logs almost unreadable */
	WC_LOG_DISABLED,  /**< use this priority in  wc_set_log_level() to
	                       completely disable logging in a facility */
};

/**
 * Sets the log verbosity for a facility
 *
 * This function allows to filter the log messages being written to a facility
 * based on their priority (see `enum wc_log_level`).
 *
 * All the messages whose priority is higher or equal to the one given as
 * parameter are being logged, the other ones are being discarded. Using the
 * special priority WC_LOG_DISABLED will discard all the logs for the facility.
 *
 * **Example:**
 *
 * After this statement is executed
 * @code{c}
 * wc_set_log_level(WC_LOG_PARSER, WC_LOG_ERR);
 * @endcode
 * only the log messages with priority  **WC_LOG_EMERG**, **WC_LOG_ALERT**,
 * **WC_LOG_CRIT** and **WC_LOG_ERR** are going to show up in the logs for
 * the **WC_LOG_PARSER** facility.
 *
 * @param f the facility
 * @param l the log level (priority)
 */
void wc_set_log_level(enum wc_log_facility f, enum wc_log_level l);

#ifdef WITH_SYSLOG
#include <syslog.h> /* for options flags */

/**
 * Use syslog as the log backend
 *
 * After calling this functions, every log message produces by the SDK will be
 * sent to syslog. The parameters are the same as those from **openlog(3)**.
 *
 * @see Details for options flags and facilities in `man 3 openlog`.
 * @note If you switch from this backend to another backend, during the
 * lifetime of your application, you should call **closelog()** to close the
 * connection to syslog.
 *
 * @param ident _The string pointed to by ident is prepended to every message,
 * and is typically set to the program name.  If ident is NULL, the program
 * name is used._
 * @param option _The option argument specifies flags which control the
 * operation of openlog() and subsequent calls to syslog()._
 * @param facility _The facility argument establishes a default to be used if
 * none is specified in subsequent calls to syslog()._
 */
void wc_log_use_syslog(const char *ident, int option, int facility);
#endif

#ifdef WITH_JOURNALD
/**
 * Use journald as the log backend
 *
 * After calling this functions, every log message produces by the SDK will be
 * sent to journald using the native journald interface. The journal entries
 * will have some additional custom properties set, when the log macros are
 * being used, to help with filtering and diagnostic:
 *
 * - **CODE_FILE**: the C source file the log comes from
 * - **CODE_LINE**: the line in the source code
 * - **CODE_FUNC**: the function name
 * - **PRIORITY**: a number representing the log level (**WC_LOG_EMERG**=0,
 * **WC_LOG_EXTRADEBUG**=8)
 * - **WC_FACILITY**: the facility the log was sent to
 */
void wc_log_use_journald(void);
#endif

/**
 * Use stderror as the log backend (default)
 */
void wc_log_use_stderr(void);

/**
 * Send a formatted message to the log backend
 *
 * This function sends a message to the previously chosen log backend (default
 * stderror). It is not meant to be used directly, but through the various
 * **W_LOG()** macros and its derivatives, that automatically populates the
 * source file, line and function arguments.
 *
 * @param f the facility
 * @param l the log level
 * @param file the source code file name this log comes from
 * @param func the function name
 * @param line the line number in the source file
 * @param fmt a printf-like format string for the log message, the following
 * arguments are the format string arguments
 */
void wc_log(enum wc_log_facility f, enum wc_log_level l, const char *file, const char *func, int line, const char *fmt, ...)
	__attribute__ ((format (printf, 6, 7)));

/**
 * @}
 */

/**
 * @ingroup webcom-log
 * Logs a formatted message
 *
 * This macro sends a message to the previously chosen log backend, and
 * automatically sets the source file, line and function to the local
 * value.
 *
 * @remark several other macros are defined from this one to avoid specifying
 * the log level as an argument:
 * - `W_EXDBG(_facility, _fmt, ...)`
 * - `W_DBG(_facility, _fmt, ...)`
 * - `W_INFO(_facility, _fmt, ...)`
 * - `W_NOT(_facility, _fmt, ...)`
 * - `W_WARN(_facility, _fmt, ...)`
 * - `W_ERR(_facility, _fmt, ...)`
 * - `W_CRIT(_facility, _fmt, ...)`
 * - `W_ALRT(_facility, _fmt, ...)`
 * - `W_EMRG(_facility, _fmt, ...)`
 *
 * @param _facility the facility
 * @param _level the log level
 * @param _fmt a **literal** format string (a newline will automatically be
 * appended), the following arguments are the format string arguments
 */
#define W_LOG(_facility, _level, _fmt, ...) \
	wc_log((_facility), (_level), \
			__FILE__, __func__, __LINE__, \
			_fmt"\n", ## __VA_ARGS__)

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

/**
 * @ingroup webcom-log
 * Logs a formatted message in the local facility
 *
 * Helper macro to avoid manually mentioning the log facility in a compilation
 * unit, useful if almost every log message in this unit goes to the same
 * faciliy.
 *
 * You must define the **LOCAL_LOG_FACILITY** macro to the desired enum
 * wc_log_facility value **before** including webcom-log.h to set the local
 * facility, otherwise **WC_LOG_GENERAL** will be used.
 *
 * **Example:**
 * @code{c}
 * #define LOCAL_LOG_FACILITY WC_LOG_PARSER
 *
 * #include <webcom-c/webcom.h> // includes webcom-log.h
 *
 * int foo(void) {
 *     ...
 *     WL_LOG(WC_LOG_INFO, "Entering foo, %d", 42); // will be written in the WC_LOG_PARSER facility
 *     ...
 * }
 * @endcode
 *
 * @remark several other macros are defined from this one to avoid specifying
 * the log level as an argument:
 * - `WL_EXDBG(_fmt, ...)`
 * - `WL_DBG(_fmt, ...)`
 * - `WL_INFO(_fmt, ...)`
 * - `WL_NOT(_fmt, ...)`
 * - `WL_WARN(_fmt, ...)`
 * - `WL_ERR(_fmt, ...)`
 * - `WL_CRIT(_fmt, ...)`
 * - `WL_ALRT(_fmt, ...)`
 * - `WL_EMRG(_fmt, ...)`
 *
 * @param _level the log level
 *
 * @param _fmt a **literal** format string (a newline will automatically be
 * appended), the following arguments are the format string arguments
 */
#define WL_LOG(_level, _fmt, ...)	W_LOG(LOCAL_LOG_FACILITY, (_level), _fmt, ## __VA_ARGS__)

#define WL_EXDBG(_fmt, ...)	WL_LOG(WC_LOG_EXTRADEBUG, _fmt, ## __VA_ARGS__)
#define WL_DBG(  _fmt, ...)	WL_LOG(WC_LOG_DEBUG,      _fmt, ## __VA_ARGS__)
#define WL_INFO( _fmt, ...)	WL_LOG(WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define WL_NOT(  _fmt, ...)	WL_LOG(WC_LOG_NOTICE,     _fmt, ## __VA_ARGS__)
#define WL_WARN( _fmt, ...)	WL_LOG(WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define WL_ERR(  _fmt, ...)	WL_LOG(WC_LOG_ERR,        _fmt, ## __VA_ARGS__)
#define WL_CRIT( _fmt, ...)	WL_LOG(WC_LOG_CRIT,       _fmt, ## __VA_ARGS__)
#define WL_ALRT( _fmt, ...)	WL_LOG(WC_LOG_ALERT,      _fmt, ## __VA_ARGS__)
#define WL_EMRG( _fmt, ...)	WL_LOG(WC_LOG_EMERG,      _fmt, ## __VA_ARGS__)

/**
 * @ingroup webcom-log
 * Logs a formatted message in the application's facility
 *
 * This macro should be used by the application code using the SDK. All
 * messages written with this macro will be routed to the
 * **WC_LOG_APPLICATION** facility.
 *
 * @remark several other macros are defined from this one to avoid specifying
 * the log level as an argument:
 * - `APP_EXDBG(_fmt, ...)`
 * - `APP_DBG(_fmt, ...)`
 * - `APP_INFO(_fmt, ...)`
 * - `APP_NOT(_fmt, ...)`
 * - `APP_WARN(_fmt, ...)`
 * - `APP_ERR(_fmt, ...)`
 * - `APP_CRIT(_fmt, ...)`
 * - `APP_ALRT(_fmt, ...)`
 * - `APP_EMRG(_fmt, ...)`
 *
 * @param _level the log level
 * @param _fmt a **literal** format string (a newline will automatically be
 * appended), the following arguments are the format string arguments
 */
#define APP_LOG(_level, _fmt, ...) W_LOG(WC_LOG_APPLICATION, (_level), _fmt, ## __VA_ARGS__)

#define APP_EXDBG(_fmt, ...)	APP_LOG(WC_LOG_EXTRADEBUG, _fmt, ## __VA_ARGS__)
#define APP_DBG(  _fmt, ...)	APP_LOG(WC_LOG_DEBUG,      _fmt, ## __VA_ARGS__)
#define APP_INFO( _fmt, ...)	APP_LOG(WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define APP_NOT(  _fmt, ...)	APP_LOG(WC_LOG_NOTICE,     _fmt, ## __VA_ARGS__)
#define APP_WARN( _fmt, ...)	APP_LOG(WC_LOG_INFO,       _fmt, ## __VA_ARGS__)
#define APP_ERR(  _fmt, ...)	APP_LOG(WC_LOG_ERR,        _fmt, ## __VA_ARGS__)
#define APP_CRIT( _fmt, ...)	APP_LOG(WC_LOG_CRIT,       _fmt, ## __VA_ARGS__)
#define APP_ALRT( _fmt, ...)	APP_LOG(WC_LOG_ALERT,      _fmt, ## __VA_ARGS__)
#define APP_EMRG( _fmt, ...)	APP_LOG(WC_LOG_EMERG,      _fmt, ## __VA_ARGS__)

#endif /* INCLUDE_WEBCOM_C_WEBCOM_LOG_H_ */
