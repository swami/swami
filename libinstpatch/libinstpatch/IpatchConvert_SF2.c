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
 * SECTION: IpatchConvert_SF2
 * @short_description: SoundFont conversion handlers
 * @see_also: #IpatchConverter
 * @stability: Stable
 *
 * Conversion handlers for SoundFont objects.
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "misc.h"
#include "IpatchConvert_SF2.h"
#include "IpatchConverter.h"
#include "IpatchConverter_priv.h"
#include "IpatchSndFile.h"
#include "IpatchSF2.h"
#include "IpatchSF2Gen.h"
#include "IpatchSF2Inst.h"
#include "IpatchSF2IZone.h"
#include "IpatchSF2Reader.h"
#include "IpatchSF2Mod.h"
#include "IpatchSF2Preset.h"
#include "IpatchSF2PZone.h"
#include "IpatchSF2Sample.h"
#include "IpatchSF2Writer.h"
#include "IpatchDLS2.h"
#include "IpatchDLS2Conn.h"
#include "IpatchDLS2Inst.h"
#include "IpatchDLS2Region.h"
#include "IpatchDLS2Sample.h"
#include "IpatchSample.h"
#include "IpatchSampleData.h"
#include "IpatchSampleStoreSndFile.h"
#include "IpatchSampleStoreVirtual.h"
#include "IpatchSampleList.h"
#include "IpatchBase.h"
#include "IpatchFile.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_SF2_TO_FILE_CREATE_STORES
};

/*
 * SoundFont conversion handlers
 * IpatchSF2 <==> IpatchSF2File
 * IpatchSndFile => IpatchSF2Sample
 */

static IpatchSF2Sample *create_sf2_sample_helper (IpatchSampleStoreSndFile *store,
                                                  gboolean left);
static void _sf2_to_file_get_property (GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec);
static void _sf2_to_file_set_property (GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec);

/**
 * _ipatch_convert_SF2_init: (skip)
 *
 * Init routine for SF2 conversion types
 */
void
_ipatch_convert_SF2_init (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SF2_TO_FILE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_FILE_TO_SF2);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_FILE_TO_SF2_SAMPLE);

  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_SF2_TO_FILE, 0, 0,
				 IPATCH_TYPE_SF2, 0, 1,
				 IPATCH_TYPE_SF2_FILE, IPATCH_TYPE_FILE, 1);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_FILE_TO_SF2, 0, 0,
				 IPATCH_TYPE_SF2_FILE, 0, 1,
				 IPATCH_TYPE_SF2, IPATCH_TYPE_BASE, 0);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_FILE_TO_SF2_SAMPLE, 0, 0,
				 IPATCH_TYPE_SND_FILE, 0, 1,
				 IPATCH_TYPE_SF2_SAMPLE, 0, 0);
}

/* ===============
 * Convert methods
 * =============== */

static gboolean
_sf2_to_file_convert (IpatchConverter *converter, GError **err)
{
  IpatchSF2 *sfont;
  IpatchSF2File *file;
  IpatchFileHandle *handle;
  IpatchSF2Writer *writer;
  gboolean create_stores;
  IpatchList *stores;
  int retval;

  sfont = IPATCH_SF2 (IPATCH_CONVERTER_INPUT (converter));
  file = IPATCH_SF2_FILE (IPATCH_CONVERTER_OUTPUT (converter));

  handle = ipatch_file_open (IPATCH_FILE (file), NULL, "w", err);
  if (!handle) return (FALSE);

  writer = ipatch_sf2_writer_new (handle, sfont);	/* ++ ref new writer */
  retval = ipatch_sf2_writer_save (writer, err);

  g_object_get (converter, "create-stores", &create_stores, NULL);

  if (retval && create_stores)
  {
    stores = ipatch_sf2_writer_create_stores (writer);     // ++ reference sample stores

    if (stores)
    {
      ipatch_converter_add_output (converter, G_OBJECT (stores));
      g_object_unref (stores);                          // -- unref sample stores
    }
  }

  g_object_unref (writer);	/* -- unref writer */

  return (retval);
}

static gboolean
_file_to_sf2_convert (IpatchConverter *converter, GError **err)
{
  IpatchSF2 *sfont;
  IpatchSF2File *file;
  IpatchFileHandle *handle;
  IpatchSF2Reader *reader;

  file = IPATCH_SF2_FILE (IPATCH_CONVERTER_INPUT (converter));

  handle = ipatch_file_open (IPATCH_FILE (file), NULL, "r", err);
  if (!handle) return (FALSE);

  reader = ipatch_sf2_reader_new (handle); /* ++ ref new reader */
  sfont = ipatch_sf2_reader_load (reader, err); /* ++ ref loaded SoundFont */
  g_object_unref (reader);	/* -- unref reader */

  if (sfont)
    {
      ipatch_converter_add_output (converter, G_OBJECT (sfont));
      g_object_unref (sfont);	/* -- unref loaded SoundFont */
      return (TRUE);
    }
  else return (FALSE);
}

/* Will produce 2 samples when stereo */
static gboolean
_file_to_sf2_sample_convert (IpatchConverter *converter, GError **err)
{
  IpatchSndFile *file;
  IpatchSF2Sample *sf2sample, *r_sf2sample;
  IpatchSampleStoreSndFile *store;
  char *filename, *title;
  guint length;
  int format;

  file = IPATCH_SND_FILE (IPATCH_CONVERTER_INPUT (converter));

  filename = ipatch_file_get_name (IPATCH_FILE (file));       /* ++ alloc file name */

  if (!filename)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_PROGRAM,
                 _("Sample file object must have a file name"));
    return (FALSE);
  }

  /* ++ ref store */
  store = IPATCH_SAMPLE_STORE_SND_FILE (ipatch_sample_store_snd_file_new (filename));

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
                "title", &title,                /* ++ alloc */
		"sample-size", &length,
		"sample-format", &format,
		NULL);

  if (length < 4)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
                 _("Sample '%s' is too small"),
                 title ? title : _("<no name>"));
    g_free (title);             /* -- free title */
    g_object_unref (store);     /* -- unref store */
    return (FALSE);
  }

  g_free (title);               /* -- free title */

  sf2sample = create_sf2_sample_helper (store, TRUE); /* ++ ref sf2sample */
  ipatch_converter_add_output (converter, (GObject *)sf2sample);

  /* Stereo? - Add the right channel too */
  if (IPATCH_SAMPLE_FORMAT_GET_CHANNELS (format) == IPATCH_SAMPLE_STEREO)
  {
    r_sf2sample = create_sf2_sample_helper (store, FALSE); /* ++ ref r_sf2sample */
    ipatch_converter_add_output (converter, (GObject *)r_sf2sample);

    g_object_set (sf2sample, "linked-sample", r_sf2sample, NULL);
    g_object_set (r_sf2sample, "linked-sample", sf2sample, NULL);

    g_object_unref (r_sf2sample);         /* -- unref r_sf2sample */
  }

  g_object_unref (sf2sample);         /* -- unref sf2sample */
  g_object_unref (store);     /* -- unref store */

  return (TRUE);
}

/* Helper function to create IpatchSF2Sample objects of mono or left/right
 * channels of stereo */
static IpatchSF2Sample *
create_sf2_sample_helper (IpatchSampleStoreSndFile *store, gboolean left)
{
  IpatchSampleList *samlist;
  IpatchSample *virtstore;
  IpatchSF2Sample *sf2sample;
  IpatchSampleData *sampledata;
  char newtitle[IPATCH_SFONT_NAME_SIZE + 1];
  int format, rate, loop_type, root_note, fine_tune;
  guint length, loop_start, loop_end;
  char *title;
  int channel = IPATCH_SF2_SAMPLE_CHANNEL_MONO;

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

  if (title && title[0] != '\0')
  {
    strncpy (newtitle, title, IPATCH_SFONT_NAME_SIZE);
    newtitle[IPATCH_SFONT_NAME_SIZE] = '\0';
  }
  else strcpy (newtitle, _("Untitled"));

  g_free (title);       /* -- free title */

  /* if loop not set, or messed up.. */
  if (loop_type == IPATCH_SAMPLE_LOOP_NONE
      || loop_end <= loop_start || loop_end > length)
  {
    if (length >= 48)
    {
      loop_start = 8;
      loop_end = length - 8;
    }
    else			/* sample is rather small */
    {
      loop_start = 1;
      loop_end = length - 1;
    }
  }

  if (IPATCH_SAMPLE_FORMAT_GET_CHANNELS (format) == IPATCH_SAMPLE_STEREO)
  {
    /* Create sample list containing single channel of stereo */
    samlist = ipatch_sample_list_new ();                /* ++ alloc samlist */
    ipatch_sample_list_append (samlist, (IpatchSample *)store, 0, length,
                               left ? IPATCH_SAMPLE_LEFT : IPATCH_SAMPLE_RIGHT);

    /* Create virtual store for mono audio and assign the sample list to it */
    virtstore = ipatch_sample_store_virtual_new ();     /* ++ ref virtstore */

    g_object_set (virtstore,
                  "sample-format", format & ~IPATCH_SAMPLE_CHANNEL_MASK,
                  "sample-rate", rate,
                  NULL);
    ipatch_sample_store_virtual_set_list (IPATCH_SAMPLE_STORE_VIRTUAL (virtstore),
                                          0, samlist);  /* !! caller takes over samlist */

    sampledata = ipatch_sample_data_new ();       /* ++ ref sampledata */
    ipatch_sample_data_add (sampledata, IPATCH_SAMPLE_STORE (virtstore));
    g_object_unref (virtstore);     /* -- unref virtstore */

    /* FIXME - Allow for other postfixes for i18n, this could be done by
     * assigning strings as GObject data to the source file object */
    if (strlen (newtitle) > 18)
      strcpy (newtitle + 18, left ? "_L" : "_R");
    else strcat (newtitle, left ? "_L" : "_R");

    channel = left ? IPATCH_SF2_SAMPLE_CHANNEL_LEFT : IPATCH_SF2_SAMPLE_CHANNEL_RIGHT;
  }
  else
  {
    sampledata = ipatch_sample_data_new ();       /* ++ ref sampledata */
    ipatch_sample_data_add (sampledata, IPATCH_SAMPLE_STORE (store));
  }
  
  sf2sample = ipatch_sf2_sample_new ();               /* ++ ref sf2sample */

  g_object_set (sf2sample,
                "name", &newtitle,
                "sample-data", sampledata,
                "sample-rate", rate,
                "root-note", (root_note != -1) ? root_note
                : IPATCH_SAMPLE_ROOT_NOTE_DEFAULT,
                "fine-tune", fine_tune,
                "loop-start", loop_start,
                "loop-end", loop_end,
                "channel", channel,
                NULL);

  g_object_unref (sampledata);  /* -- unref sampledata */

  return (sf2sample);   /* !! caller takes over reference */
}

/* SF2 -> File class is handled manually to install properties */
static void
_sf2_to_file_class_init (IpatchConverterClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->get_property = _sf2_to_file_get_property;
  obj_class->set_property = _sf2_to_file_set_property;

  klass->verify = NULL;
  klass->notes = NULL;
  klass->convert = _sf2_to_file_convert;

  g_object_class_install_property (obj_class, PROP_SF2_TO_FILE_CREATE_STORES,
                    g_param_spec_boolean ("create-stores", "Create stores", "Create sample stores",
                                          FALSE, G_PARAM_READWRITE));
}

static void
_sf2_to_file_get_property (GObject *object, guint property_id,
                           GValue *value, GParamSpec *pspec)
{
  IpatchConverterSF2ToFile *converter = (IpatchConverterSF2ToFile *)object;

  switch (property_id)
  {
    case PROP_SF2_TO_FILE_CREATE_STORES:
      g_value_set_boolean (value, converter->create_stores);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
_sf2_to_file_set_property (GObject *object, guint property_id,
                           const GValue *value, GParamSpec *pspec)
{
  IpatchConverterSF2ToFile *converter = (IpatchConverterSF2ToFile *)object;

  switch (property_id)
  {
    case PROP_SF2_TO_FILE_CREATE_STORES:
      converter->create_stores = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

CONVERTER_CLASS_INIT(file_to_sf2);
CONVERTER_CLASS_INIT(file_to_sf2_sample);

CONVERTER_GET_TYPE(sf2_to_file, SF2ToFile);
CONVERTER_GET_TYPE(file_to_sf2, FileToSF2);
CONVERTER_GET_TYPE(file_to_sf2_sample, FileToSF2Sample);

