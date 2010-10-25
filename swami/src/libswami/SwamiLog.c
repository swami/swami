/*
 * SwamiLog.c - Message logging and debugging functions
 *
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
 *
 * To contact the author of this program:
 * Email: Josh Green <jgreen@users.sourceforge.net>
 * Swami homepage: http://swami.sourceforge.net
 */

#include <stdio.h>
#include <glib.h>
#include <stdarg.h>

GQuark
swami_error_quark (void)
{
  static GQuark q = 0;

  if (q == 0)
    q = g_quark_from_static_string ("libswami-error-quark");

  return (q);
}

int
_swami_ret_g_log (const gchar *log_domain, GLogLevelFlags log_level,
		  const gchar *format, ...)
{
  va_list args;
  va_start (args, format);
  g_logv (log_domain, log_level, format, args);
  va_end (args);

  return (TRUE);
}

void _swami_pretty_log_handler (GLogLevelFlags level,
				char *file, char *function, int line,
				char *format, ...)
{
  va_list args;
  char *s, *s2;

  va_start (args, format);
  s = g_strdup_vprintf (format, args);
  va_end (args);

  s2 = g_strdup_printf ("file %s: line %d (%s): %s", file, line, function, s);
  g_free (s);

  g_log (NULL, level, "%s", s2);
  g_free (s2);
}
