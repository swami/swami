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
 * SECTION: IpatchSampleData
 * @short_description: Sample data proxy object.
 * @see_also: #IpatchSampleStore
 * @stability: Stable
 *
 * An object which acts as a proxy for sample data and one or more cached
 * versions of the same audio but potentially differing in sample format
 * and/or storage media.  #IpatchSampleStore is used instead of the
 * #IpatchSample interface, for increased performance.
 */
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleData.h"
#include "IpatchSampleStoreCache.h"
#include "IpatchSampleStoreRam.h"
#include "IpatchSampleStoreSwap.h"
#include "IpatchSample.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"
#include "config.h"

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
static void ipatch_sample_data_dispose (GObject *gobject);
static void ipatch_sample_data_release_store (IpatchSampleStore *store);
static gint sample_cache_clean_sort (gconstpointer a, gconstpointer b);


/* master sample data list and lock */
G_LOCK_DEFINE_STATIC (sample_data_list);
static GSList *sample_data_list = NULL;

/* Lock for metric variables below */
G_LOCK_DEFINE_STATIC (sample_cache_vars);
static guint64 sample_cache_total_size = 0;     /* Total size of cached samples */
static guint64 sample_cache_unused_size = 0;    /* Size of unused cached samples */

/* Variables used to ensure that duplicate sample caching does not occur */
static GMutex *caching_mutex;
static GCond *caching_cond;
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
 * Returns: (transfer full): New object list populated with all #IpatchSampleData objects
 * with a reference count of 1 which the caller owns, removing the reference
 * will free the list.
 *
 * Since: 1.1.0
 */
IpatchList *
ipatch_get_sample_data_list (void)
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

/* GHFunc for g_hash_table_foreach() to remove a store from its parent sample data */
static void
remove_stores (gpointer key, gpointer value, gpointer user_data)
{
  IpatchSampleStore *store = key;
  IpatchSampleData *sampledata;

  sampledata = (IpatchSampleData *)ipatch_item_get_parent ((IpatchItem *)store);  // ++ ref parent sample data
  ipatch_sample_data_remove (sampledata, store);
  g_object_unref (sampledata);          // -- unref parent sample data
}

/**
 * ipatch_migrate_file_sample_data:
 * @oldfile: (nullable): Old file to migrate samples from (can be %NULL, but not both @oldfile and @newfile)
 * @newfile: (nullable): New file which has stores which may be used for migration
 *   (can be %NULL, but not both @oldfile and @newfile)
 * @filename: (nullable): File name used for replace if @oldfile is %NULL and
 *   #IPATCH_SAMPLE_DATA_MIGRATE_REPLACE is set in @flags
 * @flags: (type IpatchSampleDataMigrateFlags): Flag options for migration
 * @err: Location to store error info or %NULL to ignore
 *
 * Used for migrating sample data when saving, deleting, replacing or closing instrument files.
 *
 * If @oldfile is set, sample data will be migrated for those that have native sample references to it, and the
 * old sample stores are removed.
 *
 * When saving a file, @newfile can be set.  In this case new #IpatchSampleStore objects should have already been
 * added to their applicable #IpatchSampleData objects.  #IpatchSampleData objects will be migrated to these stores
 * if they match the native format and the criteria set by the @flags parameter.
 *
 * If sample data needs to be migrated but there is no format identical store from @newfile, then
 * a new duplicate #IpatchSampleStoreSwap will be created and set as the new native sample.
 *
 * If the #IPATCH_SAMPLE_DATA_MIGRATE_REMOVE_NEW_IF_UNUSED flag is set in @flags, then unused #IpatchSampleStore
 * objects referencing @newfile will be removed if unused.
 *
 * The #IPATCH_SAMPLE_DATA_MIGRATE_TO_NEWFILE flag can be used to migrate all native compatible samples to @newfile.
 * Default is to only migrate samples which reference @oldfile or swap.
 *
 * The #IPATCH_SAMPLE_DATA_MIGRATE_LEAVE_IN_SWAP flag can be used to not migrate samples out of swap.  The default
 * is to migrate samples out of swap to @newfile if possible.
 *
 * The #IPATCH_SAMPLE_DATA_MIGRATE_REPLACE flag can be used to replace @oldfile with @newfile using
 * ipatch_file_replace().  If @oldfile is not set, @filename can be used to replace a file, which should not
 * be referenced by any other #IpatchFile object.  If neither of these is set, then this flag is ignored.
 *
 * NOTE - Not really thread safe.  It is assumed that sample stores referencing @oldfile or @newfile
 * will not be added or removed (respectively) by other threads during this function.  The side
 * effect of this would potentially be added samples still referencing @oldfile or removed samples
 * being re-added.
 *
 * Returns: %TRUE on succcess, %FALSE otherwise (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gboolean
ipatch_migrate_file_sample_data (IpatchFile *oldfile, IpatchFile *newfile, const char *filename,
                                 guint flags, GError **err)
{
  IpatchSampleData *sampledata;
  IpatchSampleStore *old_store, *store, *native_store, *new_store;
  IpatchList *old_list, *store_list;
  GHashTable *replace_hash;     // Hash of new stores to use to replace native stores
  GHashTable *remove_hash;      // Hash of new stores to remove
  GHashTable *swap_hash;        // Hash of created swap stores to replace native stores
  GHashTableIter iter;
  gpointer key, value;
  GList *p, *p2;
  int native_fmt, new_fmt;
  int sample_rate;
  gboolean is_swap_store;

  g_return_val_if_fail (!oldfile || IPATCH_IS_FILE (oldfile), FALSE);
  g_return_val_if_fail (!newfile || IPATCH_IS_FILE (newfile), FALSE);
  g_return_val_if_fail (oldfile || newfile, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  replace_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);
  remove_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);
  swap_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);

  if (oldfile)
  {
    old_list = ipatch_file_get_refs_by_type (oldfile, IPATCH_TYPE_SAMPLE_STORE);  // ++ ref list of stores referencing oldfile
    p = old_list->items;
  }
  else p = NULL;

  // Loop over sample stores referencing old file
  for (; p; p = p->next)
  {
    old_store = (IpatchSampleStore *)(p->data);
    sampledata = (IpatchSampleData *)ipatch_item_get_parent ((IpatchItem *)old_store);  // ++ ref parent sample data
    if (!sampledata) continue;          // Probably shouldn't happen, right?

    store_list = ipatch_sample_data_get_samples (sampledata);   // ++ ref sample data store list

    if (!store_list || !store_list->items)      // Shouldn't happen, but..
    {
      if (store_list) g_object_unref (store_list);      // -- unref store list
      g_object_unref (sampledata);                      // -- unref sample data
      continue;
    }

    native_store = (IpatchSampleStore *)(store_list->items->data);      // Native sample store
    new_store = NULL;

    if (newfile)      // New file provided?
    {
      native_fmt = ipatch_sample_store_get_format (native_store);

      // Search for sample store referencing new file
      for (p2 = store_list->items->next; p2; p2 = p2->next)
      {
        store = (IpatchSampleStore *)(p2->data);

        if (ipatch_file_test_ref_object (newfile, (GObject *)store))
        {
          new_fmt = ipatch_sample_store_get_format (store);
          break;
        }
      }

      if (p2) new_store = store;
    }

    // Should sample be migrated?
    if (native_store == old_store                               // If native store is in old file, must be migrated
        || ((new_store && new_fmt == native_fmt)                // If there is a new store of compatible format..
            && ((flags & IPATCH_SAMPLE_DATA_MIGRATE_TO_NEWFILE) // .. and to newfile flag specified
                || (!(flags & IPATCH_SAMPLE_DATA_MIGRATE_LEAVE_IN_SWAP)
                    && IPATCH_IS_SAMPLE_STORE_SWAP (native_store)))))   // .. or native store is in swap and not leave in swap
    { // If no store in newfile or incompatible format - migrate to swap
      if (!new_store || new_fmt != native_fmt)
      { // Add new store to remove list if requested
        if (new_store && (flags & IPATCH_SAMPLE_DATA_MIGRATE_REMOVE_NEW_IF_UNUSED))
          g_hash_table_insert (remove_hash, g_object_ref (new_store), GUINT_TO_POINTER (1));    // ++ ref for remove hash

        g_object_get (old_store, "sample-rate", &sample_rate, NULL);

        store = (IpatchSampleStore *)ipatch_sample_store_swap_new ();   // ++ ref new swap sample store
        g_object_set (store, "sample-rate", sample_rate, NULL);
        ipatch_sample_data_add (sampledata, store);

        if (!ipatch_sample_copy ((IpatchSample *)store, (IpatchSample *)old_store,
                                  IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err))
        {
          g_object_unref (store);                               // -- unref swap sample store
          g_object_unref (sampledata);                          // -- unref sample data
          g_hash_table_destroy (replace_hash);                  // -- Free replace hash
          g_hash_table_destroy (remove_hash);                   // -- Free remove hash
          g_hash_table_foreach (swap_hash, remove_stores, NULL);        // -- Remove swap stores
          g_hash_table_destroy (swap_hash);                     // -- Free swap hash
          g_object_unref (store_list);                          // -- unref sample data store list

          if (oldfile) g_object_unref (old_list);               // -- unref store list
          return (FALSE);
        }

        g_hash_table_insert (swap_hash, store, GUINT_TO_POINTER (1));   // !! hash takes over swap reference
      } // Compatible store in newfile - add to replace list
      else g_hash_table_insert (replace_hash, g_object_ref (new_store), GUINT_TO_POINTER (1));  // ++ ref for replace hash
    } // Migration not necessary for this sample - remove new store if present and REMOVE_NEW_IF_UNUSED requested
    else if (new_store && (flags & IPATCH_SAMPLE_DATA_MIGRATE_REMOVE_NEW_IF_UNUSED))
      g_hash_table_insert (remove_hash, g_object_ref (new_store), GUINT_TO_POINTER (1));        // ++ ref new store for remove hash

    g_object_unref (store_list);        // -- unref sample data store list
    g_object_unref (sampledata);        // -- unref sample data
  }

  if (oldfile) g_object_unref (old_list);       // -- unref store list

  // Replace oldfile/filename with newfile if requested
  if ((flags & IPATCH_SAMPLE_DATA_MIGRATE_REPLACE) && newfile)
  {
    if ((oldfile && !ipatch_file_replace (newfile, oldfile, err))
        || (!oldfile && filename && !ipatch_file_rename (newfile, filename, err)))
    {
      g_hash_table_destroy (replace_hash);              // -- Free replace hash
      g_hash_table_destroy (remove_hash);               // -- Free remove hash
      g_hash_table_foreach (swap_hash, remove_stores, NULL);    // -- Remove swap stores
      g_hash_table_destroy (swap_hash);                 // -- Free swap hash
      return (FALSE);
    }
  }

  // Process new stores if newfile specified
  if (newfile)
  {
    store_list = ipatch_file_get_refs_by_type (newfile, IPATCH_TYPE_SAMPLE_STORE);  // ++ ref list of stores referencing newfile

    for (p = store_list->items; p; p = p->next)
    {
      store = (IpatchSampleStore *)(p->data);
      if (g_hash_table_lookup (replace_hash, store)) continue;          // Already in replace hash? - skip

      // If TO_NEWFILE or not LEAVE_IN_SWAP and is in swap, check if sample can be migrated to this store
      if ((flags & IPATCH_SAMPLE_DATA_MIGRATE_TO_NEWFILE) || !(flags & IPATCH_SAMPLE_DATA_MIGRATE_LEAVE_IN_SWAP))
      {
        sampledata = (IpatchSampleData *)ipatch_item_get_parent ((IpatchItem *)store);  // ++ ref parent sample data
        if (!sampledata) continue;              // Probably shouldn't happen, right?

        native_store = ipatch_sample_data_get_native_sample (sampledata);               // ++ ref native sample store
        g_object_unref (sampledata);            // -- unref sample data

        if (native_store)
        {
          native_fmt = ipatch_sample_store_get_format (native_store);
          is_swap_store = IPATCH_IS_SAMPLE_STORE_SWAP (native_store);
          g_object_unref (native_store);        // -- unref native store (only need the pointer value from here on)

          if (store == native_store) continue;  // already the native store? - skip

          // Is format compatible and TO_NEWFILE flag set or native store is swap and LEAVE_IN_SWAP flag not set
          if (native_fmt == ipatch_sample_store_get_format (store)
              && ((flags & IPATCH_SAMPLE_DATA_MIGRATE_TO_NEWFILE)
                  || (is_swap_store && !(flags & IPATCH_SAMPLE_DATA_MIGRATE_LEAVE_IN_SWAP))))
          {
            g_hash_table_insert (replace_hash, g_object_ref (store), GUINT_TO_POINTER (1));       // ++ ref store for replace hash
            continue;
          }
        }       // if native_store
      } // if TO_NEWFILE or !LEAVE_IN_SWAP

      if (flags & IPATCH_SAMPLE_DATA_MIGRATE_REMOVE_NEW_IF_UNUSED)
        g_hash_table_insert (remove_hash, g_object_ref (store), GUINT_TO_POINTER (1));    // ++ ref store for remove hash
    }

    g_object_unref (store_list);                // -- unref newfile stores
  }

  // Replace native stores with those in replace hash (already added to sample data objects)
  for (g_hash_table_iter_init (&iter, replace_hash); g_hash_table_iter_next (&iter, &key, &value); )
  {
    store = (IpatchSampleStore *)key;
    sampledata = (IpatchSampleData *)ipatch_item_get_parent ((IpatchItem *)store);      // ++ ref parent sample data
    ipatch_sample_data_replace_native_sample (sampledata, store);
    g_object_unref (sampledata);                // -- unref sample data
  }

  // Replace native stores with those in swap hash (already added to sample data objects)
  for (g_hash_table_iter_init (&iter, swap_hash); g_hash_table_iter_next (&iter, &key, &value); )
  {
    store = (IpatchSampleStore *)key;
    sampledata = (IpatchSampleData *)ipatch_item_get_parent ((IpatchItem *)store);      // ++ ref parent sample data
    ipatch_sample_data_replace_native_sample (sampledata, store);
    g_object_unref (sampledata);                // -- unref sample data
  }

  // Remove stores in remove hash
  for (g_hash_table_iter_init (&iter, remove_hash); g_hash_table_iter_next (&iter, &key, &value); )
  {
    store = (IpatchSampleStore *)key;
    sampledata = (IpatchSampleData *)ipatch_item_get_parent ((IpatchItem *)store);      // ++ ref parent sample data
    ipatch_sample_data_remove (sampledata, store);
    g_object_unref (sampledata);                // -- unref sample data
  }

  g_hash_table_destroy (replace_hash);              // -- Free replace hash
  g_hash_table_destroy (remove_hash);               // -- Free remove hash
  g_hash_table_destroy (swap_hash);                 // -- Free swap hash

  return (TRUE);
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

  obj_class->dispose = ipatch_sample_data_dispose;
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

  caching_mutex = g_mutex_new ();
  caching_cond = g_cond_new ();
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
  g_atomic_int_set (&sampledata->usecount, 0);  // Initial use count is 0
}

static void
ipatch_sample_data_dispose (GObject *gobject)
{
  IpatchSampleData *sampledata = IPATCH_SAMPLE_DATA (gobject);
  GSList *p;

  IPATCH_ITEM_WLOCK (sampledata);

  for (p = sampledata->samples; p; p = g_slist_delete_link (p, p))
    ipatch_sample_data_release_store (IPATCH_SAMPLE_STORE (p->data));

  sampledata->samples = NULL;

  IPATCH_ITEM_WUNLOCK (sampledata);


  if (G_OBJECT_CLASS (ipatch_sample_data_parent_class)->dispose)
    G_OBJECT_CLASS (ipatch_sample_data_parent_class)->dispose (gobject);
}

/**
 * ipatch_sample_data_new:
 *
 * Create a new sample data object.
 *
 * Returns: New sample data with a reference count of 1 which the caller owns.
 * Use count should be incremented when added to an active instrument sample
 * using ipatch_sample_data_used() which will add it to the pool of sample data objects.
 */
IpatchSampleData *
ipatch_sample_data_new (void)
{
  return (IPATCH_SAMPLE_DATA (g_object_new (IPATCH_TYPE_SAMPLE_DATA, NULL)));
}

/**
 * ipatch_sample_data_used:
 * @sampledata: Sample data object
 *
 * Increment use count of a sample data object.  Use references are not the same as object references and
 * in contrast indicate if a sample data object should continue to be a part of the sample data pool - if it is
 * a dependency of an active loaded instrument for example.
 *
 * Since: 1.1.0
 */
void 
ipatch_sample_data_used (IpatchSampleData *sampledata)
{
  g_return_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata));

  if (g_atomic_int_exchange_and_add (&sampledata->usecount, 1) == 0)
  { // add to the master list
    G_LOCK (sample_data_list);
    sample_data_list = g_slist_prepend (sample_data_list, sampledata);
    G_UNLOCK (sample_data_list);
  }
}

/**
 * ipatch_sample_data_unused:
 * @sampledata: Sample data object
 *
 * Decrement use count of a sample data object.  When the use count drops to 0 it will be removed from the
 * pool of active sample data objects.  Sample data object is only freed after it's reference count goes to 0 though.
 *
 * Since: 1.1.0
 */
void
ipatch_sample_data_unused (IpatchSampleData *sampledata)
{
  g_return_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata));

  if (g_atomic_int_dec_and_test (&sampledata->usecount))
  { // remove from master list
    G_LOCK (sample_data_list);
    sample_data_list = g_slist_remove (sample_data_list, sampledata);
    G_UNLOCK (sample_data_list);
  }
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
  IPATCH_ITEM_WLOCK (store);
  IPATCH_ITEM (store)->parent = IPATCH_ITEM (sampledata);
  IPATCH_ITEM_WUNLOCK (store);

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
    ipatch_sample_data_release_store (store);   // -- release store
    g_slist_free_1 (p);
  }
}

/* Release a sample store, by clearing its parent sample data pointer,
 * updating cache metrics (if its an #IpatchSampleStoreCache) and unrefing it */
static void
ipatch_sample_data_release_store (IpatchSampleStore *store)
{
  guint size_bytes;

  if (IPATCH_IS_SAMPLE_STORE_CACHE (store))
  {
    size_bytes = ipatch_sample_store_get_size_bytes (store);

    IPATCH_ITEM_RLOCK (store);        /* ++ lock store */

    /* Recursive lock: store, sample_cache_vars */
    G_LOCK (sample_cache_vars);

    sample_cache_total_size -= size_bytes;

    /* Only subtract unused size from total unused size, if no opens active */
    if (((IpatchSampleStoreCache *)store)->open_count == 0)
      sample_cache_unused_size -= size_bytes;

    G_UNLOCK (sample_cache_vars);

    IPATCH_ITEM_RUNLOCK (store);      /* -- unlock store */
  }

  /* IpatchSampleData not really a container, just unset the store's parent directly */
  IPATCH_ITEM_WLOCK (store);
  IPATCH_ITEM (store)->parent = NULL;
  IPATCH_ITEM_WUNLOCK (store);

  g_object_unref (store);     /* -- unref sample store */
}

/**
 * ipatch_sample_data_replace_native_sample:
 * @sampledata: Sample data object
 * @store: Sample store object
 *
 * Replace the native sample of a sample data object.  This function can be used
 * even if there are no samples yet, in which case it behaves just like
 * ipatch_sample_data_add().
 *
 * The @store object can already be added to @sampledata, does nothing if already native sample
 * (libInstPatch version 1.1.0+).
 */
void
ipatch_sample_data_replace_native_sample (IpatchSampleData *sampledata,
                                          IpatchSampleStore *store)
{
  IpatchSampleStore *oldsample = NULL;
  IpatchItem *store_item = (IpatchItem *)store;
  IpatchItem *sampledata_item = (IpatchItem *)sampledata;
  gboolean already_added = FALSE;
  GSList *p, *prev, *oldlistitem = NULL;

  g_return_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata));
  g_return_if_fail (IPATCH_IS_SAMPLE_STORE (store));

  /* IpatchSampleData not really a container, just set the store's parent directly */
  IPATCH_ITEM_WLOCK (store);

  if (log_if_fail (!store_item->parent || store_item->parent == sampledata_item))
  {
    IPATCH_ITEM_WUNLOCK (store);
    return;
  }

  already_added = store_item->parent == sampledata_item;
  store_item->parent = (IpatchItem *)sampledata;

  IPATCH_ITEM_WUNLOCK (store);


  IPATCH_ITEM_WLOCK (sampledata);

  if (already_added)
  {
    for (p = sampledata->samples, prev = NULL; p; prev = p, p = p->next)
    {
      if (((IpatchSampleStore *)(p->data)) == store)
      {
        if (p == sampledata->samples)       // Do nothing if sample store is already the native sample
        {
          IPATCH_ITEM_WUNLOCK (sampledata);
          return;
        }

        oldlistitem = p;
        prev->next = p->next;
      }
    }
  }

  if (sampledata->samples)
  {
    oldsample = sampledata->samples->data;
    sampledata->samples->data = store;
  }
  else sampledata->samples = g_slist_prepend (sampledata->samples, store);

  IPATCH_ITEM_WUNLOCK (sampledata);


  // Only ref store if not already added to sampledata
  if (!oldlistitem)
    g_object_ref (store);        /* ++ ref sample for sampledata */

  if (oldsample) ipatch_sample_data_release_store (oldsample);  // -- release store
  if (oldlistitem) g_slist_free_1 (oldlistitem);
}

/**
 * ipatch_sample_data_get_samples:
 * @sampledata: Sample data object
 *
 * Get an object list of samples in a sample data object.  The first sample is
 * the native sample.
 *
 * Returns: (transfer full): Newly created list of #IpatchSampleStore objects with a refcount of
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
 * Returns: (transfer full): Native sample, or %NULL if no native sample in the sample data object,
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
 * @handle: (out): Caller supplied structure to initialize
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
  gboolean retval;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), FALSE);

  native_sample = ipatch_sample_data_get_native_sample (sampledata);    // ++ ref sample store
  g_return_val_if_fail (native_sample != NULL, FALSE);

  retval = ipatch_sample_handle_open ((IpatchSample *)native_sample, handle, mode,
                                      format, channel_map, err);
  g_object_unref (native_sample);               // -- unref sample store

  return (retval);
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
 * Returns: (transfer full): Cached sample with the given @format for which the caller owns a
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
  int rate;
  int i;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  /* Mask the channel_map by the number of channels in format */
  for (i = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format), maskval = 0; i > 0; i--)
    maskval |= 0x7 << ((i - 1) * 3);

  channel_map &= maskval;

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
  g_mutex_lock (caching_mutex);

  for (p = caching_list; p; p = p->next)
  {
    cinfo = p->data;

    if (cinfo->store == store && cinfo->format == format
        && cinfo->channel_map == channel_map)
      break;
  }

  if (p)        /* Matching cache operation in progress? */
  {
    g_cond_wait (caching_cond, caching_mutex);
    g_mutex_unlock (caching_mutex);
    goto try_again;
  }

  /* No matching active cache in progress.. - Add it */
  new_cinfo->store = store;
  new_cinfo->format = format;
  new_cinfo->channel_map = channel_map;
  caching_list = g_slist_prepend (caching_list, new_cinfo);

  g_mutex_unlock (caching_mutex);


  /* Cache the sample outside of lock */

  g_object_get (store,
                "sample-size", &size_bytes,
                "sample-rate", &rate,
                NULL);

  size_bytes *= ipatch_sample_format_size (format);

  /* Add to sample_cache_total_size and sample_cache_unused_size too.  Do this
   * before ipatch_sample_copy_below() since it modifies sample_cache_unused_size */
  G_LOCK (sample_cache_vars);
  sample_cache_total_size += size_bytes;
  sample_cache_unused_size += size_bytes;
  G_UNLOCK (sample_cache_vars);

  c_sample = ipatch_sample_store_cache_new (NULL);      /* ++ ref new sample */

  g_object_set (c_sample, "sample-format", format,
                "sample-rate", rate,
                NULL);

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

  g_mutex_lock (caching_mutex);

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

  g_mutex_unlock (caching_mutex);

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
 * Returns: (transfer full): Cached sample store with the given @format and @channel_map for
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
 * ipatch_sample_cache_get_unused_size:
 *
 * Get the current unused sample cache data size.
 *
 * Returns: Total amount of unused sample cache data in RAM.
 *
 * Since: 1.1.0
 */
guint64
ipatch_sample_cache_get_unused_size (void)
{
  guint64 cur_unused_size;

  G_LOCK (sample_cache_vars);           /* ++ lock sample data list and variables */
  cur_unused_size = sample_cache_unused_size;
  G_UNLOCK (sample_cache_vars);         /* -- unlock sample data list and variables */

  return (cur_unused_size);
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
 * Returns: (transfer full): The blank sample data object. Remember to unref it when not
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
      ipatch_sample_data_used (blank_sampledata);   // ++ inc use count for static blank_sampledata

      sample = ipatch_sample_store_ram_get_blank ();
      ipatch_sample_data_add (blank_sampledata, (IpatchSampleStore *)sample);
    }
  else g_object_ref (blank_sampledata);

  return (blank_sampledata);
}

/**
 * _ipatch_sample_data_cache_add_unused_size: (skip)
 *
 * Function used by IpatchSampleStoreCache.c
 */
void
_ipatch_sample_data_cache_add_unused_size (int size)
{
  G_LOCK (sample_cache_vars);
  sample_cache_unused_size += size;
  G_UNLOCK (sample_cache_vars);
}

#ifdef IPATCH_DEBUG
/**
 * ipatch_sample_data_dump: (skip)
 *
 * Dump sample data information to stdout for debugging purposes.
 */
void
ipatch_sample_data_dump (void)
{
  IpatchList *data_list;
  IpatchList *store_list;
  IpatchSampleStore *store;
  GList *pdata, *pstore;

  data_list = ipatch_get_sample_data_list ();           // ++ ref sample data list

  for (pdata = data_list->items; pdata; pdata = pdata->next)
  {
    store_list = ipatch_sample_data_get_samples (IPATCH_SAMPLE_DATA (pdata->data));     // ++ ref stores list

    printf ("Data %p: samples=%d\n", pdata->data, g_list_length (store_list->items));

    for (pstore = store_list->items; pstore; pstore = pstore->next)
    {
      int sample_format, sample_rate, loop_type, root_note, fine_tune;
      guint sample_size, loop_start, loop_end;
      char *title;

      store = IPATCH_SAMPLE_STORE (pstore->data);

      g_object_get (store,
                    "title", &title,
                    "sample-size", &sample_size,        // ++ allocate title
                    "sample-format", &sample_format,
                    "sample-rate", &sample_rate,
                    "loop-type", &loop_type,
                    "loop-start", &loop_start,
                    "loop-end", &loop_end,
                    "root-note", &root_note,
                    "fine-tune", &fine_tune,
                    NULL);

      printf ("  Store %p (%s): title='%s' size=%u fmt=0x%X rate=%d ltype=%d lstart=%u lend=%u root=%d fine=%d\n",
              store, g_type_name (G_OBJECT_TYPE (store)), title, sample_size, sample_format,
              sample_rate, loop_type, loop_start, loop_end, root_note, fine_tune);

      g_free (title);                                   // -- free title

      // Show some details about file stores
      if (IPATCH_IS_SAMPLE_STORE_FILE (store))
      {
        IpatchFile *file;
        char *filename = NULL;
        guint location;

        g_object_get (store, "file", &file, "location", &location, NULL);        // ++ ref file object

        if (file) filename = ipatch_file_get_name (file);                       // ++ alloc file name

        printf ("    File %p (%s): filename='%s' location=%u\n", file, g_type_name (G_OBJECT_TYPE (file)),
                filename, location);

        g_free (filename);                                                      // -- free file name
        g_object_unref (file);                                                  // -- unref file
      } // Show details about swap stores
      else if (IPATCH_IS_SAMPLE_STORE_SWAP (store))
      {
        IpatchSampleStoreSwap *swap_store = (IpatchSampleStoreSwap *)store;

        if (swap_store->ram_location)
          printf ("    RAM %p\n", swap_store->ram_location);
        else printf ("    Swap %u\n", swap_store->location);
      }
    }

    g_object_unref (store_list);                        // -- unref stores list
  }

  g_object_unref (data_list);                           // -- unref sample data list
}
#endif

