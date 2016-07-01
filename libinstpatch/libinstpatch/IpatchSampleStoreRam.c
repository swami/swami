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
 * SECTION: IpatchSampleStoreRam
 * @short_description: Sample store object for audio data in RAM
 * @see_also: 
 * @stability: Stable
 */
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleStoreRam.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* properties */
enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_FREE_DATA
};


static void ipatch_sample_store_ram_sample_iface_init (IpatchSampleIface *iface);
static void ipatch_sample_store_ram_set_property
  (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_ram_get_property
  (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_ram_finalize (GObject *object);
static gboolean ipatch_sample_store_ram_sample_iface_open
  (IpatchSampleHandle *handle, GError **err);
static gboolean ipatch_sample_store_ram_sample_iface_read
  (IpatchSampleHandle *handle, guint offset, guint frames, gpointer buf, GError **err);
static gboolean ipatch_sample_store_ram_sample_iface_write
  (IpatchSampleHandle *handle, guint offset, guint frames, gconstpointer buf, GError **err);


G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreRam, ipatch_sample_store_ram,
                         IPATCH_TYPE_SAMPLE_STORE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_ram_sample_iface_init))

static void
ipatch_sample_store_ram_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_ram_sample_iface_open;
  iface->read = ipatch_sample_store_ram_sample_iface_read;
  iface->write = ipatch_sample_store_ram_sample_iface_write;
}

static void
ipatch_sample_store_ram_class_init (IpatchSampleStoreRamClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_sample_store_ram_finalize;
  obj_class->get_property = ipatch_sample_store_ram_get_property;
  item_class->item_set_property = ipatch_sample_store_ram_set_property;

  g_object_class_install_property (obj_class, PROP_LOCATION,
      g_param_spec_pointer ("location", "Location", "Sample data pointer",
                            G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_FREE_DATA,
      g_param_spec_boolean ("free-data", "Free data", "Free data when object destroyed",
                            FALSE, G_PARAM_READWRITE));
}

static void
ipatch_sample_store_ram_set_property (GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreRam *store = IPATCH_SAMPLE_STORE_RAM (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      g_return_if_fail (store->location == NULL);

      /* Lock not needed, should be set only once before use */
      store->location = g_value_get_pointer (value);
      break;
    case PROP_FREE_DATA:
      ipatch_item_set_flags (object, IPATCH_SAMPLE_STORE_RAM_ALLOCATED);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_sample_store_ram_get_property (GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreRam *store = IPATCH_SAMPLE_STORE_RAM (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      /* Lock not needed, should be set only once before use */
      g_value_set_pointer (value, store->location);
      break;
    case PROP_FREE_DATA:
      g_value_set_boolean (value, (ipatch_item_get_flags ((IpatchItem *)object)
                                   & IPATCH_SAMPLE_STORE_RAM_ALLOCATED) != 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_sample_store_ram_init (IpatchSampleStoreRam *store)
{
}

static void
ipatch_sample_store_ram_finalize (GObject *object)
{
  IpatchSampleStoreRam *store = IPATCH_SAMPLE_STORE_RAM (object);

  if (ipatch_item_get_flags (IPATCH_ITEM (store)) & IPATCH_SAMPLE_STORE_RAM_ALLOCATED)
  {
    g_free (store->location);
    store->location = NULL;
  }

  if (G_OBJECT_CLASS (ipatch_sample_store_ram_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sample_store_ram_parent_class)->finalize (object);
}

static gboolean
ipatch_sample_store_ram_sample_iface_open (IpatchSampleHandle *handle,
                                           GError **err)
{
  IpatchSampleStoreRam *store = IPATCH_SAMPLE_STORE_RAM (handle->sample);
  guint bytes;

  g_return_val_if_fail (!handle->read_mode || store->location, FALSE);

  /* Locking not needed, since new samples will be written with audio before
   * being used by multiple threads */
  if (!store->location)
  {
    ipatch_item_set_flags (IPATCH_ITEM (store), IPATCH_SAMPLE_STORE_RAM_ALLOCATED);
    ipatch_sample_get_size (handle->sample, &bytes);
    store->location = g_malloc0 (bytes);
  }

  /* Store frame size to data1 */
  handle->data1 = GUINT_TO_POINTER (ipatch_sample_format_size (ipatch_sample_store_get_format (store)));

  return (TRUE);
}

static gboolean
ipatch_sample_store_ram_sample_iface_read (IpatchSampleHandle *handle,
                                           guint offset, guint frames,
                                           gpointer buf, GError **err)
{
  IpatchSampleStoreRam *store = (IpatchSampleStoreRam *)(handle->sample);
  guint8 frame_size = GPOINTER_TO_UINT (handle->data1);

  /* No need to lock, sample data should not change after initial load */

  memcpy (buf, &((gint8 *)(store->location))[offset * frame_size],
	  frames * frame_size);

  return (TRUE);
}

static gboolean
ipatch_sample_store_ram_sample_iface_write (IpatchSampleHandle *handle,
                                            guint offset, guint frames,
                                            gconstpointer buf, GError **err)
{
  IpatchSampleStoreRam *store = (IpatchSampleStoreRam *)(handle->sample);
  guint8 frame_size = GPOINTER_TO_UINT (handle->data1);

  /* No need to lock, sample data written only once and before used by
     multiple threads */

  memcpy (&((gint8 *)(store->location))[offset * frame_size], buf,
	  frames * frame_size);

  return (TRUE);
}

/**
 * ipatch_sample_store_ram_new:
 * @location: (nullable): Location of existing sample data or %NULL if the sample buffer
 *   should be allocated (in which case the sample must be written to first).
 * @free_data: %TRUE if sample data at @location should be freed when object
 *   is destroyed
 *
 * Creates a new RAM sample store.
 *
 * Returns: (type IpatchSampleStoreRam): New RAM sample store,
 *   cast as a #IpatchSample for convenience.
 */
IpatchSample *
ipatch_sample_store_ram_new (gpointer location, gboolean free_data)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_RAM,
                                       "location", location,
                                       "free-data", free_data,
                                       NULL)));
}

/**
 * ipatch_sample_store_ram_get_blank:
 *
 * Get blank mono RAM sample object.  Return's a sample object
 * with 48 stereo 16 bit samples of silent audio.  Only creates it on
 * the first call, subsequent calls return the same sample object. Therefore it
 * should not be modified.
 *
 * Returns: (transfer full): The blank sample object. Remember to unref it when not
 * using it anymore with g_object_unref().
 */
IpatchSample *
ipatch_sample_store_ram_get_blank (void)
{
  static IpatchSample *blank_sample = NULL;
  gpointer dataptr;

  if (!blank_sample)
  {
    dataptr = g_malloc (48 * 2);
    blank_sample = ipatch_sample_store_ram_new (dataptr, TRUE);

    g_object_set (blank_sample,
                  "sample-size", 48,
                  "sample-format", IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_ENDIAN_HOST,
                  "sample-rate", IPATCH_SAMPLE_RATE_DEFAULT,
                  NULL);
  }
  else g_object_ref (blank_sample);

  return (blank_sample);
}
