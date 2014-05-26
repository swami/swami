/*
 * IpatchSF2VoiceCache_SF2.h - Voice cache converters for SoundFont types
 *
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
 *
 */
#ifndef __IPATCH_SF2_VOICE_CACHE_SF2_H__
#define __IPATCH_SF2_VOICE_CACHE_SF2_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchConverterSF2VoiceCache.h>

typedef IpatchConverterSF2VoiceCache IpatchConverterSF2PresetToSF2VoiceCache;
typedef IpatchConverterSF2VoiceCacheClass IpatchConverterSF2PresetToSF2VoiceCacheClass;
typedef IpatchConverterSF2VoiceCache IpatchConverterSF2PZoneToSF2VoiceCache;
typedef IpatchConverterSF2VoiceCacheClass IpatchConverterSF2PZoneToSF2VoiceCacheClass;
typedef IpatchConverterSF2VoiceCache IpatchConverterSF2InstToSF2VoiceCache;
typedef IpatchConverterSF2VoiceCacheClass IpatchConverterSF2InstToSF2VoiceCacheClass;
typedef IpatchConverterSF2VoiceCache IpatchConverterSF2IZoneToSF2VoiceCache;
typedef IpatchConverterSF2VoiceCacheClass IpatchConverterSF2IZoneToSF2VoiceCacheClass;
typedef IpatchConverterSF2VoiceCache IpatchConverterSF2SampleToSF2VoiceCache;
typedef IpatchConverterSF2VoiceCacheClass IpatchConverterSF2SampleToSF2VoiceCacheClass;


#define IPATCH_TYPE_CONVERTER_SF2_PRESET_TO_SF2_VOICE_CACHE \
  (ipatch_converter_sf2_preset_to_sf2_voice_cache_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_PZONE_TO_SF2_VOICE_CACHE \
  (ipatch_converter_sf2_pzone_to_sf2_voice_cache_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_INST_TO_SF2_VOICE_CACHE \
  (ipatch_converter_sf2_inst_to_sf2_voice_cache_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_IZONE_TO_SF2_VOICE_CACHE \
  (ipatch_converter_sf2_izone_to_sf2_voice_cache_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_SAMPLE_TO_SF2_VOICE_CACHE \
  (ipatch_converter_sf2_sample_to_sf2_voice_cache_get_type ())


GType ipatch_converter_sf2_preset_to_sf2_voice_cache_get_type (void);
GType ipatch_converter_sf2_pzone_to_sf2_voice_cache_get_type (void);
GType ipatch_converter_sf2_inst_to_sf2_voice_cache_get_type (void);
GType ipatch_converter_sf2_izone_to_sf2_voice_cache_get_type (void);
GType ipatch_converter_sf2_sample_to_sf2_voice_cache_get_type (void);

#endif
