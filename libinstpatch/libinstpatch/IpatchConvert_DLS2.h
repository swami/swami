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
#ifndef __IPATCH_CONVERT_DLS2_H__
#define __IPATCH_CONVERT_DLS2_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchConverter.h>

typedef struct
{
  IpatchConverter parent_instance;

  /*< private >*/
  gboolean create_stores;
} IpatchConverterDLS2ToFile;

typedef IpatchConverterClass IpatchConverterDLS2ToFileClass;
typedef IpatchConverter IpatchConverterFileToDLS2;
typedef IpatchConverterClass IpatchConverterFileToDLS2Class;
typedef IpatchConverter IpatchConverterFileToDLS2Sample;
typedef IpatchConverterClass IpatchConverterFileToDLS2SampleClass;


#define IPATCH_TYPE_CONVERTER_DLS2_TO_FILE \
  (ipatch_converter_dls2_to_file_get_type ())
#define IPATCH_TYPE_CONVERTER_FILE_TO_DLS2 \
  (ipatch_converter_file_to_dls2_get_type ())
#define IPATCH_TYPE_CONVERTER_FILE_TO_DLS2_SAMPLE \
  (ipatch_converter_file_to_dls2_sample_get_type ())


GType ipatch_converter_dls2_to_file_get_type (void);
GType ipatch_converter_file_to_dls2_get_type (void);
GType ipatch_converter_file_to_dls2_sample_get_type (void);

#endif
