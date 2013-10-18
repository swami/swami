/*
 * libInstPatch
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleData.h"
#include "IpatchSampleStoreCache.h"
#include "IpatchSampleStoreRam.h"
#include "IpatchSample.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_SAMPLE_SIZE,
  PROP_SAMPLE_FORMAT,
  PROP_SAMPLE_RATE,
  PROP_SAMPLE_DATA,
  PROP_LOOP_TYPE,
  PROP_LOOP_START,
  PROP_LOOP_END,
  PROP_ROOT_NOTE,
  PROP_FINE_TUNE
};

#define OBJECT_REFCOUNT(obj)         (((GObject *)(obj))->ref_count)
#define OBJECT_REFCOUNT_VALUE(obj)   g_atomic_int_get ((gint *)&OBJECT_REFCOUNT (obj))

/* Info structure used to ensure that duplicate sample caching does not occur */
typedef struct
{
  IpatchSampleStore *store;
  int format;
  guint32 channel_map;
} CachingInfo;

static void ipatch_sample_data_sample_iface_init (IpatchSampleIface *iface);
static gboolean ipatch_sample_data_sample_iface_open (IpatchSampleHandle *handle,
                                                      GError **err);
static void ipatch_sample_data_get_property (GObject *object, guint property_id,
                                             GValue *value, GParamSpec *pspec);
static void ipatch_sample_data_finalize (GObject *gobject);
static gint sample_cache_clean_sort (gconstpointer a, gconstpointer b);


/* master sample data list and lock */
G_LOCK_DEFINE_STATIC (sample_data_list);
static GSList *sample_data_list = NULL;

/* Lock for metric variables below */
G_LOCK_DEFINE_STATIC (sample_cache_vars);
static guint64 sample_cache_total_size = 0;     /* Total size of cached samples */
static guint64 sample_cache_unused_size = 0;    /* Size of unused cached samples */

/* Variables used to ensure that duplicate sample caching does not occur */
static GMutex caching_mutex;
static GCond caching_cond;
static GSList *caching_list = NULL;


G_DEFINE_TYPE_WITH_CODE (IpatchSampleData, ipatch_sample_data, IPATCH_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_data_sample_iface_init))


/**
 * ipatch_get_sample_data_list:
 *
 * Creates an object list copy of the master sample data list (all
 * existing sample data objects).
 *
 * Returns: New object list populated with all #IpatchSampleData objects
 * with a reference count of 1 which the caller owns, removing the reference
 * will free the list.
 */
IpatchList *
ipatch_sample_data_get_list (void)
{
  IpatchList *list;
  GSList *p;

  list = ipatch_list_new ();	/* ++ ref new list */

  G_LOCK (sample_data_list);

  for (p = sample_data_list; p; p = p->next)
  {
    list->items = g_list_prepend (list->items, p->data);
    g_object_ref (p->data);	/* ++ ref object for list */
  }

  G_UNLOCK (sample_data_list);

  return (list);		/* !! caller takes over reference */
}

static void
ipatch_sample_data_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_data_sample_iface_open;
}

static gboolean
ipatch_sample_data_sample_iface_open (IpatchSampleHandle *handle, GError **err)
{
  IpatchSampleData *sampledata = IPATCH_SAMPLE_DATA (handle->sample);
  IpatchSample *sample = NULL;
  gboolean retval;

  IPATCH_ITEM_RLOCK (sampledata);
  if (sampledata->samples) sample = g_object_ref (sampledata->samples->data);   /* ++ ref */
  IPATCH_ITEM_RUNLOCK (sampledata);

  g_return_val_if_fail (sample != NULL, FALSE);

  retval = ipatch_sample_handle_cascade_open (handle, sample, err);
  g_object_unref (sample);      /* -- unref sample */
  return (retval);
}

static void
ipatch_sample_data_class_init (IpatchSampleDataClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = ipatch_sample_data_finalize;
  obj_class->get_property = ipatch_sample_data_get_property;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_SIZE, "sample-size");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_FORMAT, "sample-format");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_RATE, "sample-rate");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_DATA, "sample-data");
  ipatch_sample_install_property_readonly (obj_class, PROP_LOOP_TYPE, "loop-type");
  ipatch_sample_install_property_readonly (obj_class, PROP_LOOP_START, "loop-start");
  ipatch_sample_install_property_readonly (obj_class, PROP_LOOP_END, "loop-end");
  ipatch_sample_install_property_readonly (obj_class, PROP_ROOT_NOTE, "root-note");
  ipatch_sample_install_property_readonly (obj_class, PROP_FINE_TUNE, "fine-tune");
}

static void
ipatch_sample_data_get_property (GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec)
{
  IpatchSampleData *sampledata = IPATCH_SAMPLE_DATA (object);
  IpatchSampleStore *store = NULL;

  IPATCH_ITEM_RLOCK (sampledata);
  if (sampledata->samples)
    store = g_object_ref (sampledata->samples->data);   /* ++ ref */
  IPATCH_ITEM_RUNLOCK (sampledata);

  g_return_if_fail (store != NULL);

  switch (property_id)
  {
    case PROP_TITLE:
      g_object_get_property ((GObject *)store, "title", value);
      break;
    case PROP_SAMPLE_SIZE:
      g_value_set_uint (value, ipatch_sample_store_get_size (store));
      break;
    case PROP_SAMPLE_FORMAT:
      g_value_set_int (value, ipatch_sample_store_get_format (store));
      break;
    case PROP_SAMPLE_RATE:
      g_value_set_int (value, ipatch_sample_store_get_rate (store));
      break;
    case PROP_SAMPLE_DATA:
      g_value_set_object (value, sampledata);
      break;
    case PROP_LOOP_TYPE:
      g_object_get_property ((GObject *)store, "loop-type", value);
      break;
    case PROP_LOOP_START:
      g_object_get_property ((GObject *)store, "loop-start", value);
      break;
    case PROP_LOOP_END:
      g_object_get_property ((GObject *)store, "loop-end", value);
      break;
    case PROP_ROOT_NOTE:
      g_object_get_property ((GObject *)store, "root-note", value);
      break;
    case PROP_FINE_TUNE:
      g_object_get_property ((GObject *)store, "fine-tune", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  g_object_unref (store);       /* -- unref store */
}

static void
ipatch_sample_data_init (IpatchSampleData *sampledata)
{
  /* add to the master list */
  G_LOCK (sample_data_list);
  sample_data_list = g_slist_prepend (sample_data_list, sampledata);
  G_UNLOCK (sample_data_list);
}

static void
ipatch_sample_data_finalize (GObject *gobject)
{
  IpatchSampleData *sampledata = IPATCH_SAMPLE_DATA (gobject);

  /* remove from master list */
  G_LOCK (sample_data_list);
  sample_data_list = g_slist_remove (sample_data_list, sampledata);
  G_UNLOCK (sample_data_list);

  if (G_OBJECT_CLASS (ipatch_sample_data_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sample_data_parent_class)->finalize (gobject);
}

/**
 * ipatch_sample_data_new:
 *
 * Create a new sample data object.
 *
 * Returns: New sample data with a reference count of 1. Caller owns
 * the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchSampleData *
ipatch_sample_data_new (void)
{
  return (IPATCH_SAMPLE_DATA (g_object_new (IPATCH_TYPE_SAMPLE_DATA, NULL)));
}

/**
 * ipatch_sample_data_add:
 * @sampledata: Sample data object
 * @store: Sample store to add
 *
 * Add a sample to a sample data object.  If no samples have yet been added,
 * then the added sample becomes the native sample.  All samples added to a
 * given @sampledata object must have the same frame count and should also
 * have the same sample rate.  This is not enforced though and is left to the
 * caller to ensure.
 */
void
ipatch_sample_data_add (IpatchSampleData *sampledata, IpatchSampleStore *store)
{
  g_return_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata));
  g_return_if_fail (IPATCH_IS_SAMPLE_STORE (store));

  g_object_ref (store);         /* ++ ref sample for sampledata object */

  /* IpatchSampleData not really a container, just set the store's parent directly */
  IPATCH_ITEM (store)->parent = IPATCH_ITEM (sampledata);

  IPATCH_ITEM_WLOCK (sampledata);
  sampledata->samples = g_slist_append (sampledata->samples, store);
  IPATCH_ITEM_WUNLOCK (sampledata);
}

/**
 * ipatch_sample_data_remove:
 * @sampledata: Sample data object
 * @store: Sample store to remove
 *
 * Remove a sample from a sample data object.  The native sample should not
 * be removed from an active sample data object.  Use
 * ipatch_sample_data_replace_native_sample() if replacement is desired.
 */
void
ipatch_sample_data_remove (IpatchSampleData *sampledata, IpatchSampleStore *store)
{
  GSList *p, *prev = NULL;
  guint size_bytes;

  g_return_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata));
  g_return_if_fail (IPATCH_IS_SAMPLE_STORE (store));

  IPATCH_ITEM_WLOCK (sampledata);

  for (p = sampledata->samples; p; prev = p, p = p->next)
  {
    if (p->data == store)
    {
      if (prev) prev->next = p->next;
      else sampledata->samples = p->next;

      break;
    }
  }

  IPATCH_ITEM_WUNLOCK (sampledata);

  if (p)
  {
    if (IPATCH_IS_SAMPLE_STORE_CACHE (p->data))
    {
      IpatchSampleStoreCache *store = p->data;

      size_bytes = ipatch_sample_store_get_size_bytes ((IpatchSampleStore *)store);

      IPATCH_ITEM_RLOCK (store);        /* ++ lock store */

      /* Recursive lock: store, sample_cache_vars */
      G_LOCK (sample_cache_vars);

      sample_cache_total_size -= size_bytes;

      /* Only subtract unused size from total unused size, if no opens active */
      if (store->open_count == 0)
        sample_cache_unused_size -= size_bytes;

      G_UNLOCK (sample_cache_vars);

      IPATCH_ITEM_RUNLOCK (store);      /* -- unlock store */
    }

    g_object_unref (p->data);   /* -- unref sample */
    g_slist_free_1 (p);
  }
}

/**
 * ipatch_sample_data_replace_native_sample:
 * @sampledata: Sample data object
 * @store: Sample store object
 *
 * Replace the native sample of a sample data object.  This function can be used
 * even if there are no samples yet, in which case it behaves just like
 * ipatch_sample_data_add().
 */
void
ipatch_sample_data_replace_native_sample (IpatchSampleData *sampledata,
                                          IpatchSampleStore *store)
{
  IpatchSampleStore *oldsample = NULL;

  g_return_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata));
  g_return_if_fail (IPATCH_IS_SAMPLE_STORE (store));

  g_object_ref (store);        /* ++ ref sample for sampledata */

  /* IpatchSampleData not really a container, just set the store's parent directly */
  IPATCH_ITEM (store)->parent = IPATCH_ITEM (sampledata);

  IPATCH_ITEM_WLOCK (sampledata);

  if (sampledata->samples)
  {
    oldsample = sampledata->samples->data;
    sampledata->samples->data = store;
  }
  else sampledata->samples = g_slist_prepend (sampledata->samples, store);

  IPATCH_ITEM_WUNLOCK (sampledata);

  if (oldsample) g_object_unref (oldsample);    /* -- unref sample */
}

/**
 * ipatch_sample_data_get_samples:
 * @sampledata: Sample data object
 *
 * Get an object list of samples in a sample data object.  The first sample is
 * the native sample.
 *
 * Returns: Newly created list of #IpatchSampleStore objects with a refcount of
 *   1 which the caller owns.
 */
IpatchList *
ipatch_sample_data_get_samples (IpatchSampleData *sampledata)
{
  IpatchList *list;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), NULL);

  list = ipatch_list_new ();	/* ++ ref new list */

  IPATCH_ITEM_RLOCK (sampledata);

  for (p = sampledata->samples; p; p = p->next)
  {
    g_object_ref (p->data);             /* ++ ref object for list */
    list->items = g_list_prepend (list->items, p->data);        /* Prepend for speed */
  }

  IPATCH_ITEM_RUNLOCK (sampledata);

  list->items = g_list_reverse (list->items);   /* Correct for prepend operation */

  return (list);        /* !! caller takes over reference */
}

/**
 * ipatch_sample_data_get_size:
 * @sampledata: Sample data to get size of
 *
 * Get the size in frames of the samples in the @sampledata object.
 *
 * Returns: Size in frames of stores in sample data.
 */
guint
ipatch_sample_data_get_size (IpatchSampleData *sampledata)
{
  guint size = 0;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), 0);

  IPATCH_ITEM_RLOCK (sampledata);
  if (sampledata->samples)
    size = ipatch_sample_store_get_size ((IpatchSampleStore *)(sampledata->samples->data));
  IPATCH_ITEM_RUNLOCK (sampledata);

  return (size);
}

/**
 * ipatch_sample_data_get_native_sample:
 * @sampledata: Sample data object
 *
 * Get the native sample of a sample data object.
 *
 * Returns: Native sample, or %NULL if no native sample in the sample data object,
 *   caller owns a reference.
 */
IpatchSampleStore *
ipatch_sample_data_get_native_sample (IpatchSampleData *sampledata)
{
  IpatchSampleStore *sample = NULL;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), NULL);

  IPATCH_ITEM_RLOCK (sampledata);
  if (sampledata->samples)
    sample = g_object_ref (sampledata->samples->data);  /* ++ ref sample */
  IPATCH_ITEM_RUNLOCK (sampledata);

  return (sample);  /* !! caller takes over ref */
}

/**
 * ipatch_sample_data_get_native_format:
 * @sampledata: Sample data object
 *
 * Convenience function to get the sample format of the native sample in a
 * sample data object. See ipatch_sample_get_format() for more info.
 *
 * Returns: Sample format or 0 if @sampledata has no native sample.
 */
int
ipatch_sample_data_get_native_format (IpatchSampleData *sampledata)
{
  IpatchSampleStore *store;
  int format = 0;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), 0);

  IPATCH_ITEM_RLOCK (sampledata);
  if (sampledata->samples)
  {
    store = sampledata->samples->data;
    format = ipatch_sample_store_get_format (store);
  }
  IPATCH_ITEM_RUNLOCK (sampledata);

  return (format);
}

/**
 * ipatch_sample_data_open_native_sample:
 * @sampledata: Sample data
 * @handle: Caller supplied structure to initialize
 * @mode: Access mode to sample, 'r' for reading and 'w' for writing
 * @format: Sample format to convert to/from (0 for no conversion or to assign
 *   a transform object with ipatch_sample_handle_set_transform()).
 * @channel_map: Channel mapping if @format is set (set to 0 otherwise), use
 *   #IPATCH_SAMPLE_UNITY_CHANNEL_MAP for 1 to 1 channel mapping
 *   (see ipatch_sample_get_transform_funcs() for details).
 * @err: Location to store error information
 *
 * A convenience function to open a handle to a @sampledata object's native sample.
 * See ipatch_sample_handle_open() for more details.  This is identical to calling
 * ipatch_sample_data_get_native_sample() and then ipatch_sample_handle_open() on
 * the returned sample.
 *
 * Returns: %TRUE on success, %FALSE on failure (in which case @err may be set)
 */
gboolean
ipatch_sample_data_open_native_sample (IpatchSampleData *sampledata,
                                       IpatchSampleHandle *handle, char mode,
                                       int format, guint32 channel_map, GError **err)
{
  IpatchSampleStore *native_sample;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), FALSE);

  native_sample = ipatch_sample_data_get_native_sample (sampledata);
  g_return_val_if_fail (native_sample != NULL, FALSE);

  return (ipatch_sample_handle_open ((IpatchSample *)native_sample, handle, mode,
                                     format, channel_map, err));
}

/**
 * ipatch_sample_data_get_cache_sample:
 * @sampledata: Sample data object
 * @format: Sample format of cached sample to convert native sample to
 * @channel_map: Channel mapping to use for new cached sample when converting
 *   from native format, use #IPATCH_SAMPLE_UNITY_CHANNEL_MAP for 1 to 1 channel
 *   mapping (see ipatch_sample_get_transform_funcs() for details).
 * @err: Location to store error information
 *
 * Get a cached version, in RAM, of a sample.  If an existing cached sample
 * already exists with the given format and channel map, it is used.  Otherwise
 * a new #IpatchSampleStoreCache sample is created and the native sample is
 * converted as necessary.  If a matching cached sample is currently being
 * created by another thread, this function will block until it is created and
 * return it.
 *
 * Returns: Cached sample with the given @format for which the caller owns a
 *   reference or %NULL if @sampledata contains no samples or a sample
 *   conversion error occurred (I/O error for example).
 */
IpatchSampleStore *
ipatch_sample_data_get_cache_sample (IpatchSampleData *sampledata, int format,
                                     guint32 channel_map, GError **err)
{
  IpatchSampleStore *store;
  IpatchSample *c_sample;
  guint size_bytes;
  GSList *p, *prev = NULL;
  int src_format;
  guint32 maskval, src_channel_map;
  CachingInfo *cinfo;   /* Silence gcc (why?) */
  CachingInfo *new_cinfo = NULL;
  int i;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  /* Mask the channel_map by the number of channels in format */
  for (i = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format), maskval = 0; i > 0; i--)
    maskval |= 0x7 << ((i - 1) * 3);

  channel_map &= ~maskval;

try_again:

  IPATCH_ITEM_WLOCK (sampledata);

  for (p = sampledata->samples; p; p = p->next)
  {
    store = (IpatchSampleStore *)(p->data);
    if (!IPATCH_IS_SAMPLE_STORE_CACHE (store)) continue;

    src_format = ipatch_sample_store_get_format (store);
    src_channel_map
      = ipatch_sample_store_cache_get_channel_map ((IpatchSampleStoreCache *)store);

    if (format == src_format && channel_map == src_channel_map) break;
  }

  if (!p)
  {
    if (sampledata->samples)
      store = sampledata->samples->data;
    else store = NULL;
  }

  if (store) g_object_ref (store);    /* ++ ref the sample */

  IPATCH_ITEM_WUNLOCK (sampledata);


  /* Sample already cached or no samples in sample data object? - Return it or NULL */
  if (p || !store)
  {
    if (new_cinfo) g_slice_free (CachingInfo, new_cinfo);
    return (store);       /* !! caller takes over reference */
  }

  src_format = ipatch_sample_store_get_format (store);
  g_return_val_if_fail (ipatch_sample_format_transform_verify (src_format, format, channel_map), NULL);


  if (!new_cinfo) new_cinfo = g_slice_new (CachingInfo);

  /* Check if another thread is currently caching the same sample */
  g_mutex_lock (&caching_mutex);

  for (p = caching_list; p; p = p->next)
  {
    cinfo = p->data;

    if (cinfo->store == store && cinfo->format == format
        && cinfo->channel_map == channel_map)
      break;
  }

  if (p)        /* Matching cache operation in progress? */
  {
    g_cond_wait (&caching_cond, &caching_mutex);
    g_mutex_unlock (&caching_mutex);
    goto try_again;
  }

  /* No matching active cache in progress.. - Add it */
  new_cinfo->store = store;
  new_cinfo->format = format;
  new_cinfo->channel_map = channel_map;
  caching_list = g_slist_prepend (caching_list, new_cinfo);

  g_mutex_unlock (&caching_mutex);


  /* Cache the sample outside of lock */

  size_bytes = ipatch_sample_store_get_size (store) * ipatch_sample_format_size (format);

  /* Add to sample_cache_total_size and sample_cache_unused_size too.  Do this
   * before ipatch_sample_copy_below() since it modifies sample_cache_unused_size */
  G_LOCK (sample_cache_vars);
  sample_cache_total_size += size_bytes;
  sample_cache_unused_size += size_bytes;
  G_UNLOCK (sample_cache_vars);

  c_sample = ipatch_sample_store_cache_new (NULL);      /* ++ ref new sample */
  ipatch_sample_set_format (c_sample, format);
  ((IpatchSampleStoreCache *)c_sample)->channel_map = channel_map;

  if (!ipatch_sample_copy (c_sample, (IpatchSample *)store, channel_map, err))
  {
    g_object_unref (c_sample);          /* -- unref new sample */
    g_object_unref (store);             /* -- unref native sample */
    c_sample = NULL;
    goto caching_err;
  }

  g_object_unref (store);               /* -- unref the native sample */

  ipatch_sample_get_size (c_sample, &size_bytes);

  /* There is a chance that a sample could have been cached by another thread, but this
   * is unlikely and would just lead to a duplicate cached sample which would
   * eventually get removed.  For the sake of performance we leave out a check
   * for this. */

  g_object_ref (c_sample);      /* ++ ref sample for sampledata */

  /* IpatchSampleData not really a container, just set the store's parent directly */
  IPATCH_ITEM (store)->parent = IPATCH_ITEM (sampledata);

  IPATCH_ITEM_WLOCK (sampledata);
  sampledata->samples = g_slist_append (sampledata->samples, c_sample);
  IPATCH_ITEM_WUNLOCK (sampledata);

caching_err:    /* If caching operation fails, make sure we remove CachingInfo from list */

  g_mutex_lock (&caching_mutex);

  for (p = caching_list; p; prev = p, p = p->next)
  {
    cinfo = p->data;

    if (cinfo->store == store && cinfo->format == format
        && cinfo->channel_map == channel_map)
    {
      if (prev) prev->next = p->next;
      else caching_list = p->next;
      break;
    }
  }

  g_mutex_unlock (&caching_mutex);

  g_slice_free (CachingInfo, cinfo);
  g_slist_free1 (p);

  return ((IpatchSampleStore *)c_sample);      /* !! caller takes over reference */
}

/**
 * ipatch_sample_data_lookup_cache_sample:
 * @sampledata: Sample data object
 * @format: Sample format
 * @channel_map: Channel mapping of cached sample relative to native sample format.
 *
 * Like ipatch_sample_data_get_cache_sample() but does not create a new cache
 * sample if it doesn't exist.
 *
 * Returns: Cached sample store with the given @format and @channel_map for
 *   which the caller owns a reference or %NULL if @sampledata does not contain
 *   a matching cached sample.
 */
IpatchSampleStore *
ipatch_sample_data_lookup_cache_sample (IpatchSampleData *sampledata, int format,
                                        guint32 channel_map)
{
  IpatchSampleStore *store;
  int src_format;
  guint32 maskval, src_channel_map;
  GSList *p;
  int i;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), NULL);
  g_return_val_if_fail (ipatch_sample_format_verify (format), NULL);

  /* Mask the channel_map by the number of channels in format */
  for (i = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format), maskval = 0; i > 0; i--)
    maskval |= 0x7 << ((i - 1) * 3);

  channel_map &= ~maskval;

  IPATCH_ITEM_WLOCK (sampledata);

  for (p = sampledata->samples; p; p = p->next)
  {
    store = (IpatchSampleStore *)(p->data);
    if (!IPATCH_IS_SAMPLE_STORE_CACHE (store)) continue;

    src_format = ipatch_sample_store_get_format (store);
    src_channel_map
      = ipatch_sample_store_cache_get_channel_map ((IpatchSampleStoreCache *)store);

    if (format == src_format && channel_map == src_channel_map)
    {
      g_object_ref (store);    /* ++ ref sample */
      break;
    }
  }

  IPATCH_ITEM_WUNLOCK (sampledata);

  return (p ? store : NULL);   /* !! caller takes over reference */
}

/**
 * ipatch_sample_data_open_cache_sample:
 * @sampledata: Sample data object
 * @handle: Caller supplied sample handle to initialize
 * @format: Sample format
 * @err: Location to store error information
 *
 * Like ipatch_sample_data_get_cache_sample() but opens the resulting cached
 * sample as a convenience.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_sample_data_open_cache_sample (IpatchSampleData *sampledata,
                                      IpatchSampleHandle *handle, int format,
                                      guint32 channel_map, GError **err)
{
  IpatchSampleStore *store;
  gboolean retval;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), FALSE);
  g_return_val_if_fail (handle != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* ++ ref store */
  store = ipatch_sample_data_get_cache_sample (sampledata, format, channel_map, err);
  if (!store) return (FALSE);

  retval = ipatch_sample_handle_open (IPATCH_SAMPLE (store), handle, 'r', format,
                                      IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err);
  g_object_unref (store);     /* -- unref store */
  return (retval);
}

/**
 * ipatch_sample_data_cache_clean:
 * @max_unused_size: Maximum unused cached sample data size (0 to remove all unused samples)
 * @max_unused_age: Maximum age of unused cached samples in seconds (0 to disable
 *   time based removal), this is the age since they were last in an open state.
 *
 * Should be called periodically to release unused cached samples by size and/or
 * age criteria.
 */
void
ipatch_sample_cache_clean (guint64 max_unused_size, guint max_unused_age)
{
  IpatchSampleData *sampledata;
  IpatchSampleStoreCache *store;
  guint64 cur_unused_size;
  GSList *unused = NULL;
  GTimeVal time;
  glong last_open;
  GSList *p, *p2;

  if (max_unused_age != 0) g_get_current_time (&time);

  G_LOCK (sample_cache_vars);        /* ++ lock sample data list and variables */

  /* Optimize case of no unused samples or not removing by age and unused size does
   * not exceed max_unused_size */
  if (sample_cache_unused_size == 0
      || (max_unused_age == 0 && sample_cache_unused_size <= max_unused_size))
  {
    G_UNLOCK (sample_cache_vars);        /* -- unlock sample data list and variables */
    return;
  }

  G_UNLOCK (sample_cache_vars);        /* -- unlock sample data list and variables */


  G_LOCK (sample_data_list);            /* Lock sample data list */

  for (p = sample_data_list; p; p = p->next)
  {
    sampledata = (IpatchSampleData *)(p->data);

    IPATCH_ITEM_RLOCK (sampledata);

    for (p2 = sampledata->samples; p2; p2 = p2->next)
    {
      store = (IpatchSampleStoreCache *)(p2->data);

      if (IPATCH_IS_SAMPLE_STORE_CACHE (store)
          && ipatch_sample_store_cache_get_open_count (store) == 0)
      {
        unused = g_slist_prepend (unused, g_object_ref (store));        /* ++ ref store for list */
        g_object_ref (sampledata);      /* ++ ref sample data, to guarantee its existence out of lock */
      }
    }

    IPATCH_ITEM_RUNLOCK (sampledata);
  }

  G_UNLOCK (sample_data_list);        /* -- unlock sample data list */


  /* Sort list by last open age (oldest first) */
  unused = g_slist_sort (unused, sample_cache_clean_sort);

  /* Free samples until criteria no longer matches */
  for (p = unused; p; p = p->next)
  {
    store = (IpatchSampleStoreCache *)(p->data);

    IPATCH_ITEM_RLOCK (store);
    last_open = store->last_open;
    IPATCH_ITEM_RUNLOCK (store);

    sampledata = (IpatchSampleData *)(((IpatchItem *)store)->parent);

    if (last_open == 0)
    {
      g_object_unref (sampledata);
      g_object_unref (store);
      continue;       /* Store got opened since it was added to list? */
    }

    G_LOCK (sample_cache_vars);
    cur_unused_size = sample_cache_unused_size;
    G_UNLOCK (sample_cache_vars);

    /* Once size drops below max_unused_size and max_unused_age is 0 or this
     * sample was used more recent than max_unused_age - we're done */
    if (cur_unused_size <= max_unused_size
        && (max_unused_age == 0 || time.tv_sec - last_open <= max_unused_age))
      break;

    ipatch_sample_data_remove (sampledata, (IpatchSampleStore *)store);
    g_object_unref (sampledata);        /* -- unref sample data from list */
    g_object_unref (store);             /* -- unref sample store from list */
  }

  g_slist_free (unused);         /* -- free list */
}

/* Sort list of unused items by age (oldest to newest) */
static gint
sample_cache_clean_sort (gconstpointer a, gconstpointer b)
{
  const IpatchSampleStoreCache *astore = a, *bstore = b;
  glong alast_open, blast_open;

  IPATCH_ITEM_RLOCK (astore);
  alast_open = astore->last_open;
  IPATCH_ITEM_RUNLOCK (astore);

  IPATCH_ITEM_RLOCK (bstore);
  blast_open = bstore->last_open;
  IPATCH_ITEM_RUNLOCK (bstore);

  /* In case store got opened since add to list */
  if (alast_open == 0) return (1);
  if (blast_open == 0) return (-1);

  if (alast_open < blast_open) return (-1);
  if (alast_open > blast_open) return (1);
  return (0);
}

/**
 * ipatch_sample_data_get_blank:
 *
 * Get blank sample data object.  Return's a sample data structure
 * with the minimum amount of data which is blank. Only creates it on
 * the first call, subsequent calls return the same sample data
 * object. Therefore it should not be modified. The blank sample data's
 * reference count has been incremented and should be removed by the
 * caller with g_object_unref() when finished with it.
 *
 * Returns: The blank sample data object. Remember to unref it when not
 * using it anymore with g_object_unref().
 */
IpatchSampleData *
ipatch_sample_data_get_blank (void)
{
  static IpatchSampleData *blank_sampledata = NULL;
  IpatchSample *sample;

  if (!blank_sampledata) /* blank sampledata already created? */
    {
      blank_sampledata = ipatch_sample_data_new (); /* ++ ref new item */
      g_object_ref (blank_sampledata); /* ++ ref for static blank_sampledata */

      sample = ipatch_sample_store_ram_get_blank ();
      ipatch_sample_data_add (blank_sampledata, (IpatchSampleStore *)sample);
    }
  else g_object_ref (blank_sampledata);

  return (blank_sampledata);
}

/* Function used by IpatchSampleStoreCache.c */
void
_ipatch_sample_data_cache_add_unused_size (int size)
{
  G_LOCK (sample_cache_vars);
  sample_cache_unused_size += size;
  G_UNLOCK (sample_cache_vars);
}
