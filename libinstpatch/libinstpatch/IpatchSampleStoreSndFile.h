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
#ifndef __IPATCH_SAMPLE_STORE_SND_FILE_H__
#define __IPATCH_SAMPLE_STORE_SND_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSampleStore.h>

#include <sndfile.h>

/* forward type declarations */

typedef struct _IpatchSampleStoreSndFile IpatchSampleStoreSndFile;
typedef struct _IpatchSampleStoreSndFileClass IpatchSampleStoreSndFileClass;

#define IPATCH_TYPE_SAMPLE_STORE_SND_FILE (ipatch_sample_store_snd_file_get_type ())
#define IPATCH_SAMPLE_STORE_SND_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_STORE_SND_FILE, \
  IpatchSampleStoreSndFile))
#define IPATCH_SAMPLE_STORE_SND_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_STORE_SND_FILE, \
  IpatchSampleStoreSndFileClass))
#define IPATCH_IS_SAMPLE_STORE_SND_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_STORE_SND_FILE))
#define IPATCH_IS_SAMPLE_STORE_SND_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_STORE_SND_FILE))


/* libsndfile sample instance */
struct _IpatchSampleStoreSndFile
{
  IpatchSampleStore parent_instance;

  char *filename;               /* File name where sample data is */
  gboolean identified;          /* TRUE if file has been identified (for reading) */
  gboolean raw;                 /* TRUE if audio data is read raw from libsndfile */
  int file_format;              /* File format enum (enum is dynamic) */
  int sub_format;               /* File sub format enum (enum is dynamic) */
  int endian;                   /* File endian byte order enum */

  guint loop_start;             /* loop start */
  guint loop_end;               /* loop end */
  guint8 loop_type;             /* loop type */
  guint8 root_note;             /* root note */
  gint8 fine_tune;              /* fine tune */
};

/* libsndfile sample class */
struct _IpatchSampleStoreSndFileClass
{
  IpatchSampleStoreClass parent_class;
};

/**
 * IPATCH_SAMPLE_STORE_SND_FILE_UNUSED_FLAG_SHIFT: (skip)
 */
/* we reserve 3 flags for expansion */
#define IPATCH_SAMPLE_STORE_SND_FILE_UNUSED_FLAG_SHIFT \
  (IPATCH_SAMPLE_STORE_UNUSED_FLAG_SHIFT + 3)

GType ipatch_sample_store_snd_file_get_type (void);
IpatchSample *ipatch_sample_store_snd_file_new (const char *filename);
gboolean ipatch_sample_store_snd_file_init_read (IpatchSampleStoreSndFile *store);
gboolean ipatch_sample_store_snd_file_init_write (IpatchSampleStoreSndFile *store,
                                                  int file_format, int sub_format,
                                                  int endian, int channels,
                                                  int samplerate);
#endif
