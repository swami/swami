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
#ifndef __IPATCH_UTIL_H__
#define __IPATCH_UTIL_H__

#include <glib.h>
#include <glib-object.h>

extern GValue *ipatch_util_value_bool_true;
extern GValue *ipatch_util_value_bool_false;

/* a pointer to a constant boolean GValue for TRUE or FALSE depending on input
 * value. */
#define IPATCH_UTIL_VALUE_BOOL(b) \
  ((b) ? ipatch_util_value_bool_true : ipatch_util_value_bool_false)

guint ipatch_util_value_hash (GValue *val);
guint ipatch_util_value_array_hash (GValueArray *valarray);
guint64 ipatch_util_file_size (const char *fname, GError **err);
char *ipatch_util_abs_filename (const char *filename);
void ipatch_util_weakref_destroy (gpointer value);

#endif
