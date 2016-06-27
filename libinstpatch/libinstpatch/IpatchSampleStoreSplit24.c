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
 * SECTION: IpatchSampleStoreSplit24
 * @short_description: Sample storage object for 24 bit audio in 16 and 8 bit segments
 * @see_also: 
 * @stability: Stable
 *
 * SoundFont 2.04 adds support for 24 bit audio.  This is done in a semi
 * backwards compatible fashion where the most significant 16 bits is stored
 * separately from the remaining 8 bit segments.  This storage object handles
 * this transparently.
 */
#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleStoreSplit24.h"
#include "ipatch_priv.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_LOCATION_LSBYTES
};

/* Size of allocated copy buffer for each open sample handle */
#define READBUF_SIZE  16384

static void ipatch_sample_store_split24_sample_iface_init (IpatchSampleIface *iface);
static void ipatch_sample_store_split24_class_init (IpatchSampleStoreSplit24Class *klass);
static void ipatch_sample_store_split24_set_property
  (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_split24_get_property
  (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean ipatch_sample_store_split24_sample_iface_open
  (IpatchSampleHandle *handle, GError **err);
static void ipatch_sample_store_split24_sample_iface_close (IpatchSampleHandle *handle);
static gboolean ipatch_sample_store_split24_sample_iface_read
  (IpatchSampleHandle *handle, guint offset, guint frames, gpointer buf, GError **err);


G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreSplit24, ipatch_sample_store_split24,
                         IPATCH_TYPE_SAMPLE_STORE_FILE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_split24_sample_iface_init))

static void
ipatch_sample_store_split24_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_split24_sample_iface_open;
  iface->close = ipatch_sample_store_split24_sample_iface_close;
  iface->read = ipatch_sample_store_split24_sample_iface_read;
  iface->write = NULL;
}

static void
ipatch_sample_store_split24_class_init (IpatchSampleStoreSplit24Class *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->get_property = ipatch_sample_store_split24_get_property;
  item_class->item_set_property = ipatch_sample_store_split24_set_property;

  g_object_class_install_property (obj_class, PROP_LOCATION_LSBYTES,
      g_param_spec_uint ("location-lsbytes", "Location LS-Bytes",
                         "LS byte sample data file position",
                         0, G_MAXUINT, 0, G_PARAM_READWRITE));
}

static void
ipatch_sample_store_split24_set_property (GObject *object, guint property_id,
                                          const GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreSplit24 *split24 = IPATCH_SAMPLE_STORE_SPLIT24 (object);

  switch (property_id)
    {
    case PROP_LOCATION_LSBYTES:
      g_return_if_fail (split24->loc_lsbytes == 0);

      /* Only set once before use, no lock required */
      split24->loc_lsbytes = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_sample_store_split24_get_property (GObject *object, guint property_id,
				          GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreSplit24 *split24 = IPATCH_SAMPLE_STORE_SPLIT24 (object);

  switch (property_id)
    {
    case PROP_LOCATION_LSBYTES:
      /* Only set once before use, no lock required */
      g_value_set_uint (value, split24->loc_lsbytes);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_sample_store_split24_init (IpatchSampleStoreSplit24 *store)
{
}

static gboolean
ipatch_sample_store_split24_sample_iface_open (IpatchSampleHandle *handle, GError **err)
{
  IpatchSampleStoreSplit24 *split24_store = (IpatchSampleStoreSplit24 *)(handle->sample);
  IpatchSampleStoreFile *file_store = (IpatchSampleStoreFile *)split24_store;
  int fmt;

  g_return_val_if_fail (file_store->file != NULL, FALSE);
  g_return_val_if_fail (file_store->location != 0, FALSE);
  g_return_val_if_fail (split24_store->loc_lsbytes != 0, FALSE);

  fmt = ipatch_sample_store_get_format (split24_store);
  fmt &= ~IPATCH_SAMPLE_ENDIAN_MASK;  /* we can do either endian */
  g_return_val_if_fail (fmt == IPATCH_SAMPLE_24BIT, FALSE);

  /* No lock needed - file object set only once */
  handle->data1 = ipatch_file_open (file_store->file, NULL,
                                    handle->read_mode ? "r" : "w", err);
  if (!handle->data1) return (FALSE);

  handle->data2 = g_malloc (READBUF_SIZE);

  return (TRUE);
}

static void
ipatch_sample_store_split24_sample_iface_close (IpatchSampleHandle *handle)
{
  if (handle->data1)
  {
    ipatch_file_close (handle->data1);
    g_free (handle->data2);
    handle->data1 = NULL;
    handle->data2 = NULL;
  }
}

static gboolean
ipatch_sample_store_split24_sample_iface_read (IpatchSampleHandle *handle,
                                               guint offset, guint frames,
                                               gpointer buf, GError **err)
{
  IpatchSampleStoreSplit24 *split24_store = (IpatchSampleStoreSplit24 *)(handle->sample);
  IpatchSampleStoreFile *file_store = (IpatchSampleStoreFile *)split24_store;
  guint samplepos, thissize, curofs;
  gboolean lilendian;
  IpatchFileHandle *fhandle = (IpatchFileHandle *)(handle->data1);
  guint8 *readbuf = (guint8 *)(handle->data2);
  guint8 *i8p;
  int i;

  lilendian = (ipatch_sample_store_get_format (split24_store)
	       & IPATCH_SAMPLE_ENDIAN_MASK) == IPATCH_SAMPLE_LENDIAN;

  samplepos = 0;
  curofs = offset;
  thissize = READBUF_SIZE / 2;	// 16 bit samples

  /* copy 16 bit sample data */
  while (samplepos < frames)
  {
    if (frames - samplepos < thissize) thissize = frames - samplepos;

    if (!ipatch_file_seek (fhandle, file_store->location + curofs * 2, G_SEEK_SET, err))
      return (FALSE);

    if (!ipatch_file_read (fhandle, readbuf, thissize * 2, err))
      return (FALSE);

    i8p = buf + samplepos * 4;

    if (lilendian)
    {
      for (i = 0; i < thissize; i++)
      {
        i8p[i * 4 + 1] = readbuf[i * 2];
        i8p[i * 4 + 2] = readbuf[i * 2 + 1];
        i8p[i * 4 + 3] = 0;
      }
    }
    else
    {
      for (i = 0; i < thissize; i++)
      {
        i8p[i * 4 + 2] = readbuf[i * 2];
        i8p[i * 4 + 1] = readbuf[i * 2 + 1];
        i8p[i * 4 + 0] = 0;
      }
    }

    samplepos += thissize;
    curofs += thissize;
  }


  samplepos = 0;
  curofs = offset;
  thissize = READBUF_SIZE;

  /* copy upper byte of 24 bit samples */
  while (samplepos < frames)
  {
    if (frames - samplepos < thissize) thissize = frames - samplepos;

    if (!ipatch_file_seek (fhandle, split24_store->loc_lsbytes + curofs, G_SEEK_SET, err))
      return (FALSE);

    if (!ipatch_file_read (fhandle, readbuf, thissize, err))
      return (FALSE);

    i8p = buf + samplepos * 4;

    if (lilendian)
    {
      for (i = 0; i < thissize; i++)
        i8p[i * 4] = readbuf[i];
    }
    else
    {
      for (i = 0; i < thissize; i++)
        i8p[i * 4 + 3] = readbuf[i];
    }

    samplepos += thissize;
    curofs += thissize;
  }

  return (TRUE);
}

/**
 * ipatch_sample_store_split24_new:
 * @file: File object
 * @loc_16bit: Location of 16 bit audio data
 * @loc_lsbytes: Location of 24 bit LS bytes
 *
 * Creates a new split 24 bit sample store (lower byte of 24 bit
 * samples is stored in a separate block).  New SoundFont 2.04 uses this method.
 *
 * Returns: (type IpatchSampleStoreSplit24): New split 24 sample store
 */
IpatchSample *
ipatch_sample_store_split24_new (IpatchFile *file, guint loc_16bit,
                                 guint loc_lsbytes)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_SPLIT24,
                                       "file", file,
                                       "location", loc_16bit,
                                       "location-lsbytes", loc_lsbytes,
                                       NULL)));
}
