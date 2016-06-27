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
 * SECTION: IpatchSampleStoreCache
 * @short_description: Sample store object for cached samples in RAM
 * @see_also: #IpatchSampleData
 * @stability: Stable
 *
 * This sample store type is tightly integrated with #IpatchSampleData to provide
 * managed cached samples in RAM.
 */
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleStoreCache.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* properties */
enum
{
  PROP_0,
  PROP_LOCATION
};

/* Defined in IpatchSampleData.c */
extern void _ipatch_sample_data_cache_add_unused_size (int size);

static void ipatch_sample_store_cache_sample_iface_init (IpatchSampleIface *iface);
static void ipatch_sample_store_cache_set_property
  (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_cache_get_property
  (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_cache_finalize (GObject *object);
static gboolean ipatch_sample_store_cache_sample_iface_open
  (IpatchSampleHandle *handle, GError **err);
static void ipatch_sample_store_cache_sample_iface_close (IpatchSampleHandle *handle);
static gboolean ipatch_sample_store_cache_sample_iface_read
  (IpatchSampleHandle *handle, guint offset, guint frames, gpointer buf, GError **err);
static gboolean ipatch_sample_store_cache_sample_iface_write
  (IpatchSampleHandle *handle, guint offset, guint frames, gconstpointer buf, GError **err);


G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreCache, ipatch_sample_store_cache,
                         IPATCH_TYPE_SAMPLE_STORE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_cache_sample_iface_init))

static void
ipatch_sample_store_cache_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_cache_sample_iface_open;
  iface->close = ipatch_sample_store_cache_sample_iface_close;
  iface->read = ipatch_sample_store_cache_sample_iface_read;
  iface->write = ipatch_sample_store_cache_sample_iface_write;
}

static void
ipatch_sample_store_cache_class_init (IpatchSampleStoreCacheClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_sample_store_cache_finalize;
  obj_class->get_property = ipatch_sample_store_cache_get_property;
  item_class->item_set_property = ipatch_sample_store_cache_set_property;

  g_object_class_install_property (obj_class, PROP_LOCATION,
      g_param_spec_pointer ("location", "Location", "Sample data pointer",
                            G_PARAM_READWRITE));
}

static void
ipatch_sample_store_cache_set_property (GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreCache *store = IPATCH_SAMPLE_STORE_CACHE (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      g_return_if_fail (store->location == NULL);

      /* Lock not needed, should be set only once before use */
      store->location = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_sample_store_cache_get_property (GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreCache *store = IPATCH_SAMPLE_STORE_CACHE (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      /* Lock not needed, should be set only once before use */
      g_value_set_pointer (value, store->location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_sample_store_cache_init (IpatchSampleStoreCache *store)
{
  GTimeVal timeval;

  /* Initialize last open to current time */
  g_get_current_time (&timeval);
  store->last_open = timeval.tv_sec;
}

static void
ipatch_sample_store_cache_finalize (GObject *object)
{
  IpatchSampleStoreCache *store = IPATCH_SAMPLE_STORE_CACHE (object);

  g_free (store->location);
  store->location = NULL;

  if (G_OBJECT_CLASS (ipatch_sample_store_cache_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sample_store_cache_parent_class)->finalize (object);
}

static gboolean
ipatch_sample_store_cache_sample_iface_open (IpatchSampleHandle *handle,
                                             GError **err)
{
  IpatchSampleStoreCache *store = IPATCH_SAMPLE_STORE_CACHE (handle->sample);
  guint bytes;

  g_return_val_if_fail (!handle->read_mode || store->location, FALSE);

  IPATCH_ITEM_WLOCK (store);
  store->last_open = 0;         /* Reset time to 0 to indicate store is open */

  if (store->open_count == 0)   /* Recursive lock: store, sample_cache_vars */
    _ipatch_sample_data_cache_add_unused_size
      (-(int)ipatch_sample_store_get_size_bytes ((IpatchSampleStore *)store));

  g_atomic_int_inc (&store->open_count);
  IPATCH_ITEM_WUNLOCK (store);

  /* Locking not needed, since new samples will be written with audio before
   * being used by multiple threads */
  if (!store->location)
  {
    bytes = ipatch_sample_store_get_size_bytes ((IpatchSampleStore *)store);
    store->location = g_malloc0 (bytes);
  }

  /* Store frame size to data1 */
  handle->data1 = GUINT_TO_POINTER (ipatch_sample_format_size (ipatch_sample_store_get_format (store)));

  return (TRUE);
}

static void
ipatch_sample_store_cache_sample_iface_close (IpatchSampleHandle *handle)
{
  IpatchSampleStoreCache *store = IPATCH_SAMPLE_STORE_CACHE (handle->sample);
  GTimeVal timeval;

  IPATCH_ITEM_WLOCK (store);

  /* Set last open time if there are no more open handles */
  if (g_atomic_int_dec_and_test (&store->open_count))
  {
    g_get_current_time (&timeval);
    store->last_open = timeval.tv_sec;

    /* Recursive lock: store, sample_cache_vars */
    _ipatch_sample_data_cache_add_unused_size
      (ipatch_sample_store_get_size_bytes ((IpatchSampleStore *)store));
  }

  IPATCH_ITEM_WUNLOCK (store);
}

static gboolean
ipatch_sample_store_cache_sample_iface_read (IpatchSampleHandle *handle,
                                             guint offset, guint frames,
                                             gpointer buf, GError **err)
{
  IpatchSampleStoreCache *store = (IpatchSampleStoreCache *)(handle->sample);
  guint8 frame_size = GPOINTER_TO_UINT (handle->data1);

  /* No need to lock, sample data should not change after initial load */

  memcpy (buf, &((gint8 *)(store->location))[offset * frame_size],
	  frames * frame_size);

  return (TRUE);
}

static gboolean
ipatch_sample_store_cache_sample_iface_write (IpatchSampleHandle *handle,
                                              guint offset, guint frames,
                                              gconstpointer buf, GError **err)
{
  IpatchSampleStoreCache *store = (IpatchSampleStoreCache *)(handle->sample);
  guint8 frame_size = GPOINTER_TO_UINT (handle->data1);

  /* No need to lock, sample data written only once and before used by
     multiple threads */

  memcpy (&((gint8 *)(store->location))[offset * frame_size], buf,
	  frames * frame_size);

  return (TRUE);
}

/**
 * ipatch_sample_store_cache_new:
 * @location: Location of existing sample data or %NULL if the sample buffer
 *   should be allocated (in which case the sample must be written to first).
 *
 * Creates a new cached RAM sample store.  If @location is provided, its allocation
 * is taken over by the store.
 *
 * NOTE: This store type should not be used outside of the #IpatchSampleData
 * implementation, as it is tightly coupled with it.
 *
 * Returns: (type IpatchSampleStoreCache): New cached RAM sample store, cast
 *   as a #IpatchSample for convenience.
 */
IpatchSample *
ipatch_sample_store_cache_new (gpointer location)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_CACHE,
                                       "location", location,
                                       NULL)));
}

/**
 * ipatch_sample_store_cache_open:
 * @store: Sample cache store
 *
 * A dummy open function which can be used if the location pointer will be
 * accessed directly, rather than opening a #IpatchSampleHandle.  Keeping a
 * cached sample store open will ensure it isn't destroyed.  Call
 * ipatch_sample_store_cache_close() when done with it.
 */
void
ipatch_sample_store_cache_open (IpatchSampleStoreCache *store)
{
  int size;

  g_return_if_fail (IPATCH_IS_SAMPLE_STORE_CACHE (store));

  size = ipatch_sample_store_get_size_bytes ((IpatchSampleStore *)store);

  IPATCH_ITEM_WLOCK (store);
  store->last_open = 0;         /* Reset time to 0 to indicate store is open */

  if (store->open_count == 0)   /* Recursive lock: store, sample_cache_vars */
    _ipatch_sample_data_cache_add_unused_size (-size);

  g_atomic_int_inc (&store->open_count);
  IPATCH_ITEM_WUNLOCK (store);
}

/**
 * ipatch_sample_store_cache_close:
 * @store: Sample cache store
 *
 * A dummy close function which is called after a sample store cache is no
 * longer needed after opening it with ipatch_sample_store_cache_open().
 */
void
ipatch_sample_store_cache_close (IpatchSampleStoreCache *store)
{
  GTimeVal timeval;
  int size;

  g_return_if_fail (IPATCH_IS_SAMPLE_STORE_CACHE (store));

  size = ipatch_sample_store_get_size_bytes ((IpatchSampleStore *)store);

  IPATCH_ITEM_WLOCK (store);

  /* Set last open time if there are no more open handles */
  if (g_atomic_int_dec_and_test (&store->open_count))
  {
    g_get_current_time (&timeval);
    store->last_open = timeval.tv_sec;

    /* Recursive lock: store, sample_cache_vars */
    _ipatch_sample_data_cache_add_unused_size (size);
  }

  IPATCH_ITEM_WUNLOCK (store);
}
