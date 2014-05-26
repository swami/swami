/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Author of this file: (C) 2012 BALATON Zoltan <balaton@eik.bme.hu>
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
#ifndef __IPATCH_CONVERT_SLI_H__
#define __IPATCH_CONVERT_SLI_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchConverter.h>

typedef struct
{
  IpatchConverter parent_instance;

  /*< private >*/
  gboolean create_stores;
} IpatchConverterSLIToFile;

typedef IpatchConverterClass IpatchConverterSLIToFileClass;
typedef IpatchConverter IpatchConverterFileToSLI;
typedef IpatchConverterClass IpatchConverterFileToSLIClass;
typedef IpatchConverter IpatchConverterFileToSLISample;
typedef IpatchConverterClass IpatchConverterFileToSLISampleClass;


#define IPATCH_TYPE_CONVERTER_SLI_TO_FILE \
  (ipatch_converter_sli_to_file_get_type ())
#define IPATCH_TYPE_CONVERTER_FILE_TO_SLI \
  (ipatch_converter_file_to_sli_get_type ())
#define IPATCH_TYPE_CONVERTER_FILE_TO_SLI_SAMPLE \
  (ipatch_converter_file_to_sli_sample_get_type ())

GType ipatch_converter_sli_to_file_get_type (void);
GType ipatch_converter_file_to_sli_get_type (void);
GType ipatch_converter_file_to_sli_sample_get_type (void);

#endif

