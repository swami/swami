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
#ifndef __IPATCH_SND_FILE_H__
#define __IPATCH_SND_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchFile.h>

#include <sndfile.h>

typedef struct _IpatchSndFile IpatchSndFile;
typedef struct _IpatchSndFileClass IpatchSndFileClass;

#define IPATCH_TYPE_SND_FILE   (ipatch_snd_file_get_type ())
#define IPATCH_SND_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SND_FILE, IpatchSndFile))
#define IPATCH_SND_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SND_FILE, \
   IpatchSndFileClass))
#define IPATCH_IS_SND_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SND_FILE))
#define IPATCH_IS_SND_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SND_FILE))
#define IPATCH_SND_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SND_FILE, \
   IpatchSndFileClass))

#define IPATCH_TYPE_SND_FILE_FORMAT \
  (ipatch_snd_file_format_get_type())

#define IPATCH_TYPE_SND_FILE_SUB_FORMAT \
  (ipatch_snd_file_sub_format_get_type())

/**
 * IpatchSndFileEndian:
 * @IPATCH_SND_FILE_ENDIAN_FILE: Use the default endian for the file format
 * @IPATCH_SND_FILE_ENDIAN_LITTLE: Little endian byte order
 * @IPATCH_SND_FILE_ENDIAN_BIG: Big endian byte order
 * @IPATCH_SND_FILE_ENDIAN_CPU: Native CPU byte order
 *
 * Endian byte order libsndfile enum.
 */
typedef enum
{
  IPATCH_SND_FILE_ENDIAN_FILE,
  IPATCH_SND_FILE_ENDIAN_LITTLE,
  IPATCH_SND_FILE_ENDIAN_BIG,
  IPATCH_SND_FILE_ENDIAN_CPU
} IpatchSndFileEndian;

/**
 * IPATCH_SND_FILE_DEFAULT_FORMAT:
 *
 * Default file format enum for #IPATCH_TYPE_SND_FILE_FORMAT.
 */
#define IPATCH_SND_FILE_DEFAULT_FORMAT     SF_FORMAT_WAV

/**
 * IPATCH_SND_FILE_DEFAULT_SUB_FORMAT:
 *
 * Default file sub format enum for #IPATCH_TYPE_SND_FILE_SUB_FORMAT.
 */
#define IPATCH_SND_FILE_DEFAULT_SUB_FORMAT SF_FORMAT_PCM_16

/**
 * IPATCH_SND_FILE_DEFAULT_ENDIAN:
 *
 * Default endian byte order from enum #IPATCH_TYPE_SND_FILE_ENDIAN.
 */
#define IPATCH_SND_FILE_DEFAULT_ENDIAN     IPATCH_SND_FILE_ENDIAN_FILE

/* DLS file object (derived from IpatchFile) */
struct _IpatchSndFile
{
  IpatchFile parent_instance;
};

/* DLS file class (derived from IpatchFile) */
struct _IpatchSndFileClass
{
  IpatchFileClass parent_class;
};


GType ipatch_snd_file_get_type (void);
IpatchSndFile *ipatch_snd_file_new (void);
GType ipatch_snd_file_format_get_type (void);
GType ipatch_snd_file_sub_format_get_type (void);
int *ipatch_snd_file_format_get_sub_formats (int format, guint *size);
int ipatch_snd_file_sample_format_to_sub_format (int sample_format, int file_format);

#endif
