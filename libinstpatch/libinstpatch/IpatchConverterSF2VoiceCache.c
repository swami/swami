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
 * SECTION: IpatchConverterSF2VoiceCache
 * @short_description: Base object type used for #IpatchSF2Voice cache converters
 * @see_also: 
 * @stability: Stable
 *
 * Defines a base type which other #IpatchSF2Voice converter types are derived
 * from.  Defines some properties like "solo-item" for solo-ing a sub
 * component of an instrument.
 */
#include "IpatchConverterSF2VoiceCache.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_SOLO_ITEM
};

static void
ipatch_converter_sf2_voice_cache_set_property (GObject *object, guint property_id,
                                               const GValue *value, GParamSpec *pspec);
static void
ipatch_converter_sf2_voice_cache_get_property (GObject *object, guint property_id,
                                               GValue *value, GParamSpec *pspec);


G_DEFINE_ABSTRACT_TYPE (IpatchConverterSF2VoiceCache, ipatch_converter_sf2_voice_cache,
                        IPATCH_TYPE_CONVERTER)

static void
ipatch_converter_sf2_voice_cache_class_init (IpatchConverterSF2VoiceCacheClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  obj_class->set_property = ipatch_converter_sf2_voice_cache_set_property;
  obj_class->get_property = ipatch_converter_sf2_voice_cache_get_property;

  g_object_class_install_property (obj_class, PROP_SOLO_ITEM,
                g_param_spec_object ("solo-item", _("Solo item"), _("Solo item"),
                                     IPATCH_TYPE_ITEM, G_PARAM_READWRITE));
}

static void
ipatch_converter_sf2_voice_cache_init (IpatchConverterSF2VoiceCache *converter)
{
}

static void
ipatch_converter_sf2_voice_cache_set_property (GObject *object, guint property_id,
                                               const GValue *value, GParamSpec *pspec)
{
  IpatchConverterSF2VoiceCache *converter = IPATCH_CONVERTER_SF2_VOICE_CACHE (object);

  /* No lock needed, since converters aren't multi-threaded */

  switch (property_id)
  {
    case PROP_SOLO_ITEM:
      if (converter->solo_item) g_object_unref (converter->solo_item);
      converter->solo_item = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ipatch_converter_sf2_voice_cache_get_property (GObject *object, guint property_id,
                                               GValue *value, GParamSpec *pspec)
{
  IpatchConverterSF2VoiceCache *converter = IPATCH_CONVERTER_SF2_VOICE_CACHE (object);

  /* No lock needed, since converters aren't multi-threaded */

  switch (property_id)
  {
    case PROP_SOLO_ITEM:
      g_value_set_object (value, converter->solo_item);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}
