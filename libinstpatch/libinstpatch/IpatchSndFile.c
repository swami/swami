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
 * SECTION: IpatchSndFile
 * @short_description: libsndfile file object
 * @see_also: 
 * @stability: Stable
 *
 * Object type for libsndfile audio file identification.
 */
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include <sndfile.h>

#include "IpatchSndFile.h"
#include "sample.h"
#include "ipatch_priv.h"
#include "i18n.h"
#include "misc.h"

static gboolean ipatch_snd_file_identify (IpatchFile *file, IpatchFileHandle *handle,
                                          GError **err);

G_DEFINE_TYPE (IpatchSndFile, ipatch_snd_file, IPATCH_TYPE_FILE);


/* Get type of dynamic libsndfile file format enum (register it as needed) */
GType
ipatch_snd_file_format_get_type (void)
{
  static GType type = 0;

  if (!type)
  {
    GEnumValue *values;
    SF_FORMAT_INFO finfo;
    int major_count;
    int value_index = 0;
    int i;

    sf_command (NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof (int));

    values = g_new (GEnumValue, major_count + 1);

    for (i = 0; i < major_count; i++)
    {
      finfo.format = i;
      sf_command (NULL, SFC_GET_FORMAT_MAJOR, &finfo, sizeof (finfo));

      /* Skip RAW format since we use IpatchSampleStoreFile instead, for more flexibility */
      if (finfo.format == SF_FORMAT_RAW) continue;

      values[value_index].value = finfo.format;
      values[value_index].value_name = finfo.extension;
      values[value_index].value_nick = finfo.extension;
      value_index++;
    }

    values[value_index].value = 0;
    values[value_index].value_name = NULL;
    values[value_index].value_nick = NULL;

    type = g_enum_register_static ("IpatchSndFileFormat", values);
  }

  return (type);
}

/* Get type of dynamic libsndfile file sub format enum (register it as needed) */
GType
ipatch_snd_file_sub_format_get_type (void)
{
  static GType type = 0;
  char *name, *s;

  if (!type)
  {
    GEnumValue *values;
    SF_FORMAT_INFO sinfo;
    int subtype_count;
    int value_index = 0;
    int i;

    sf_command (NULL, SFC_GET_FORMAT_SUBTYPE_COUNT, &subtype_count, sizeof (int));

    values = g_new (GEnumValue, subtype_count + 1);

    for (i = 0; i < subtype_count; i++)
    {
      sinfo.format = i;
      sf_command (NULL, SFC_GET_FORMAT_SUBTYPE, &sinfo, sizeof (sinfo));

      name = g_ascii_strdown (sinfo.name, -1);        /* ++ alloc forever */

      /* Replace spaces and '.' with dashes */
      for (s = name; *s; s++)
        if (*s == ' ' || *s == '.') *s = '-';

      values[value_index].value = sinfo.format;
      values[value_index].value_name = name;
      values[value_index].value_nick = name;
      value_index++;
    }

    values[value_index].value = 0;
    values[value_index].value_name = NULL;
    values[value_index].value_nick = NULL;

    type = g_enum_register_static ("IpatchSndFileSubFormat", values);
  }

  return (type);
}

static void
ipatch_snd_file_class_init (IpatchSndFileClass *klass)
{
  IpatchFileClass *file_class = IPATCH_FILE_CLASS (klass);
  file_class->identify = ipatch_snd_file_identify;

  /* Set to last execution (subtract another 100, since we really want to be last) */
  file_class->identify_order = IPATCH_FILE_IDENTIFY_ORDER_LAST - 100;
}

static void
ipatch_snd_file_init (IpatchSndFile *file)
{
}

/* Identify if this file format is known by libsndfile */
static gboolean
ipatch_snd_file_identify (IpatchFile *file, IpatchFileHandle *handle, GError **err)
{
  SNDFILE *sfhandle;
  SF_INFO info = { 0 };
  char *filename;

  filename = ipatch_file_get_name (file);       /* ++ alloc file name */
  if (!filename) return (FALSE);

  sfhandle = sf_open (filename, SFM_READ, &info);
  if (sfhandle) sf_close (sfhandle);
  g_free (filename);    /* -- free file name */

  return (sfhandle != NULL);
}

/**
 * ipatch_snd_file_new:
 *
 * Create a new libsndfile file object.
 *
 * Returns: New libsndfile file object (derived from IpatchFile) with a
 * reference count of 1. Caller owns the reference and removing it will
 * destroy the item.
 */
IpatchSndFile *
ipatch_snd_file_new (void)
{
  return (IPATCH_SND_FILE (g_object_new (IPATCH_TYPE_SND_FILE, NULL)));
}

/**
 * ipatch_snd_file_format_get_sub_formats:
 * @format: (type IpatchSndFileFormat): "IpatchSndFileFormat" GEnum to get sub formats of
 * @size: (out): Location to store size of returned sub formats array
 *
 * Get supported sub formats of a given libsndfile format.
 *
 * Returns: (array length=size): Newly allocated list of sub format
 *   enum values or %NULL if @format is invalid
 */
int *
ipatch_snd_file_format_get_sub_formats (int format, guint *size)
{
  SF_FORMAT_INFO info;
  SF_INFO sfinfo;
  GArray *array;
  int subtype_count, s;

  if (size) *size = 0;          /* In case of error */

  g_return_val_if_fail (size != NULL, NULL);

  format &= SF_FORMAT_TYPEMASK;         /* Mask out everything but file type */
  array = g_array_new (FALSE, FALSE, sizeof (int));     /* ++ alloc array */

  memset (&sfinfo, 0, sizeof (sfinfo));
  sf_command (NULL, SFC_GET_FORMAT_SUBTYPE_COUNT, &subtype_count, sizeof (int));
  sfinfo.channels = 1;

  for (s = 0; s < subtype_count; s++)
  {
    info.format = s;
    sf_command (NULL, SFC_GET_FORMAT_SUBTYPE, &info, sizeof (info));

    sfinfo.format = format | info.format;

    if (sf_format_check (&sfinfo))
      g_array_append_val (array, info.format);
  }

  *size = array->len;

  return ((int *)g_array_free (array, FALSE)); /* !! caller takes over alloc */
}

/* FIXME-GIR: @file_format accepts -1 as well! */

/**
 * ipatch_snd_file_sample_format_to_sub_format:
 * @sample_format: libinstpatch sample format (see #sample)
 * @file_format: libsndfile format GEnum "IpatchSndFileFormat" value or -1 to
 *   not limit sub formats to a given file format.
 *
 * Get the optimal libsndfile sub format for a libinstpatch sample format.  The
 * returned value may not be an exact equivalent, in the case of unsigned
 * sample data with bit widths greater than 8, but will return the optimal
 * format in those cases.  If @file_format is not -1 then the resulting sub
 * format is guaranteed to be valid for it.
 *
 * Returns: Optimal libsndfile sub format enum value or -1 on error (invalid
 *   @sample_format).
 */
int
ipatch_snd_file_sample_format_to_sub_format (int sample_format, int file_format)
{
  int sub_format, i;
  int *formats;
  guint size;
  
  g_return_val_if_fail (ipatch_sample_format_verify (sample_format), -1);

  switch (IPATCH_SAMPLE_FORMAT_GET_WIDTH (sample_format))
  {
    case IPATCH_SAMPLE_8BIT:
      sub_format = SF_FORMAT_PCM_S8;
      break;
    case IPATCH_SAMPLE_16BIT:
      sub_format = SF_FORMAT_PCM_16;
      break;
    case IPATCH_SAMPLE_24BIT:
    case IPATCH_SAMPLE_REAL24BIT:
      sub_format = SF_FORMAT_PCM_24;
      break;
    case IPATCH_SAMPLE_32BIT:
      sub_format = SF_FORMAT_PCM_32;
      break;
    case IPATCH_SAMPLE_FLOAT:
      sub_format = SF_FORMAT_FLOAT;
      break;
    case IPATCH_SAMPLE_DOUBLE:
      sub_format = SF_FORMAT_DOUBLE;
      break;
    default:
      sub_format = SF_FORMAT_PCM_16;
      break;
  }

  if (file_format)
  { /* ++ alloc array of valid sub formats for this file format */
    formats = ipatch_snd_file_format_get_sub_formats (file_format, &size);

    if (!formats) return (-1);  /* Invalid file_format value */

    for (i = 0; i < size; i++)
      if (formats[i] == sub_format)
        break;

    if (i == size)
      sub_format = formats[0];  /* sub format not found?  Just use first one.  FIXME? */

    g_free (formats);   /* -- free formats array */
  }

  return (sub_format);
}
