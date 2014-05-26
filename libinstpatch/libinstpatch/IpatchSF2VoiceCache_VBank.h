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
#ifndef __IPATCH_SF2_VOICE_CACHE_VBANK_H__
#define __IPATCH_SF2_VOICE_CACHE_VBANK_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchConverter.h>

typedef IpatchConverter IpatchConverterVBankInstToSF2VoiceCache;
typedef IpatchConverterClass IpatchConverterVBankInstToSF2VoiceCacheClass;
typedef IpatchConverter IpatchConverterVBankRegionToSF2VoiceCache;
typedef IpatchConverterClass IpatchConverterVBankRegionToSF2VoiceCacheClass;


#define IPATCH_TYPE_CONVERTER_VBANK_INST_TO_SF2_VOICE_CACHE \
  (ipatch_converter_vbank_inst_to_sf2_voice_cache_get_type ())
#define IPATCH_TYPE_CONVERTER_VBANK_REGION_TO_SF2_VOICE_CACHE \
  (ipatch_converter_vbank_region_to_sf2_voice_cache_get_type ())


GType ipatch_converter_vbank_inst_to_sf2_voice_cache_get_type (void);
GType ipatch_converter_vbank_region_to_sf2_voice_cache_get_type (void);

#endif
