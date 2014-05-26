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
#ifndef __IPATCH_CONVERTER_SF2_VOICE_CACHE_H__
#define __IPATCH_CONVERTER_SF2_VOICE_CACHE_H__

typedef struct _IpatchConverterSF2VoiceCache IpatchConverterSF2VoiceCache;
typedef struct _IpatchConverterSF2VoiceCacheClass IpatchConverterSF2VoiceCacheClass;

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchConverter.h>
#include <libinstpatch/IpatchItem.h>

#define IPATCH_TYPE_CONVERTER_SF2_VOICE_CACHE  \
  (ipatch_converter_sf2_voice_cache_get_type ())
#define IPATCH_CONVERTER_SF2_VOICE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_CONVERTER_SF2_VOICE_CACHE, \
  IpatchConverterSF2VoiceCache))
#define IPATCH_CONVERTER_SF2_VOICE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_CONVERTER_SF2_VOICE_CACHE, \
  IpatchSF2VoiceCacheClass))
#define IPATCH_IS_CONVERTER_SF2_VOICE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_CONVERTER_SF2_VOICE_CACHE))
#define IPATCH_IS_CONVERTER_SF2_VOICE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_CONVERTER_SF2_VOICE_CACHE))


struct _IpatchConverterSF2VoiceCache
{
  IpatchConverter parent_instance;
  IpatchItem *solo_item;
};

struct _IpatchConverterSF2VoiceCacheClass
{
  IpatchConverterClass parent_class;
};

GType ipatch_converter_sf2_voice_cache_get_type (void);

#endif
