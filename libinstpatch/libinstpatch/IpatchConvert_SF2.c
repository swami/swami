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

/*
 * SoundFont conversion handlers (DLS is master format):
 * IpatchSF2 <==> IpatchSF2File
 * IpatchSndFile => IpatchSF2Sample
 * IpatchSF2 <==> IpatchDLS2
 * IpatchSF2Preset <==> IpatchDLS2Inst
 * IpatchSF2Inst <==> IpatchDLS2Inst
 * IpatchSF2PZone <==> IpatchDLS2Region
 * IpatchSF2IZone <==> IpatchDLS2Region
 * IpatchSF2Sample <==> IpatchDLS2Sample
 */

static IpatchSF2Sample *create_sf2_sample_helper (IpatchSampleStoreSndFile *store,
                                                  gboolean left);

/* init routine for SF2 conversion types */
void
_ipatch_convert_SF2_init (void)
{
  g_type_class_ref (IPATCH_TYPE_CONVERTER_SF2_TO_FILE);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_FILE_TO_SF2);
  g_type_class_ref (IPATCH_TYPE_CONVERTER_FILE_TO_SF2_SAMPLE);

  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_SF2_TO_FILE, 0,
				 IPATCH_TYPE_SF2, 0, 1,
				 IPATCH_TYPE_SF2_FILE, IPATCH_TYPE_FILE, 1);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_FILE_TO_SF2, 0,
				 IPATCH_TYPE_SF2_FILE, IPATCH_TYPE_FILE, 1,
				 IPATCH_TYPE_SF2, IPATCH_TYPE_BASE, 0);
  ipatch_register_converter_map (IPATCH_TYPE_CONVERTER_FILE_TO_SF2_SAMPLE, 0,
				 IPATCH_TYPE_SND_FILE, IPATCH_TYPE_FILE, 1,
				 IPATCH_TYPE_SF2_SAMPLE, 0, 0);
}

/* =============
 * Notes methods
 * ============= */

#if 0
NULL_NOTES (sf2_to_dls2)

static char *
_dls2_to_sf2_notes (IpatchConverter *converter)
{
  return (g_strconcat (
   _("Loss of precision (DLS has higher precision parameters)\n"),
   _("Info text for samples and instruments will be lost (except name)\n"),
   _("8 bit samples will be converted to 16 bit\n"),
  NULL));
}

NULL_NOTES (sf2_preset_to_dls2_inst);
NULL_NOTES (dls2_inst_to_sf2_preset);
NULL_NOTES (sf2_inst_to_dls2_inst);
NULL_NOTES (dls2_inst_to_sf2_inst);
NULL_NOTES (sf2_pzone_to_dls2_region);
NULL_NOTES (dls2_region_to_sf2_pzone);
NULL_NOTES (sf2_izone_to_dls2_region);
NULL_NOTES (dls2_region_to_sf2_izone);
NULL_NOTES (sf2_sample_to_dls2_sample);

static char *
_dls2_sample_to_sf2_sample_notes (IpatchConverter *converter)
{
  return (g_strconcat (
   _("Info strings will be lost (except name)\n"),
   _("Loop type will be lost (SoundFont stores loop type in instruments)\n"),
   _("8bit samples will be converted to 16 bit\n"),
   NULL));
}
#endif


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
  int retval;

  sfont = IPATCH_SF2 (IPATCH_CONVERTER_INPUT (converter));
  file = IPATCH_SF2_FILE (IPATCH_CONVERTER_OUTPUT (converter));

  handle = ipatch_file_open (IPATCH_FILE (file), NULL, "w", err);
  if (!handle) return (FALSE);

  writer = ipatch_sf2_writer_new (handle, sfont);	/* ++ ref new writer */
  retval = ipatch_sf2_writer_save (writer, err);
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

#if 0

static gboolean
_sf2_to_dls2_convert (IpatchConverter *converter, GError **err)
{
  return (TRUE);
}

static gboolean
_dls2_to_sf2_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2 *dls;
  IpatchSF2 *sfont;
  IpatchSF2Preset *sf_preset;
  IpatchDLS2Inst *dls_inst;
  IpatchList *dlsinst_list;
  IpatchIter dlsinst_iter;

  dls = IPATCH_DLS2 (IPATCH_CONVERTER_INPUT (converter));
  sfont = IPATCH_SF2 (IPATCH_CONVERTER_OUTPUT (converter));

  /* ++ ref new list */
  dlsinst_list = ipatch_container_get_children (IPATCH_CONTAINER (dls),
						IPATCH_TYPE_DLS2_INST);
  ipatch_list_init_iter (dlsinst_list, &dlsinst_iter);
  dls_inst = ipatch_dls2_inst_first (&dlsinst_iter);
  while (dls_inst)	       /* loop over DLS instruments */
    {
      sf_preset = ipatch_sf2_preset_new (); /* ++ ref new SoundFont preset */

      /* convert DLS instrument to SoundFont preset */
      if (!ipatch_convert (dls_inst, sf_preset, err, NULL))
	{
	  g_object_unref (sf_preset); /* -- unref SoundFont preset */
	  g_object_unref (dlsinst_list); /* -- unref instrument list */
	  return (FALSE);
	}

      /* add preset to SoundFont */
      ipatch_container_append (IPATCH_CONTAINER (sfont), IPATCH_ITEM (sf_preset));
      g_object_unref (sf_preset); /* -- unref creator's reference */

      dls_inst = ipatch_dls2_inst_next (&dlsinst_iter);
    }

  g_object_unref (dlsinst_list); /* -- unref instrument list */

  return (TRUE);
}

static gboolean
_sf2_preset_to_dls2_inst_convert (IpatchConverter *converter, GError **err)
{
  return (TRUE);
}

static gboolean
_dls2_inst_to_sf2_preset_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2Inst *dls_inst;
  IpatchSF2Preset *sf_preset;
  IpatchSF2Inst *sf_inst;
  IpatchSF2PZone *pzone;
  char *name;
  int bank, program;

  dls_inst = IPATCH_DLS2_INST (IPATCH_CONVERTER_INPUT (converter));
  sf_preset = IPATCH_SF2_PRESET (IPATCH_CONVERTER_OUTPUT (converter));

  g_object_get (dls_inst,
		"name", &name,
		"bank", &bank,
		"program", &program,
		NULL);

  /* FIXME - what about program # clashes with percussion instruments? */
  if (bank & IPATCH_DLS2_INST_BANK_PERCUSSION) bank = 128;

  g_object_set (sf_preset,
		"name", name,
		"bank", bank,
		"program", program,
		NULL);

  sf_inst = ipatch_sf2_inst_new (); /* ++ ref new instrument */

  /* convert DLS instrument to SoundFont instrument */
  if (!ipatch_convert (dls_inst, sf_inst, err, NULL))
    {
      g_object_unref (sf_inst); /* -- unref SoundFont instrument */
      return (FALSE);
    }

  /* create a new preset zone linking to new instrument */
  pzone = ipatch_sf2_preset_new_zone (sf_preset, sf_inst); /* ++ ref */

  g_object_unref (sf_inst);	/* -- unref instrument */
  g_object_unref (pzone);	/* -- unref preset zone */

  return (TRUE);
}

static gboolean
_sf2_inst_to_dls2_inst_convert (IpatchConverter *converter, GError **err)
{
  return (TRUE);
}

static gboolean
_dls2_inst_to_sf2_inst_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2Inst *dls_inst;
  IpatchDLS2Region *dls_region;
  IpatchSF2Inst *sf_inst;
  IpatchSF2Zone *sf_izone;
  IpatchList *dlsregion_list;
  IpatchIter dlsregion_iter;
  char *name;

  dls_inst = IPATCH_DLS2_INST (IPATCH_CONVERTER_INPUT (converter));
  sf_inst = IPATCH_SF2_INST (IPATCH_CONVERTER_OUTPUT (converter));

  g_object_get (dls_inst, "name", &name, NULL);
  g_object_set (dls_inst, "name", name, NULL);

  /* ++ ref new list */
  dlsregion_list = ipatch_container_get_children (IPATCH_CONTAINER (dls_inst),
						  IPATCH_TYPE_DLS2_REGION);
  ipatch_list_init_iter (dlsregion_list, &dlsregion_iter);
  dls_region = ipatch_dls2_region_first (&dlsregion_iter);
  while (dls_region)	       /* loop over DLS regions */
    {
      sf_izone = ipatch_sf2_izone_new (); /* ++ ref new SoundFont izone */

      /* convert DLS region to SoundFont instrument zone */
      if (!ipatch_convert (dls_region, sf_izone, err, NULL))
	{
	  g_object_unref (sf_izone); /* -- unref SoundFont izone */
	  g_object_unref (dlsregion_list); /* -- unref region list */
	  return (FALSE);
	}

      /* add SoundFont zone to instrument */
      ipatch_container_append (IPATCH_CONTAINER (sf_inst),
			       IPATCH_ITEM (sf_izone));
      g_object_unref (sf_izone); /* -- unref creator's reference */

      dls_region = ipatch_dls2_region_next (&dlsregion_iter);
    }

  g_object_unref (dlsregion_list); /* -- unref region list */

  return (TRUE);
}

static gboolean
_sf2_pzone_to_dls2_region_convert (IpatchConverter *converter, GError **err)
{
  return (TRUE);
}

static gboolean
_dls2_region_to_sf2_pzone_convert (IpatchConverter *converter, GError **err)
{
  return (TRUE);
}

static gboolean
_sf2_izone_to_dls2_region_convert (IpatchConverter *converter, GError **err)
{
  return (TRUE);
}

static gboolean
_dls2_region_to_sf2_izone_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2Region *dls_region;
  IpatchSF2Zone *sf_izone;
  IpatchDLS2Sample *dls_sample;
  IpatchDLS2SampleInfo *sample_info, *regsample_info, *temp_info;
  IpatchSF2GenAmount amt;
  guint32 flags;

  dls_region = IPATCH_DLS2_REGION (IPATCH_CONVERTER_INPUT (converter));
  sf_izone = IPATCH_SF2_ZONE (IPATCH_CONVERTER_INPUT (converter));

  IPATCH_ITEM_RLOCK (dls_region);

  /* note range */
  if (!(dls_region->note_range_low == 0 && dls_region->note_range_high == 127))
    {
      amt.range.low = dls_region->note_range_low;
      amt.range.high = dls_region->note_range_high;
      IPATCH_SF2_ZONE_GEN_AMT (sf_izone, IPATCH_SF2_GEN_NOTE_RANGE) = amt;
      IPATCH_SF2_ZONE_GEN_SET_FLAG (sf_izone, IPATCH_SF2_GEN_NOTE_RANGE);
    }

  /* velocity range */
  if (!(dls_region->velocity_range_low == 0
	&& dls_region->velocity_range_high == 127))
    {
      amt.range.low = dls_region->velocity_range_low;
      amt.range.high = dls_region->velocity_range_high;
      IPATCH_SF2_ZONE_GEN_AMT (sf_izone, IPATCH_SF2_GEN_VELOCITY_RANGE) = amt;
      IPATCH_SF2_ZONE_GEN_SET_FLAG (sf_izone, IPATCH_SF2_GEN_VELOCITY_RANGE);
    }

  /* exclusive key group */
  if (dls_region->key_group > 0)
    {
      IPATCH_SF2_ZONE_GEN_AMT (sf_izone, IPATCH_SF2_GEN_EXCLUSIVE_CLASS).sword
	= dls_region->key_group;
      IPATCH_SF2_ZONE_GEN_SET_FLAG (sf_izone, IPATCH_SF2_GEN_EXCLUSIVE_CLASS);
    }






  flags = ipatch_item_get_flags (dls_region); /* get item flags */

  /* channel steering override? */
  if (flags & IPATCH_DLS2_REGION_MULTI_CHANNEL)
    {
      amt.sword = ipatch_dls2_region_channel_map_stereo (dls_region->channel);
      amt.sword *= 500;	      /* -500, 0, 500 = left, center, right */
      if (amt.sword != 0)	/* center is default */
	{
	  IPATCH_SF2_ZONE_GEN_AMT (sf_izone, IPATCH_SF2_GEN_PAN) = amt;
	  IPATCH_SF2_ZONE_GEN_SET_FLAG (sf_izone, IPATCH_SF2_GEN_PAN);
	}

      if (dls_region->channel > IPATCH_DLS2_REGION_CHANNEL_CENTER)
	{
	  ipatch_converter_log (converter, G_OBJECT (dls_region),
				IPATCH_CONVERTER_LOG_WARN,
				_("Loss of surround sound information"));
	}
    }

  dls_sample = ipatch_dls2_region_get_sample (dls_region); /* ++ ref sample */
  g_return_val_if_fail (dls_sample != NULL, FALSE);

  /* get region override sample info and sample info */
  sample_info = dls_sample->sample_info;
  regsample_info = dls_region->sample_info;

  /* sample looping */
  if (regsample_info && flags & IPATCH_DLS2_REGION_SAMPLE_OVERRIDE_LOOP)
    temp_info = regsample_info;
  else temp_info = sample_info;

  amt.uword = IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP; /* default */
  if (temp_info)
    {
      switch (temp_info->options & IPATCH_DLS2_SAMPLE_LOOP_MASK)
	{
	case IPATCH_DLS2_SAMPLE_LOOP_NORMAL:
	  amt.uword = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP;
	  break;
	case IPATCH_DLS2_SAMPLE_LOOP_RELEASE:
	  amt.uword = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP_RELEASE;
	  break;
	}
    }

  if (amt.uword != IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP) /* default no loop */
    {
      IPATCH_SF2_ZONE_GEN_AMT (sf_izone, IPATCH_SF2_GEN_SAMPLE_MODES) = amt;
      IPATCH_SF2_ZONE_GEN_SET_FLAG (sf_izone, IPATCH_SF2_GEN_SAMPLE_MODES);
    }

  /* root note */
  if (regsample_info && flags & IPATCH_DLS2_REGION_SAMPLE_OVERRIDE_ROOT_NOTE)
    {
      IPATCH_SF2_ZONE_GEN_AMT (sf_izone, IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE).sword
	= regsample_info->root_note;
      IPATCH_SF2_ZONE_GEN_SET_FLAG (sf_izone, IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE);
    }

  /* fine tune */
  if (regsample_info && flags & IPATCH_DLS2_REGION_SAMPLE_OVERRIDE_FINE_TUNE)
    {
      IPATCH_SF2_ZONE_GEN_AMT (sf_izone, IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE).sword
	= regsample_info->fine_tune;
      IPATCH_SF2_ZONE_GEN_SET_FLAG (sf_izone, IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE);
    }

  /* gain */
  if (regsample_info && flags & IPATCH_DLS2_REGION_SAMPLE_OVERRIDE_GAIN)
    temp_info = regsample_info;
  else temp_info = sample_info;

  if (temp_info && temp_info->gain != 0)
    {
      if (temp_info->gain < 0)	/* SoundFont only supports attenuation */
	{
	  amt.sword = ipatch_unit_dls_gain_to_sf2_atten (temp_info->gain);
	  IPATCH_SF2_ZONE_GEN_AMT (sf_izone, IPATCH_SF2_GEN_ATTENUATION) = amt;
	  IPATCH_SF2_ZONE_GEN_SET_FLAG (sf_izone, IPATCH_SF2_GEN_ATTENUATION);
	}
      else ipatch_converter_log (converter, G_OBJECT (dls_region),
				 IPATCH_CONVERTER_LOG_WARN,
				 _("Positive gain not supported"));
    }

  /* loop start */
  if (regsample_info && flags & IPATCH_DLS2_REGION_SAMPLE_LOOP_START)
    temp_info = regsample_info;
  else temp_info = sample_info;

here is where you want to continue??


  IPATCH_ITEM_RUNLOCK (dls_region);

  return (TRUE);
}

static gboolean
_sf2_sample_to_dls2_sample_convert (IpatchConverter *converter, GError **err)
{
  IpatchSF2Sample *sf_sample;
  IpatchDLS2Sample *dls_sample;
  IpatchDLS2SampleInfo *sample_info;
  guint32 size;
  char *name;

  sf_sample = IPATCH_SF2_SAMPLE (IPATCH_CONVERTER_INPUT (converter));
  dls_sample = IPATCH_DLS2_SAMPLE (IPATCH_CONVERTER_OUTPUT (converter));

  IPATCH_ITEM_RLOCK (sf_sample);

  ipatch_dls2_sample_set_data (dls_sample, sf_sample->sample_data);
  dls_sample->rate = sf_sample->rate;

  /* FIXME - Handle linked stereo SoundFont samples */

  name = ipatch_sf2_sample_get_name (sf_sample);
  g_object_set (dls_sample,
		"name", name,
		"loop-type", IPATCH_SAMPLE_LOOP_NONE, /* default no loop */
		"loop-start", sf_sample->loop_start,
		"loop-end", sf_sample->loop_end,
		"root-note", sf_sample->root_note,
		"fine-tune", sf_sample->fine_tune,
		"gain", 0,	/* unity gain */
		NULL);
  g_free (name);

  IPATCH_ITEM_RUNLOCK (sf_sample);

  return (TRUE);
}

static gboolean
_dls2_sample_to_sf2_sample_convert (IpatchConverter *converter, GError **err)
{
  IpatchDLS2Sample *dls_sample;
  IpatchDLS2SampleInfo *sample_info;
  IpatchSF2Sample *sf_sample;
  guint32 size;
  char *name;

  dls_sample = IPATCH_DLS2_SAMPLE (IPATCH_CONVERTER_INPUT (converter));
  sf_sample = IPATCH_SF2_SAMPLE (IPATCH_CONVERTER_OUTPUT (converter));

  IPATCH_ITEM_RLOCK (dls_sample);

  ipatch_sf2_sample_set_data (sf_sample, dls_sample->sample_data);
  sf_sample->rate = dls_sample->rate;
  sf_sample->type = 0;

  /* FIXME - Handle linked stereo SoundFont samples */
  ipatch_sf2_sample_set_linked (sf_sample, NULL);

  g_object_get (dls_sample, "name", &name, NULL);
  ipatch_sf2_sample_set_name (sf_sample, name);
  g_free (name);

  if (dls_sample->sample_info != NULL)
    {
      sample_info = dls_sample->sample_info;
      sf_sample->loop_start = sample_info->loop_start;
      sf_sample->loop_end = sample_info->loop_end;
      sf_sample->root_note = sample_info->root_note;
      sf_sample->fine_tune = sample_info->fine_tune;
    }
  else				/* default sample info */
    {
      size = ipatch_sample_data_get_size (dls_sample->sample_data);
      if (size >= 12)
	{
	  sf_sample->loop_start = 4;
	  sf_sample->loop_end = size - 4;
	}
      else
	{
	  sf_sample->loop_start = 0;
	  sf_sample->loop_end = size;
	}

      sf_sample->root_note = 60;
      sf_sample->fine_tune = 0;
    }

  IPATCH_ITEM_RUNLOCK (dls_sample);

  return (TRUE);
}

#endif


CONVERTER_CLASS_INIT(sf2_to_file);
CONVERTER_CLASS_INIT(file_to_sf2);
CONVERTER_CLASS_INIT(file_to_sf2_sample);

#if 0

CONVERTER_CLASS_INIT(sf2_to_dls2);
CONVERTER_CLASS_INIT(dls2_to_sf2);
CONVERTER_CLASS_INIT(sf2_preset_to_dls2_inst);
CONVERTER_CLASS_INIT(dls2_inst_to_sf2_preset);
CONVERTER_CLASS_INIT(sf2_inst_to_dls2_inst);
CONVERTER_CLASS_INIT(dls2_inst_to_sf2_inst);
CONVERTER_CLASS_INIT(sf2_pzone_to_dls2_region);
CONVERTER_CLASS_INIT(dls2_region_to_sf2_pzone);
CONVERTER_CLASS_INIT(sf2_izone_to_dls2_region);
CONVERTER_CLASS_INIT(dls2_region_to_sf2_izone);
CONVERTER_CLASS_INIT(sf2_sample_to_dls2_sample);
CONVERTER_CLASS_INIT(dls2_sample_to_sf2_sample);

#endif

CONVERTER_GET_TYPE(sf2_to_file, SF2ToFile);
CONVERTER_GET_TYPE(file_to_sf2, FileToSF2);
CONVERTER_GET_TYPE(file_to_sf2_sample, FileToSF2Sample);

#if 0
CONVERTER_GET_TYPE(sf2_to_dls2, SF2ToDLS2);
CONVERTER_GET_TYPE(dls2_to_sf2, DLS2ToSF2);
CONVERTER_GET_TYPE(sf2_preset_to_dls2_inst, SF2PresetToDLS2Inst);
CONVERTER_GET_TYPE(dls2_inst_to_sf2_preset, DLS2InstToSF2Preset);
CONVERTER_GET_TYPE(sf2_inst_to_dls2_inst, SF2InstToDLS2Inst);
CONVERTER_GET_TYPE(dls2_inst_to_sf2_inst, DLS2InstToSF2Inst);
CONVERTER_GET_TYPE(sf2_pzone_to_dls2_region, SF2PZoneToDLS2Region);
CONVERTER_GET_TYPE(dls2_region_to_sf2_pzone, DLS2RegionToSF2PZone);
CONVERTER_GET_TYPE(sf2_izone_to_dls2_region, SF2IZoneToDLS2Region);
CONVERTER_GET_TYPE(dls2_region_to_sf2_izone, DLS2RegionToSF2IZone);
CONVERTER_GET_TYPE(sf2_sample_to_dls2_sample, SF2SampleToDLS2Sample);
CONVERTER_GET_TYPE(dls2_sample_to_sf2_sample, DLS2SampleToSF2Sample);


/* DLS destination to generator mapping for constant connections */
gint16 const_conn_to_gen[] = {
  IPATCH_DLS2_CONN_DEST_LFO_DELAY, IPATCH_SF2_GEN_MOD_LFO_DELAY
  IPATCH_DLS2_CONN_DEST_LFO_FREQ, IPATCH_SF2_GEN_MOD_LFO_FREQ,
  IPATCH_DLS2_CONN_DEST_VIB_DELAY, IPATCH_SF2_GEN_VIB_LFO_DELAY,
  IPATCH_DLS2_CONN_DEST_VIB_FREQ, IPATCH_SF2_GEN_VIB_LFO_FREQ,
  IPATCH_DLS2_CONN_DEST_EG2_DELAY, IPATCH_SF2_GEN_MOD_ENV_DELAY,
  IPATCH_DLS2_CONN_DEST_EG2_ATTACK, IPATCH_SF2_GEN_MOD_ENV_ATTACK,
  IPATCH_DLS2_CONN_DEST_EG2_HOLD, IPATCH_SF2_GEN_MOD_ENV_HOLD,
  IPATCH_DLS2_CONN_DEST_EG2_DECAY, IPATCH_SF2_GEN_MOD_ENV_DECAY,
  IPATCH_DLS2_CONN_DEST_EG2_SUSTAIN, IPATCH_SF2_GEN_MOD_ENV_SUSTAIN,
  IPATCH_DLS2_CONN_DEST_EG2_RELEASE, IPATCH_SF2_GEN_MOD_ENV_RELEASE,
  IPATCH_DLS2_CONN_DEST_EG1_DELAY, IPATCH_SF2_GEN_VOL_ENV_DELAY,
  IPATCH_DLS2_CONN_DEST_EG1_ATTACK, IPATCH_SF2_GEN_VOL_ENV_ATTACK,
  IPATCH_DLS2_CONN_DEST_EG1_HOLD, IPATCH_SF2_GEN_VOL_ENV_HOLD,
  IPATCH_DLS2_CONN_DEST_EG1_DECAY, IPATCH_SF2_GEN_VOL_ENV_DECAY,
  IPATCH_DLS2_CONN_DEST_EG1_SUSTAIN, IPATCH_SF2_GEN_VOL_ENV_SUSTAIN,
  IPATCH_DLS2_CONN_DEST_EG1_RELEASE, IPATCH_SF2_GEN_VOL_ENV_RELEASE,
  IPATCH_DLS2_CONN_DEST_FILTER_CUTOFF, IPATCH_SF2_GEN_FILTER_FC,
  IPATCH_DLS2_CONN_DEST_FILTER_Q, IPATCH_SF2_GEN_FILTER_Q,
  IPATCH_DLS2_CONN_DEST_CHORUS, IPATCH_SF2_GEN_CHORUS_SEND,
  IPATCH_DLS2_CONN_DEST_REVERB, IPATCH_SF2_GEN_REVERB_SEND,
  IPATCH_DLS2_CONN_DEST_PAN, IPATCH_SF2_GEN_PAN,
  -1
};


/* no equivalent, requires duplicate sample data */
IPATCH_SF2_GEN_SAMPLE_START
IPATCH_SF2_GEN_SAMPLE_END
IPATCH_SF2_GEN_SAMPLE_COARSE_START
IPATCH_SF2_GEN_SAMPLE_COARSE_END

/* region sample info override */
IPATCH_SF2_GEN_SAMPLE_LOOP_START
IPATCH_SF2_GEN_SAMPLE_LOOP_END
IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START
IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END

/* region split values */
IPATCH_SF2_GEN_NOTE_RANGE
IPATCH_SF2_GEN_VELOCITY_RANGE

/* special */
IPATCH_SF2_GEN_ATTENUATION
IPATCH_SF2_GEN_FIXED_NOTE
IPATCH_SF2_GEN_FIXED_VELOCITY
IPATCH_SF2_GEN_SAMPLE_MODES
IPATCH_SF2_GEN_EXCLUSIVE_CLASS
IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE


/**
 * ipatch_convert_dls_conns_to_sf2_genmods:
 * @conns: List of DLS connections
 * @gens: SoundFont generator array to store parameter values to
 * @mods: Pointer to an empty list to populate with modulators
 *
 * Converts a list of DLS connections into SoundFont generators and modulators.
 *
 * Returns: Number of connections successfully converted
 */
int
ipatch_convert_dls_conns_to_sf2_genmods (GSList *conns,
					 IpatchSF2GenArray *gens,
					 GSList **mods)
{
  int converted = 0;
  IpatchDLS2Conn *conn;
  GSList *p;
  int gen;

  for (p = conns; p; p = g_slist_next (p))
    {
      conn = (IpatchDLS2Conn *)(p->data);

      /* skip unknown transforms */
      if ((conn->trans & IPATCH_DLS2_CONN_MASK_OUTPUT_TRANS)
	  >> IPATCH_DLS2_CONN_SHIFT_OUTPUT_TRANS
	  != IPATCH_DLS2_CONN_OUTPUT_TRANS_NONE

	  || (conn->trans & IPATCH_DLS2_CONN_MASK_CTRLSRC_TRANS)
	  >> IPATCH_DLS2_CONN_SHIFT_CTRLSRC_TRANS
	  > IPATCH_DLS2_CONN_TRANS_SWITCH

	  || (conn->trans & IPATCH_DLS2_CONN_MASK_SRC_TRANS)
	  >> IPATCH_DLS2_CONN_SHIFT_SRC_TRANS
	  > IPATCH_DLS2_CONN_TRANS_SWITCH)
	continue;

      /* scaled constant connection? (NONE, NONE) */
      if (conn->src == IPATCH_DLS2_CONN_SRC_NONE
	  && conn->ctrlsrc == IPATCH_DLS2_CONN_SRC_NONE)
	{
	  if (conn->dest == IPATCH_DLS2_CONN_DEST_PITCH)
	    {
	      IPATCH_SF2_GEN_COARSE_TUNE;
	      IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE;
	    }

	} /* scaled source connection? (SET, NONE) */
      else if (conn->ctrlsrc == IPATCH_DLS2_CONN_SRC_NONE)
	{
	  /* all these match only non-inverting linear transforms */
	  if (!(conn->trans & IPATCH_DLS2_CONN_MASK_SRC_INVERT)
	      && !(conn->trans & IPATCH_DLS2_CONN_MASK_SRC_TRANS))
	    switch (conn->src)
	      {
	      case IPATCH_DLS2_CONN_SRC_NOTE:
		/* all these use unipolar transforms */
		if (conn->trans & IPATCH_DLS2_CONN_MASK_SRC_POLARITY) break;

		if (conn->dest == IPATCH_DLS2_CONN_DEST_PITCH)
		  gen = IPATCH_SF2_GEN_SCALE_TUNE;
		else if (conn->dest == IPATCH_DLS2_CONN_DEST_EG1_HOLD)
		  gen = IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_HOLD;
		else if (conn->dest == IPATCH_DLS2_CONN_DEST_EG1_DECAY)
		  gen = IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_DECAY;
		else if (conn->dest == IPATCH_DLS2_CONN_DEST_EG2_HOLD)
		  gen = IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_HOLD;
		else if (conn->dest == IPATCH_DLS2_CONN_DEST_EG2_DECAY)
		  gen = IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_DECAY;
		break;
	      case IPATCH_DLS2_CONN_SRC_LFO:
		/* all these use bipolar transforms */
		if (!(conn->trans & IPATCH_DLS2_CONN_MASK_SRC_POLARITY)) break;

		if (conn->dest == IPATCH_DLS2_CONN_DEST_PITCH)
		  gen = IPATCH_SF2_GEN_MOD_LFO_TO_PITCH;
		else if (conn->dest == IPATCH_DLS2_CONN_DEST_FILTER_CUTOFF)
		  gen = IPATCH_SF2_GEN_MOD_LFO_TO_FILTER_FC;
		else if (conn->dest == IPATCH_DLS2_CONN_DEST_GAIN)
		  gen = IPATCH_SF2_GEN_MOD_LFO_TO_VOL;
		break;
	      case IPATCH_DLS2_CONN_SRC_EG2:
		if (conn->dest == IPATCH_DLS2_CONN_DEST_FILTER_CUTOFF
		    && !(conn->trans & IPATCH_DLS2_CONN_MASK_SRC_POLARITY))
		  gen = IPATCH_SF2_GEN_MOD_ENV_TO_FILTER_FC;
		else if (conn->dest == IPATCH_DLS2_CONN_DEST_PITCH
			 && conn->trans & IPATCH_DLS2_CONN_MASK_SRC_POLARITY)
		  gen = IPATCH_SF2_GEN_MOD_ENV_TO_PITCH;
		break;
	      case IPATCH_DLS2_CONN_SRC_VIB:
		if (conn->dest == IPATCH_DLS2_CONN_DEST_PITCH
		    && conn->trans & IPATCH_DLS2_CONN_MASK_SRC_POLARITY)
		  gen = IPATCH_SF2_GEN_VIB_LFO_TO_PITCH;
		break;
	      default:
		break;
	      }
	} /* controlled scaled source connection? (SET, SET) */
      else if (conn->src != IPATCH_DLS2_CONN_SRC_NONE)
	{
	}
      else continue;		/* Invalid (NONE, SET), skip */

      dls_units = ipatch_dls_conn_get_dest_units (conn->dest);
      sf2_units = ipatch_sf2_genid_get_units (gen);
      val = ipatch_unit_convert (dls_units, sf2_units, conn->scale);
      val = ipatch_sf2_gen_clamp (gen, val);
      ipatch_sf2_gen_array_set_val (gens, gen, val);
    }

  return (converted);
}

void
ipatch_convert_dls_conn_to_sf2_genid (int dest)
{

}

#endif
