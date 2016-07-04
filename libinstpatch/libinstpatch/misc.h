/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
#ifndef __MISC_H__
#define __MISC_H__

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>

/**
 * IPATCH_ERROR: (skip)
 *
 * libInstPatch domain for g_set_error
 */
#define IPATCH_ERROR  ipatch_error_quark()

typedef enum
{
  IPATCH_ERROR_FAIL,		/* a general failure */
  IPATCH_ERROR_IO,		/* I/O error (file operations, etc) */
  IPATCH_ERROR_PROGRAM,		/* a programming error */
  IPATCH_ERROR_INVALID,		/* invalid parameter or data */
  IPATCH_ERROR_CORRUPT,		/* corrupted data */
  IPATCH_ERROR_NOMEM,		/* out of memory error */
  IPATCH_ERROR_UNSUPPORTED,	/* unsupported feature */
  IPATCH_ERROR_UNEXPECTED_EOF,	/* unexpected end of file */
  IPATCH_ERROR_UNHANDLED_CONVERSION,	/* unhandled object conversion */
  IPATCH_ERROR_BUSY		/* a resource is busy (still open, etc) - Since: 1.1.0 */
} IpatchError;

#ifdef G_HAVE_ISO_VARARGS
#define ipatch_code_error(...)  \
  _ipatch_code_error (__FILE__, __LINE__, G_GNUC_PRETTY_FUNCTION, __VA_ARGS__)
#elif defined(G_HAVE_GNUC_VARARGS)
#define ipatch_code_error(err, format...) \
  _ipatch_code_error (__FILE__, __LINE__, G_GNUC_PRETTY_FUNCTION, err, format)
#else  /* no varargs macros */
static void
ipatch_code_error (GError **err, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  _ipatch_code_errorv (NULL, -1, NULL, err, format, args);
  va_end (args);
}
#endif

extern char *ipatch_application_name;

void ipatch_init (void);
void ipatch_close (void);
void ipatch_set_application_name (const char *name);
GQuark ipatch_error_quark (void);
G_CONST_RETURN char *ipatch_gerror_message (GError *err);
void _ipatch_code_error (const char *file, guint line, const char *func,
			 GError **err, const char *format, ...);
void _ipatch_code_errorv (const char *file, guint line, const char *func,
			  GError **err, const char *format, va_list args);
void ipatch_strconcat_num (const char *src, int num, char *dest, int size);
void ipatch_dump_object (GObject *object, gboolean recursive, FILE *file);
void ipatch_glist_unref_free (GList *objlist);

#endif
