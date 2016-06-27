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
 * SECTION: IpatchSampleStoreRom
 * @short_description: Sample storage object for audio in ROM of a sound card
 * @see_also: 
 * @stability: Stable
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleStoreRom.h"
#include "ipatch_priv.h"

enum
{
  PROP_0,
  PROP_LOCATION,		/* location property (used by all types) */
};

static void ipatch_sample_store_rom_sample_iface_init (IpatchSampleIface *iface);
static void ipatch_sample_store_rom_set_property
  (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_rom_get_property
  (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean ipatch_sample_store_rom_sample_iface_open
  (IpatchSampleHandle *handle, GError **err);


G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreRom, ipatch_sample_store_rom,
                         IPATCH_TYPE_SAMPLE_STORE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_rom_sample_iface_init))

static void
ipatch_sample_store_rom_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_rom_sample_iface_open;
}

static void
ipatch_sample_store_rom_class_init (IpatchSampleStoreRomClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  gobj_class->get_property = ipatch_sample_store_rom_get_property;
  item_class->item_set_property = ipatch_sample_store_rom_set_property;

  g_object_class_install_property (gobj_class, PROP_LOCATION,
			g_param_spec_uint ("location",
					   "Location",
					   "Sample data ROM location",
					   0, G_MAXUINT, 0,
					   G_PARAM_READWRITE));
}

static void
ipatch_sample_store_rom_set_property (GObject *object, guint property_id,
			              const GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreRom *store = IPATCH_SAMPLE_STORE_ROM (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      g_return_if_fail (store->location == 0);

      /* Only set once, no lock required */
      store->location = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_sample_store_rom_get_property (GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreRom *store = IPATCH_SAMPLE_STORE_ROM (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      /* No need to lock, only set once before use */
      g_value_set_uint (value, store->location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_sample_store_rom_init (IpatchSampleStoreRom *store)
{
}

static gboolean
ipatch_sample_store_rom_sample_iface_open (IpatchSampleHandle *handle,
                                           GError **err)
{
  g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_PROGRAM,
                     "ROM sample stores cannot be opened");
  return (FALSE);
}

/**
 * ipatch_sample_store_rom_new:
 * @location: Location in ROM
 *
 * Creates a new rom sample store.  No data can actually be read or written
 * from this store type.  Its used only to keep track of ROM locations in older
 * SoundFont files.
 *
 * Returns: (type IpatchSampleStoreRom): New rom sample store, cast
 *   as an #IpatchSample for convenience.
 */
IpatchSample *
ipatch_sample_store_rom_new (guint location)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_ROM,
                                       "location", location, NULL)));
}
