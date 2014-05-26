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
/*
 * ipatch_priv.h - Private header file
 */
#ifndef __IPATCH_PRIV_H__
#define __IPATCH_PRIV_H__

#include <glib.h>
#include "IpatchItem.h"
#include "marshals.h"
#include "misc.h"
#include "i18n.h"

#define IPATCH_UNTITLED		_("Untitled")

/* macro for getting a GParamSpec property ID (FIXME - its a private field!) */
#define IPATCH_PARAM_SPEC_ID(pspec)    ((pspec)->param_id)

/* size of buffers used for transfering sample data (in bytes)
   Must be a multiple of 16 bytes */
#define IPATCH_SAMPLE_COPY_BUFFER_SIZE  (32 * 1024)

/* Size of transform buffers used by IpatchSampleTransform objects in pool */
#define IPATCH_SAMPLE_TRANS_BUFFER_SIZE  (32 * 1024)

/* size of buffers used for copying data */
#define IPATCH_COPY_BUFFER_SIZE (32 * 1024)

/* So we can start using the glib 2.10 allocator now */
#ifndef g_slice_new
#define g_slice_new(type)        g_new (type, 1)
#define g_slice_new0(type)       g_new0 (type, 1)
#define g_slice_free(type, mem)  g_free (mem)
#define g_slice_alloc(size)      g_malloc(size)
#define g_slice_free1(size, mem) g_free (mem)
#endif

/* can be used in place of g_assert_not_reached, for g_return(_val)_if_fail
   to print error and return instead of terminating program */
#define NOT_REACHED 0

#ifdef __GNUC__

#define log_if_fail(expr)  (!(expr) && \
      _ret_g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, \
	     "file %s: line %d (%s): assertion `%s' failed.", \
	     __FILE__, __LINE__, __PRETTY_FUNCTION__, \
	     #expr))

#else  /* !GNUC */

#define log_if_fail(expr)  (!(expr) && \
      _ret_g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, \
	     "file %s: line %d: assertion `%s' failed.", \
	     __FILE__, __LINE__, \
	     #expr))
#endif


int _ret_g_log (const gchar *log_domain, GLogLevelFlags log_level,
		const gchar *format, ...);

#endif
