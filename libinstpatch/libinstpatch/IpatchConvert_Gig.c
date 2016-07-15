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
 * SECTION: IpatchConvert_Gig
 * @short_description: GigaSampler conversion handlers
 * @see_also: #IpatchConverter
 * @stability: Stable
 *
 * Conversion handlers for GigaSampler objects.
 */
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include "misc.h"

#include "IpatchConverter_priv.h"
#include "IpatchConvert_Gig.h"
#include "IpatchGig.h"
#include "IpatchGigFile.h"
#include "IpatchDLSReader.h"
#include "IpatchDLSWriter.h"
#include "IpatchBase.h"
#include "IpatchSampleData.h"
#include "IpatchSndFile.h"
#include "IpatchSampleStoreSndFile.h"
#include "i18n.h"

/*
 * GigaSampler conversion handlers (DLS is master format):
 * IpatchGig <==> IpatchGigFile
 * IpatchSampleFile => IpatchGigSample
 * IpatchGig <==> IpatchDLS2
 * IpatchGigInst <==> IpatchDLS2Inst
 * IpatchGigRegion <==> IpatchDLS2Region
 */

/* defined in IpatchConvert_DLS2.c */
gboolean _file_to_dls2_sample_convert (IpatchConverter *converter, GError **err);


/**
 * _ipatch_convert_gig_init: (skip)
 *
 * Init routine for GigaSampler conversion types
 */
void
_ipatch_convert_gig_init (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_GIG_TO_FILE);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_GIG_TO_FILE, 0, 0,
				 IPATCH_TYPE_GIG, 0, 1,
				 IPATCH_TYPE_GIG_FILE, IPATCH_TYPE_FILE, 1);

  g_type_class_ref (IPATCH_TYPE_CONVERTER_FILE_TO_GIG);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_FILE_TO_GIG, 0, 0,
				 IPATCH_TYPE_GIG_FILE, 0, 1,
				 IPATCH_TYPE_GIG, IPATCH_TYPE_BASE, 0);

  g_type_class_ref (IPATCH_TYPE_CONVERTER_FILE_TO_GIG_SAMPLE);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_FILE_TO_GIG_SAMPLE, 0, 0,
				 IPATCH_TYPE_SND_FILE, 0, 1,
				 IPATCH_TYPE_GIG_SAMPLE, 0, 1);
}

/* ===============
 * Convert methods
 * =============== */

static gboolean
_gig_to_file_convert (IpatchConverter *converter, GError **err)
{
  IpatchGig *gig;
  IpatchGigFile *file;
  IpatchFileHandle *handle;
  IpatchDLSWriter *writer;
  int retval;

  gig = IPATCH_GIG (IPATCH_CONVERTER_INPUT (converter));
  file = IPATCH_GIG_FILE (IPATCH_CONVERTER_OUTPUT (converter));

  handle = ipatch_file_open (IPATCH_FILE (file), NULL, "w", err);
  if (!handle) return (FALSE);

  /* ++ ref new writer */
  writer = ipatch_dls_writer_new (handle, IPATCH_DLS2 (gig));
  retval = ipatch_dls_writer_save (writer, err);
  g_object_unref (writer);	/* -- unref writer */

  return (retval);
}

static gboolean
_file_to_gig_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2 *dls;
  IpatchGigFile *file;
  IpatchFileHandle *handle;
  IpatchDLSReader *reader;

  file = IPATCH_GIG_FILE (IPATCH_CONVERTER_INPUT (converter));

  handle = ipatch_file_open (IPATCH_FILE (file), NULL, "r", err);
  if (!handle) return (FALSE);

  /* ++ ref new reader */
  reader = ipatch_dls_reader_new (handle);
  dls = ipatch_dls_reader_load (reader, err); /* ++ ref loaded DLS object */
  g_object_unref (reader);	/* -- unref reader */

  if (dls)
    {
      ipatch_converter_add_output (converter, G_OBJECT (dls));
      g_object_unref (dls);	/* -- unref loaded Gig object */
      return (TRUE);
    }
  else return (FALSE);
}

#define _file_to_gig_sample_convert _file_to_dls2_sample_convert;

CONVERTER_CLASS_INIT(gig_to_file);
CONVERTER_GET_TYPE(gig_to_file, GigToFile);

CONVERTER_CLASS_INIT(file_to_gig);
CONVERTER_GET_TYPE(file_to_gig, FileToGig);

CONVERTER_CLASS_INIT(file_to_gig_sample);
CONVERTER_GET_TYPE(file_to_gig_sample, FileToGigSample);
