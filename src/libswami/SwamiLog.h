/*
 * SwamiLog.h - Message logging and debugging functions
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */

#ifndef __SWAMI_LOG_H__
#define __SWAMI_LOG_H__

#include <glib.h>

/* Swami domain for g_set_error */
#define SWAMI_ERROR  swami_error_quark()

typedef enum
{
    SWAMI_ERROR_FAIL,		/* general failure */
    SWAMI_ERROR_INVALID,		/* invalid parameter/setting/etc */
    SWAMI_ERROR_CANCELED,		/* an operation was canceled (SwamiLoopFinder) */
    SWAMI_ERROR_UNSUPPORTED,	/* an unsupported feature or unhandled operation */
    SWAMI_ERROR_IO		/* I/O related error */
} SwamiError;

GQuark swami_error_quark(void);


#ifdef __GNUC__

#define swami_log_if_fail(expr)  (!(expr) && \
      _swami_ret_g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, \
	     "file %s: line %d (%s): assertion `%s' failed.", \
	     __FILE__, __LINE__, __PRETTY_FUNCTION__, \
	     #expr))

#else  /* !GNUC */

#define swami_log_if_fail(expr)  (!(expr) && \
      _swami_ret_g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, \
	     "file %s: line %d: assertion `%s' failed.", \
	     __FILE__, __LINE__, \
	     #expr))
#endif

int _swami_ret_g_log(const gchar *log_domain, GLogLevelFlags log_level,
                     const gchar *format, ...);

#ifndef _WIN32
extern void _swami_pretty_log_handler(GLogLevelFlags flags,
                                      char *file, char *function, int line,
                                      char *format, ...);

#ifdef SWAMI_DEBUG_ENABLED
#define SWAMI_DEBUG(format, args...) \
  _swami_pretty_log_handler (G_LOG_LEVEL_DEBUG, \
	 	 	     __FILE__, __PRETTY_FUNCTION__, __LINE__, \
	 	 	     format, ## args);
#else
#define SWAMI_DEBUG(format, args...)
#endif

#define SWAMI_INFO(format, args...) \
  _swami_pretty_log_handler (G_LOG_LEVEL_INFO, \
	 	 	     __FILE__, __PRETTY_FUNCTION__, __LINE__, \
	 	 	     format, ## args);

#define SWAMI_PARAM_ERROR(param) \
  _swami_pretty_log_handler (G_LOG_LEVEL_CRITICAL, \
	 	 	     __FILE__, __PRETTY_FUNCTION__, __LINE__, \
	 	 	     "Invalid function parameter value for '%s'.", \
	 	 	     param);

#define SWAMI_CRITICAL(format, args...) \
  _swami_pretty_log_handler (G_LOG_LEVEL_CRITICAL, \
	 	 	     __FILE__, __PRETTY_FUNCTION__, __LINE__, \
	 	 	     format, ## args);

#endif
#endif /* __SWAMI_LOG_H__ */
