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
/**
 * SECTION: util
 * @short_description: Utility functions
 * @see_also: 
 * @stability: Stable
 */
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <errno.h>
#include "util.h"
#include "misc.h"
#include "compat.h"
#include "i18n.h"

/* Convenience boolean GValue constants */
GValue *ipatch_util_value_bool_true;
GValue *ipatch_util_value_bool_false;

/**
 * _ipatch_util_init: (skip)
 */
void
_ipatch_util_init (void)
{
  ipatch_util_value_bool_true = g_new0 (GValue, 1);
  ipatch_util_value_bool_false = g_new0 (GValue, 1);

  g_value_init (ipatch_util_value_bool_true, G_TYPE_BOOLEAN);
  g_value_init (ipatch_util_value_bool_false, G_TYPE_BOOLEAN);

  g_value_set_boolean (ipatch_util_value_bool_true, TRUE);
  g_value_set_boolean (ipatch_util_value_bool_false, FALSE);
}

/**
 * ipatch_util_value_hash:
 * @val: GValue to hash
 *
 * Hash a GValue.  The hash value can then be used in a GHashTable for example.
 *
 * Returns: Hash value corresponding to the @val key.
 */
guint
ipatch_util_value_hash (GValue *val)
{
  GValueArray *valarray;
  GType valtype;
  const char *sval;

  /* so we can access a float value as its raw data without gcc bitching */
  union
    {
      gfloat f;
      guint32 i;
    } fval;

  g_return_val_if_fail (G_IS_VALUE (val), 0);

  valtype = G_VALUE_TYPE (val);

  switch (G_TYPE_FUNDAMENTAL (valtype))
    {
    case G_TYPE_CHAR:
      return (g_value_get_char (val));
    case G_TYPE_UCHAR:
      return (g_value_get_uchar (val));
    case G_TYPE_BOOLEAN:
      return (g_value_get_boolean (val));
    case G_TYPE_INT:
      return (g_value_get_int (val));
    case G_TYPE_UINT:
      return (g_value_get_uint (val));
    case G_TYPE_LONG:
      return (g_value_get_long (val));
    case G_TYPE_ULONG:
      return (g_value_get_ulong (val));
    case G_TYPE_INT64:
      return (g_value_get_int64 (val));
    case G_TYPE_UINT64:
      return (g_value_get_uint64 (val));
    case G_TYPE_ENUM:
      return (g_value_get_enum (val));
    case G_TYPE_FLAGS:
      return (g_value_get_flags (val));
    case G_TYPE_FLOAT:
      fval.f = g_value_get_float (val);
      return (fval.i);	/* use the raw float data as hash */
    case G_TYPE_DOUBLE:
      fval.f = g_value_get_double (val);	/* convert double to float */
      return (fval.i);		/* use the raw float data as hash */
    case G_TYPE_STRING:
      sval = g_value_get_string (val);
      return (sval ? g_str_hash (sval) : 0);
    case G_TYPE_POINTER:
      return (GPOINTER_TO_UINT (g_value_get_pointer (val)));
    case G_TYPE_BOXED:
      if (valtype == G_TYPE_VALUE_ARRAY)	/* value array? */
	{
	  valarray = g_value_get_boxed (val);
	  return (ipatch_util_value_array_hash (valarray));
	}
      else return (GPOINTER_TO_UINT (g_value_get_boxed (val)));
    case G_TYPE_PARAM:
      return (GPOINTER_TO_UINT (g_value_get_param (val)));
    case G_TYPE_OBJECT:
      return (GPOINTER_TO_UINT (g_value_get_object (val)));
    default:
      g_assert_not_reached ();
      return (0);	/* to keep compiler happy */
    }
}

/**
 * ipatch_util_value_array_hash:
 * @valarray: GValueArray to hash
 *
 * Hash a GValueArray.  The hash value can then be used in a GHashTable for
 * example.
 *
 * Returns: Hash value corresponding to the sum of all values returned
 * by ipatch_util_value_hash() for each GValue in the array.
 */
guint
ipatch_util_value_array_hash (GValueArray *valarray)
{
  GValue *value;
  guint hashval = 0;
  int i;

  if (!valarray) return (0);

  for (i = 0; i < valarray->n_values; i++)
    {
      value = g_value_array_get_nth (valarray, i);
      hashval += ipatch_util_value_hash (value);
    }

  return (hashval);
}

/**
 * ipatch_util_file_size: (skip)
 * @fname: Path of file to get size of.
 * @err: Location to store error or %NULL
 *
 * Get the size of a file (tired of using stat every time?).
 *
 * Returns: File size.  Will return 0 on error, but @err must be checked if
 *   it is set to determine if an error really occurred.
 */
guint64
ipatch_util_file_size (const char *fname, GError **err)
{
  struct stat st;

  g_return_val_if_fail (fname != NULL, 0);
  g_return_val_if_fail (!err || !*err, 0);

  if (g_stat (fname, &st) != 0)
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
		   _("Error stating file '%s': %s"), fname, g_strerror (errno));
      return (0);
    }

  return (st.st_size);
}

/**
 * ipatch_util_abs_filename: (skip)
 * @filename: File name to make absolute
 *
 * Make a file name absolute, if it isn't already.
 *
 * Returns: Newly allocated filename, converted to an absolute filename (if necessary)
 *   or NULL if @filename was NULL
 *
 * Since: 1.1.0
 */
char *
ipatch_util_abs_filename (const char *filename)
{
  char *dirname, *abs_filename;

  if (!filename) return (NULL);

  if (g_path_is_absolute (filename))
    return (g_strdup (filename));       // !! caller takes over allocation

  dirname = g_get_current_dir ();       // ++ alloc
  abs_filename = g_build_filename (dirname, filename, NULL);
  g_free (dirname);                     // -- free

  return (abs_filename);                // !! caller takes over allocation
}

/**
 * ipatch_util_weakref_destroy: (skip)
 * @value: Slice allocated GWeakRef to destroy
 *
 * A GDestroyNotify function for freeing a slice allocated
 * GWeakRef.
 *
 * Since: 1.1.0
 */
void
ipatch_util_weakref_destroy (gpointer value)
{
  GWeakRef *weakref = value;
  g_weak_ref_clear (weakref);
  g_slice_free (GWeakRef, weakref);
}

