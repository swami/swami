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
#ifndef __IPATCH_SF2_VOICE_CACHE_DLS_H__
#define __IPATCH_SF2_VOICE_CACHE_DLS_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchConverter.h>
#include <libinstpatch/IpatchConverterSF2VoiceCache.h>

typedef IpatchConverterSF2VoiceCache IpatchConverterDLS2InstToSF2VoiceCache;
typedef IpatchConverterSF2VoiceCacheClass IpatchConverterDLS2InstToSF2VoiceCacheClass;
typedef IpatchConverterSF2VoiceCache IpatchConverterDLS2RegionToSF2VoiceCache;
typedef IpatchConverterSF2VoiceCacheClass IpatchConverterDLS2RegionToSF2VoiceCacheClass;
typedef IpatchConverterSF2VoiceCache IpatchConverterDLS2SampleToSF2VoiceCache;
typedef IpatchConverterSF2VoiceCacheClass IpatchConverterDLS2SampleToSF2VoiceCacheClass;

#define IPATCH_TYPE_CONVERTER_DLS2_INST_TO_SF2_VOICE_CACHE \
  (ipatch_converter_dls2_inst_to_sf2_voice_cache_get_type ())
#define IPATCH_TYPE_CONVERTER_DLS2_REGION_TO_SF2_VOICE_CACHE \
  (ipatch_converter_dls2_region_to_sf2_voice_cache_get_type ())
#define IPATCH_TYPE_CONVERTER_DLS2_SAMPLE_TO_SF2_VOICE_CACHE \
  (ipatch_converter_dls2_sample_to_sf2_voice_cache_get_type ())

GType ipatch_converter_dls2_inst_to_sf2_voice_cache_get_type (void);
GType ipatch_converter_dls2_region_to_sf2_voice_cache_get_type (void);
GType ipatch_converter_dls2_sample_to_sf2_voice_cache_get_type (void);

#endif
