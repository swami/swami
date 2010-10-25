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
#include <unistd.h>
#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleStoreSwap.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* Indicates if swap file sample location has been set yet */
#define LOCATION_SET  (1 << IPATCH_SAMPLE_STORE_FILE_UNUSED_FLAG_SHIFT)

static void ipatch_sample_store_swap_sample_iface_init (IpatchSampleIface *iface);
static gboolean ipatch_sample_store_swap_sample_iface_open (IpatchSampleHandle *handle,
                                                            GError **err);
static void ipatch_sample_store_swap_finalize (GObject *gobject);


G_LOCK_DEFINE_STATIC (swap);
static int swap_fd = -1;
static char *swap_file_name = NULL;
static guint swap_position = 0;             /* Current position in swap file, for new sample data */
static volatile gint swap_unused_size = 0;  /* Amount of wasted space (unused samples) */

G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreSwap, ipatch_sample_store_swap,
                         IPATCH_TYPE_SAMPLE_STORE_FILE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_swap_sample_iface_init))

static void
ipatch_sample_store_swap_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_swap_sample_iface_open;
}

static void
ipatch_sample_store_swap_class_init (IpatchSampleStoreSwapClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->finalize = ipatch_sample_store_swap_finalize;
}

static gboolean
ipatch_sample_store_swap_sample_iface_open (IpatchSampleHandle *handle,
                                            GError **err)
{
  IpatchSampleStoreSwap *store = IPATCH_SAMPLE_STORE_SWAP (handle->sample);
  GError *local_err = NULL;
  guint sample_size;
  int location = -1;
  guint flags;

  ipatch_sample_get_size (IPATCH_SAMPLE (store), &sample_size);
  flags = ipatch_item_get_flags (IPATCH_ITEM (store));

  G_LOCK (swap);

  if (swap_fd == -1)   /* Swap file not yet created? */
  {
    swap_fd = g_file_open_tmp ("libInstPatch_XXXXXX", &swap_file_name, &local_err);

    if (swap_fd == -1)
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                   _("Failed to create sample store swap file: %s"),
	           ipatch_gerror_message (local_err));
      g_clear_error (&local_err);
      G_UNLOCK (swap);
      return (FALSE);
    }
  }

  if (!(flags & LOCATION_SET))
  {
    location = swap_position;
    swap_position += sample_size;
  }

  G_UNLOCK (swap);


  /* Set location if it has not yet been set.
   * No lock required, since data should be written before use in MT environment. */
  if (location != -1)
  {
    g_object_set (store, "location", location, NULL);
    ipatch_item_set_flags (IPATCH_ITEM (store), LOCATION_SET);
  }

  handle->data1 = fopen (swap_file_name, handle->read_mode ? "rb" : "wb");

  if (!handle->data1)
  {
    if (handle->read_mode)
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                   _("Error opening file '%s' for reading: %s"),
                   swap_file_name, g_strerror (errno));
    else
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                   _("Error opening file '%s' for writing: %s"),
                   swap_file_name, g_strerror (errno));
    return (FALSE);
  }

  return (TRUE);
}

static void
ipatch_sample_store_swap_init (IpatchSampleStoreSwap *store)
{
}

/* finalization for a swap sample store, keeps track of unused data size */
static void
ipatch_sample_store_swap_finalize (GObject *gobject)
{
  guint size;

  ipatch_sample_get_size (IPATCH_SAMPLE (gobject), &size);
  g_atomic_int_add (&swap_unused_size, size);

  if (G_OBJECT_CLASS (ipatch_sample_store_swap_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sample_store_swap_parent_class)->finalize (gobject);
}

/**
 * ipatch_sample_store_swap_new:
 *
 * Creates a new disk swap sample store.
 *
 * Returns: New disk swap sample store, cast as an #IpatchSample for convenience.
 */
IpatchSample *
ipatch_sample_store_swap_new (void)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_SWAP, NULL)));
}

/**
 * ipatch_sample_store_swap_get_unused_size:
 *
 * Get amount of unused space in the swap file.
 *
 * Returns: Amount of unused data in bytes
 */
int
ipatch_sample_store_swap_get_unused_size (void)
{
  return (g_atomic_int_get (&swap_unused_size));
}
