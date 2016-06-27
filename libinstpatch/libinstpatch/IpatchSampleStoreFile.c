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
 * SECTION: IpatchSampleStoreFile
 * @short_description: Sample store object type for audio in files on disk
 * @see_also: 
 * @stability: Stable
 */
#include <glib.h>
#include <glib-object.h>
#include <errno.h>
#include <string.h>

#include "IpatchSampleStoreFile.h"
#include "ipatch_priv.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_FILE,
  PROP_LOCATION
};

static void ipatch_sample_store_file_sample_iface_init (IpatchSampleIface *iface);
static void ipatch_sample_store_file_get_title (IpatchSampleStoreFile *store,
                                                GValue *value);
static void ipatch_sample_store_file_set_property
  (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_file_get_property
  (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_file_finalize (GObject *object);
static gboolean ipatch_sample_store_file_sample_iface_open
  (IpatchSampleHandle *handle, GError **err);
static void ipatch_sample_store_file_sample_iface_close (IpatchSampleHandle *handle);
static gboolean ipatch_sample_store_file_sample_iface_read
  (IpatchSampleHandle *handle, guint offset, guint frames, gpointer buf, GError **err);
static gboolean ipatch_sample_store_file_sample_iface_write
  (IpatchSampleHandle *handle, guint offset, guint frames, gconstpointer buf, GError **err);


G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreFile, ipatch_sample_store_file,
                         IPATCH_TYPE_SAMPLE_STORE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_file_sample_iface_init))

static void
ipatch_sample_store_file_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_file_sample_iface_open;
  iface->close = ipatch_sample_store_file_sample_iface_close;
  iface->read = ipatch_sample_store_file_sample_iface_read;
  iface->write = ipatch_sample_store_file_sample_iface_write;
}

static void
ipatch_sample_store_file_class_init (IpatchSampleStoreFileClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_sample_store_file_finalize;
  obj_class->get_property = ipatch_sample_store_file_get_property;
  item_class->item_set_property = ipatch_sample_store_file_set_property;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");
  g_object_class_install_property (obj_class, PROP_FILE,
      g_param_spec_object ("file", "File", "File object",
                           IPATCH_TYPE_FILE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (obj_class, PROP_LOCATION,
      g_param_spec_uint ("location", "Location", "Sample data file location",
                         0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ipatch_sample_store_file_get_title (IpatchSampleStoreFile *store, GValue *value)
{
  char *filename, *s, *basename = NULL;

  if (store->file)
  {
    filename = ipatch_file_get_name (store->file);      /* ++ alloc filename */

    if (filename)
    {
      basename = g_path_get_basename (filename);  /* ++ alloc basename */

      s = strrchr (basename, '.'); /* search for dot delimiter */
      if (s && s > basename) *s = '\0';	/* terminate string at dot */

      g_free (filename);          /* -- free filename */
    }
  }

  g_value_take_string (value, basename);        /* !! caller takes over alloc */
}

static void
ipatch_sample_store_file_set_property (GObject *object, guint property_id,
				       const GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreFile *store = IPATCH_SAMPLE_STORE_FILE (object);

  /* No lock required since they should be set only once before use */

  switch (property_id)
  {
    case PROP_FILE:
      g_return_if_fail (store->file == NULL);
      store->file = g_value_get_object (value);
      ipatch_file_ref_from_object (store->file, object);        // ++ ref file from store

      /* IpatchItem notify for "title" property */
      {
        GValue titleval = { 0 };
        g_value_init (&titleval, G_TYPE_STRING);
        ipatch_sample_store_file_get_title (store, &titleval);
        ipatch_item_prop_notify ((IpatchItem *)store, ipatch_item_pspec_title,
                                 &titleval, NULL);
        g_value_unset (&titleval);
      }
      break;
    case PROP_LOCATION:
      g_return_if_fail (store->location == 0);
      store->location = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ipatch_sample_store_file_get_property (GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreFile *store = IPATCH_SAMPLE_STORE_FILE (object);

  /* No lock required since they should be set only once before use */

  switch (property_id)
  {
    case PROP_TITLE:
      ipatch_sample_store_file_get_title (store, value);
      break;
    case PROP_FILE:
      g_value_set_object (value, store->file);
      break;
    case PROP_LOCATION:
      g_value_set_uint (value, store->location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ipatch_sample_store_file_init (IpatchSampleStoreFile *store)
{
}

static void
ipatch_sample_store_file_finalize (GObject *object)
{
  IpatchSampleStoreFile *store = IPATCH_SAMPLE_STORE_FILE (object);

  if (store->file) ipatch_file_unref_from_object (store->file, object);         // -- unref from object

  if (G_OBJECT_CLASS (ipatch_sample_store_file_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sample_store_file_parent_class)->finalize (object);
}

static gboolean
ipatch_sample_store_file_sample_iface_open (IpatchSampleHandle *handle,
                                            GError **err)
{
  IpatchSampleStoreFile *store = IPATCH_SAMPLE_STORE_FILE (handle->sample);

  g_return_val_if_fail (store->file != NULL, FALSE);

  /* No lock needed - file object set only once */
  handle->data1 = ipatch_file_open (store->file, NULL,
                                    handle->read_mode ? "r" : "w", err);
  if (!handle->data1)
    return (FALSE);

  handle->data2 = GUINT_TO_POINTER (ipatch_sample_format_size (ipatch_sample_store_get_format (store)));

  return (TRUE);
}

static void
ipatch_sample_store_file_sample_iface_close (IpatchSampleHandle *handle)
{
  if (handle->data1)
  {
    ipatch_file_close (handle->data1);
    handle->data1 = NULL;
  }
}

static gboolean
ipatch_sample_store_file_sample_iface_read (IpatchSampleHandle *handle,
                                            guint offset, guint frames,
                                            gpointer buf, GError **err)
{
  IpatchSampleStoreFile *store = (IpatchSampleStoreFile *)(handle->sample);
  IpatchFileHandle *fh = (IpatchFileHandle *)(handle->data1);
  guint8 frame_size = GPOINTER_TO_UINT (handle->data2);

  if (!ipatch_file_seek (fh, store->location + offset * frame_size, G_SEEK_SET, err))
    return (FALSE);

  if (!ipatch_file_read (fh, buf, frames * frame_size, err))
    return (FALSE);

  return (TRUE);
}

static gboolean
ipatch_sample_store_file_sample_iface_write (IpatchSampleHandle *handle,
                                             guint offset, guint frames,
                                             gconstpointer buf, GError **err)
{
  IpatchSampleStoreFile *store = (IpatchSampleStoreFile *)(handle->sample);
  IpatchFileHandle *fh = (IpatchFileHandle *)(handle->data1);
  guint8 frame_size = GPOINTER_TO_UINT (handle->data2);

  if (!ipatch_file_seek (fh, store->location + offset * frame_size, G_SEEK_SET, err))
    return (FALSE);

  if (!ipatch_file_write (fh, buf, frames * frame_size, err))
    return (FALSE);

  return (TRUE);
}

/**
 * ipatch_sample_store_file_new:
 * @file: File object to use for file sample store
 * @location: Location in file of audio data
 *
 * Creates a new file sample store.
 *
 * Returns: (type IpatchSampleStoreFile): New file sample store, cast
 *   as a #IpatchSample for convenience.
 */
IpatchSample *
ipatch_sample_store_file_new (IpatchFile *file, guint location)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_FILE,
                                       "file", file,
                                       "location", location,
                                       NULL)));
}
