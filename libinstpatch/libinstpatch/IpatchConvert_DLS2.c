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
 * SECTION: IpatchConvert_DLS2
 * @short_description: DLS conversion types
 * @see_also: #IpatchConverter
 * @stability: Stable
 *
 * DLS object converter types.
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "misc.h"
#include "IpatchConvert_DLS2.h"
#include "IpatchConverter.h"
#include "IpatchConverter_priv.h"
#include "IpatchDLSFile.h"
#include "IpatchDLS2.h"
#include "IpatchDLSReader.h"
#include "IpatchDLSWriter.h"
#include "IpatchSampleData.h"
#include "IpatchSampleStoreSndFile.h"
#include "IpatchSample.h"
#include "IpatchBase.h"
#include "IpatchFile.h"
#include "IpatchSndFile.h"
#include "IpatchDLSWriter.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_DLS2_TO_FILE_CREATE_STORES
};

static void _dls2_to_file_get_property (GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec);
static void _dls2_to_file_set_property (GObject *object, guint property_id,
                                        const GValue *value, GParamSpec *pspec);

/**
 * _ipatch_convert_DLS2_init: (skip)
 *
 * Init routine for DLS conversion types
 */
void
_ipatch_convert_DLS2_init (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_DLS2_TO_FILE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_FILE_TO_DLS2);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_FILE_TO_DLS2_SAMPLE);

  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_DLS2_TO_FILE, 0, 0,
				 IPATCH_TYPE_DLS2, 0, 1,
				 IPATCH_TYPE_DLS_FILE, IPATCH_TYPE_FILE, 1);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_FILE_TO_DLS2, 0, 0,
				 IPATCH_TYPE_DLS_FILE, 0, 1,
				 IPATCH_TYPE_DLS2, IPATCH_TYPE_BASE, 0);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_FILE_TO_DLS2_SAMPLE, 0, 0,
				 IPATCH_TYPE_SND_FILE, 0, 1,
				 IPATCH_TYPE_DLS2_SAMPLE, 0, 1);
}

static gboolean
_dls2_to_file_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2 *dls;
  IpatchFile *file;
  IpatchFileHandle *handle;
  IpatchDLSWriter *writer;
  gboolean create_stores;
  IpatchList *stores;
  int retval;

  dls = IPATCH_DLS2 (IPATCH_CONVERTER_INPUT (converter));
  file = IPATCH_FILE (IPATCH_CONVERTER_OUTPUT (converter));

  handle = ipatch_file_open (IPATCH_FILE (file), NULL, "w", err);
  if (!handle) return (FALSE);

  writer = ipatch_dls_writer_new (handle, dls); /* ++ ref new writer */
  retval = ipatch_dls_writer_save (writer, err);

  g_object_get (converter, "create-stores", &create_stores, NULL);

  if (retval && create_stores)
  {
    stores = ipatch_dls_writer_create_stores (writer);         // ++ reference sample stores

    if (stores)
    {
      ipatch_converter_add_output (converter, G_OBJECT (stores));
      g_object_unref (stores);                                  // -- unref sample stores
    }
  }

  g_object_unref (writer);	/* -- unref writer */

  return (retval);
}

static gboolean
_file_to_dls2_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2 *dls;
  IpatchDLSFile *file;
  IpatchFileHandle *handle;
  IpatchDLSReader *reader;

  file = IPATCH_DLS_FILE (IPATCH_CONVERTER_INPUT (converter));

  handle = ipatch_file_open (IPATCH_FILE (file), NULL, "r", err);
  if (!handle) return (FALSE);

  reader = ipatch_dls_reader_new (handle); /* ++ ref new reader */
  dls = ipatch_dls_reader_load (reader, err); /* ++ ref loaded DLS object */
  g_object_unref (reader);	/* -- unref reader */

  if (dls)
    {
      ipatch_converter_add_output (converter, G_OBJECT (dls));
      g_object_unref (dls);	/* -- unref loaded DLS object */
      return (TRUE);
    }
  else return (FALSE);
}

/**
 * _file_to_dls2_sample_convert: (skip)
 *
 * Also used by IpatchConvert_Gig.c
 */
gboolean
_file_to_dls2_sample_convert (IpatchConverter *converter, GError **err)
{
  IpatchSndFile *file;
  IpatchDLS2Sample *dls2sample;
  IpatchSampleStoreSndFile *store;
  IpatchSampleData *sampledata;
  int format, rate, loop_type, root_note, fine_tune;
  guint length, loop_start, loop_end;
  char *filename, *title;

  file = IPATCH_SND_FILE (IPATCH_CONVERTER_INPUT (converter));
  dls2sample = IPATCH_DLS2_SAMPLE (IPATCH_CONVERTER_OUTPUT (converter));

  filename = ipatch_file_get_name (IPATCH_FILE (file));       /* ++ alloc file name */

  if (!filename)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_PROGRAM,
                 _("Sample file object must have a file name"));
    return (FALSE);
  }

  store = IPATCH_SAMPLE_STORE_SND_FILE (ipatch_sample_store_snd_file_new (filename));  /* ++ ref store */

  if (!ipatch_sample_store_snd_file_init_read (store))
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNSUPPORTED,
                 _("Sample file '%s' is invalid or unsupported"), filename);
    g_object_unref (store);     /* -- unref store */
    g_free (filename);    /* -- free filename */
    return (FALSE);
  }

  g_free (filename);    /* -- free filename */

  g_object_get (store,
		"title", &title,		/* ++ alloc title */
		"sample-size", &length,
		"sample-format", &format,
		"sample-rate", &rate,
		"loop-type", &loop_type,
		"loop-start", &loop_start,
		"loop-end", &loop_end,
		"root-note", &root_note,
		"fine-tune", &fine_tune,
		NULL);

  if (length < 4)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
                 _("Sample '%s' is too small"),
                 title ? title : _("<no name>"));
    g_free (title);	/* -- free title */
    g_object_unref (store);     /* -- unref store */
    return (FALSE);
  }

  sampledata = ipatch_sample_data_new ();       /* ++ ref sampledata */
  ipatch_sample_data_add (sampledata, IPATCH_SAMPLE_STORE (store));
  g_object_unref (store);     /* -- unref store */

  g_object_set (dls2sample,
		"name", title,
                "sample-data", sampledata,
		"sample-rate", rate,
		"root-note", (root_note != -1) ? root_note : 60,
		"fine-tune", fine_tune,
		"loop-start", loop_start,
		"loop-end", loop_end,
		NULL);

  g_object_unref (sampledata);  /* -- unref sampledata */
  g_free (title);	/* -- free title */

  return (TRUE);
}

/* DLS2 -> File class is handled manually to install properties */
static void
_dls2_to_file_class_init (IpatchConverterClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->get_property = _dls2_to_file_get_property;
  obj_class->set_property = _dls2_to_file_set_property;

  klass->verify = NULL;
  klass->notes = NULL;
  klass->convert = _dls2_to_file_convert;

  g_object_class_install_property (obj_class, PROP_DLS2_TO_FILE_CREATE_STORES,
                    g_param_spec_boolean ("create-stores", "Create stores", "Create sample stores",
                                          FALSE, G_PARAM_READWRITE));
}

static void
_dls2_to_file_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  IpatchConverterDLS2ToFile *converter = (IpatchConverterDLS2ToFile *)object;

  switch (property_id)
  {
    case PROP_DLS2_TO_FILE_CREATE_STORES:
      g_value_set_boolean (value, converter->create_stores);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
_dls2_to_file_set_property (GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  IpatchConverterDLS2ToFile *converter = (IpatchConverterDLS2ToFile *)object;

  switch (property_id)
  {
    case PROP_DLS2_TO_FILE_CREATE_STORES:
      converter->create_stores = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

CONVERTER_CLASS_INIT (file_to_dls2);
CONVERTER_CLASS_INIT(file_to_dls2_sample);

CONVERTER_GET_TYPE (dls2_to_file, DLS2ToFile);
CONVERTER_GET_TYPE (file_to_dls2, FileToDLS2);
CONVERTER_GET_TYPE (file_to_dls2_sample, FileToDLS2Sample);
