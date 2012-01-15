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
/**
 * SECTION: IpatchConvert_SF2
 * @short_description: SoundFont conversion handlers
 * @see_also: #IpatchConverter
 * @stability: Stable
 *
 * Conversion handlers for SoundFont objects.
 */
#ifndef __IPATCH_CONVERT_SF2_H__
#define __IPATCH_CONVERT_SF2_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchConverter.h>

typedef IpatchConverter IpatchConverterSF2ToFile;
typedef IpatchConverterClass IpatchConverterSF2ToFileClass;
typedef IpatchConverter IpatchConverterFileToSF2;
typedef IpatchConverterClass IpatchConverterFileToSF2Class;
typedef IpatchConverter IpatchConverterFileToSF2Sample;
typedef IpatchConverterClass IpatchConverterFileToSF2SampleClass;

#if 0
typedef IpatchConverter IpatchConverterSF2ToDLS2;
typedef IpatchConverterClass IpatchConverterSF2ToDLS2Class;
typedef IpatchConverter IpatchConverterDLS2ToSF2;
typedef IpatchConverterClass IpatchConverterDLS2ToSF2Class;
typedef IpatchConverter IpatchConverterSF2PresetToDLS2Inst;
typedef IpatchConverterClass IpatchConverterSF2PresetToDLS2InstClass;
typedef IpatchConverter IpatchConverterDLS2InstToSF2Preset;
typedef IpatchConverterClass IpatchConverterDLS2InstToSF2PresetClass;
typedef IpatchConverter IpatchConverterSF2InstToDLS2Inst;
typedef IpatchConverterClass IpatchConverterSF2InstToDLS2InstClass;
typedef IpatchConverter IpatchConverterDLS2InstToSF2Inst;
typedef IpatchConverterClass IpatchConverterDLS2InstToSF2InstClass;
typedef IpatchConverter IpatchConverterSF2PZoneToDLS2Region;
typedef IpatchConverterClass IpatchConverterSF2PZoneToDLS2RegionClass;
typedef IpatchConverter IpatchConverterDLS2RegionToSF2PZone;
typedef IpatchConverterClass IpatchConverterDLS2RegionToSF2PZoneClass;
typedef IpatchConverter IpatchConverterSF2IZoneToDLS2Region;
typedef IpatchConverterClass IpatchConverterSF2IZoneToDLS2RegionClass;
typedef IpatchConverter IpatchConverterDLS2RegionToSF2IZone;
typedef IpatchConverterClass IpatchConverterDLS2RegionToSF2IZoneClass;
typedef IpatchConverter IpatchConverterSF2SampleToDLS2Sample;
typedef IpatchConverterClass IpatchConverterSF2SampleToDLS2SampleClass;
typedef IpatchConverter IpatchConverterDLS2SampleToSF2Sample;
typedef IpatchConverterClass IpatchConverterDLS2SampleToSF2SampleClass;
#endif


#define IPATCH_TYPE_CONVERTER_SF2_TO_FILE \
  (ipatch_converter_sf2_to_file_get_type ())
#define IPATCH_TYPE_CONVERTER_FILE_TO_SF2 \
  (ipatch_converter_file_to_sf2_get_type ())
#define IPATCH_TYPE_CONVERTER_FILE_TO_SF2_SAMPLE \
  (ipatch_converter_file_to_sf2_sample_get_type ())

#if 0
#define IPATCH_TYPE_CONVERTER_SF2_TO_DLS2 \
  (ipatch_converter_sf2_to_dls2_get_type ())
#define IPATCH_TYPE_CONVERTER_DLS2_TO_SF2 \
  (ipatch_converter_dls2_to_sf2_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_PRESET_TO_DLS2_INST \
  (ipatch_converter_sf2_preset_to_dls2_inst_get_type ())
#define IPATCH_TYPE_CONVERTER_DLS2_INST_TO_SF2_PRESET \
  (ipatch_converter_dls2_inst_to_sf2_preset_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_INST_TO_DLS2_INST \
  (ipatch_converter_sf2_inst_to_dls2_inst_get_type ())
#define IPATCH_TYPE_CONVERTER_DLS2_INST_TO_SF2_INST \
  (ipatch_converter_dls2_inst_to_sf2_inst_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_PZONE_TO_DLS2_REGION \
  (ipatch_converter_sf2_pzone_to_dls2_region_get_type ())
#define IPATCH_TYPE_CONVERTER_DLS2_REGION_TO_SF2_PZONE \
  (ipatch_converter_dls2_region_to_sf2_pzone_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_IZONE_TO_DLS2_REGION \
  (ipatch_converter_sf2_izone_to_dls2_region_get_type ())
#define IPATCH_TYPE_CONVERTER_DLS2_REGION_TO_SF2_IZONE \
  (ipatch_converter_dls2_region_to_sf2_izone_get_type ())
#define IPATCH_TYPE_CONVERTER_SF2_SAMPLE_TO_DLS2_SAMPLE \
  (ipatch_converter_sf2_sample_to_dls2_sample_get_type ())
#define IPATCH_TYPE_CONVERTER_DLS2_SAMPLE_TO_SF2_SAMPLE \
  (ipatch_converter_dls2_sample_to_sf2_sample_get_type ())
#endif


GType ipatch_converter_sf2_to_file_get_type (void);
GType ipatch_converter_file_to_sf2_get_type (void);
GType ipatch_converter_file_to_sf2_sample_get_type (void);

#if 0
GType ipatch_converter_sf2_to_dls2_get_type (void);
GType ipatch_converter_dls2_to_sf2_get_type (void);
GType ipatch_converter_sf2_preset_to_dls2_inst_get_type (void);
GType ipatch_converter_dls2_inst_to_sf2_preset_get_type (void);
GType ipatch_converter_sf2_inst_to_dls2_inst_get_type (void);
GType ipatch_converter_dls2_inst_to_sf2_inst_get_type (void);
GType ipatch_converter_sf2_pzone_to_dls2_region_get_type (void);
GType ipatch_converter_dls2_region_to_sf2_pzone_get_type (void);
GType ipatch_converter_sf2_izone_to_dls2_region_get_type (void);
GType ipatch_converter_dls2_region_to_sf2_izone_get_type (void);
GType ipatch_converter_sf2_sample_to_dls2_sample_get_type (void);
GType ipatch_converter_dls2_sample_to_sf2_sample_get_type (void);
#endif

#endif

