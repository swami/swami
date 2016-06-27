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
 * SECTION: IpatchSampleStore
 * @short_description: Abstract sample storage object
 * @see_also: 
 * @stability: Stable
 *
 * Sample stores provide for various storage methods for audio data.
 * Examples include: #IpatchSampleStoreFile for audio data stored in files
 * on disk, #IpatchSampleStoreRAM for audio in RAM, #IpatchSampleStoreROM for
 * samples in ROM of a sound card, etc.
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSampleStore.h"
#include "builtin_enums.h"
#include "ipatch_priv.h"

enum
{
  PROP_0,
  PROP_SAMPLE_SIZE,		/* size of sample (in frames) */
  PROP_SAMPLE_FORMAT,		/* Sample format (Width | Channels | Signed | Endian) */
  PROP_SAMPLE_RATE,		/* sample rate hint */
  PROP_SAMPLE_DATA,             /* sample data pointer (not supported, just returns NULL) */

  /* read only dummy properties to conform to IpatchSample interface */

  PROP_LOOP_TYPE,
  PROP_LOOP_START,
  PROP_LOOP_END,
  PROP_ROOT_NOTE,
  PROP_FINE_TUNE
};

static void ipatch_sample_store_sample_iface_init (IpatchSampleIface *iface);
static void ipatch_sample_store_set_property (GObject *object,
					      guint property_id,
					      const GValue *value,
					      GParamSpec *pspec);
static void ipatch_sample_store_get_property (GObject *object,
					      guint property_id,
					      GValue *value,
					      GParamSpec *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (IpatchSampleStore, ipatch_sample_store, IPATCH_TYPE_ITEM,
                                  G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                         ipatch_sample_store_sample_iface_init))

static void
ipatch_sample_store_sample_iface_init (IpatchSampleIface *iface)
{
}

static void
ipatch_sample_store_class_init (IpatchSampleStoreClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->get_property = ipatch_sample_store_get_property;

  item_class->item_set_property = ipatch_sample_store_set_property;

  ipatch_sample_install_property (obj_class, PROP_SAMPLE_SIZE, "sample-size");
  ipatch_sample_install_property (obj_class, PROP_SAMPLE_FORMAT, "sample-format");
  ipatch_sample_install_property (obj_class, PROP_SAMPLE_RATE, "sample-rate");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_DATA, "sample-data");

  /* these are read only properties and just return the default value */
  ipatch_sample_install_property_readonly (obj_class, PROP_LOOP_TYPE, "loop-type");
  ipatch_sample_install_property_readonly (obj_class, PROP_LOOP_START, "loop-start");
  ipatch_sample_install_property_readonly (obj_class, PROP_LOOP_END, "loop-end");
  ipatch_sample_install_property_readonly (obj_class, PROP_ROOT_NOTE, "root-note");
  ipatch_sample_install_property_readonly (obj_class, PROP_FINE_TUNE, "fine-tune");
}

static void
ipatch_sample_store_set_property (GObject *object, guint property_id,
				  const GValue *value, GParamSpec *pspec)
{
  IpatchSampleStore *store = IPATCH_SAMPLE_STORE (object);
  int format;

  switch (property_id)
    {
    case PROP_SAMPLE_SIZE:
      /* Lock not required, only set once during creation, prior to MT use */
      store->size = g_value_get_uint (value);
      break;
    case PROP_SAMPLE_FORMAT:
      format = g_value_get_int (value) << IPATCH_SAMPLE_STORE_FORMAT_SHIFT;

      /* Lock not required since this should only be done once, prior to MT use */
      ipatch_item_clear_flags (store, IPATCH_SAMPLE_FORMAT_MASK << IPATCH_SAMPLE_STORE_FORMAT_SHIFT);
      ipatch_item_set_flags (store, format);
      break;
    case PROP_SAMPLE_RATE:
      store->rate = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_sample_store_get_property (GObject *object, guint property_id,
				  GValue *value, GParamSpec *pspec)
{
  IpatchSampleStore *store = IPATCH_SAMPLE_STORE (object);

  switch (property_id)
    {
    case PROP_SAMPLE_SIZE:
      /* Lock not required since it should not change, once used in MT context */
      g_value_set_uint (value, store->size);
      break;
    case PROP_SAMPLE_FORMAT:
      /* Lock not required since format should never change, once used in MT context */
      g_value_set_int (value, ipatch_sample_store_get_format (store));
      break;
    case PROP_SAMPLE_RATE:
      g_value_set_int (value, store->rate);
      break;
    case PROP_SAMPLE_DATA:
      g_value_set_object (value, NULL);
      break;
    case PROP_LOOP_TYPE:
      g_value_set_enum (value, IPATCH_SAMPLE_LOOP_NONE);
      break;
    case PROP_LOOP_START:
      g_value_set_uint (value, 0);
      break;
    case PROP_LOOP_END:
      g_value_set_uint (value, 0);
      break;
    case PROP_ROOT_NOTE:
      g_value_set_int (value, IPATCH_SAMPLE_ROOT_NOTE_DEFAULT);
      break;
    case PROP_FINE_TUNE:
      g_value_set_int (value, 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_sample_store_init (IpatchSampleStore *store)
{
  /* initialize format to default - signed 16 bit mono little endian data */
  ipatch_item_set_flags (store, IPATCH_SAMPLE_FORMAT_DEFAULT
                         << IPATCH_SAMPLE_STORE_FORMAT_SHIFT);

  store->rate = IPATCH_SAMPLE_RATE_DEFAULT;
}

/**
 * ipatch_sample_store_first: (skip)
 * @iter: Patch item iterator containing #IpatchSampleStore items
 *
 * Gets the first item in a sample store iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first sample store in @iter or %NULL if empty.
 */
IpatchSampleStore *
ipatch_sample_store_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_SAMPLE_STORE (obj));
  else return (NULL);
}

/**
 * ipatch_sample_store_next: (skip)
 * @iter: Patch item iterator containing #IpatchSampleStore items
 *
 * Gets the next item in a sample store iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next sample store in @iter or %NULL if at the end of the list.
 */
IpatchSampleStore *
ipatch_sample_store_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_SAMPLE_STORE (obj));
  else return (NULL);
}
