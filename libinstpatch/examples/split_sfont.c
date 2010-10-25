/*
 * An example of using libInstPatch.
 * Splits SoundFonts into multiple SoundFont files, one for each preset.
 * The files are named <original_file.sf2-preset.sf2>.
 *
 * Public domain use as you please
 *
 * Joshua "Element" Green - Dec 12, 2003
 */

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/libinstpatch.h>

/* check if an error is set, if it is then use its message, otherwise note
   that there is no detailed error message */
#define ERROR_MSG(err) (err ? err->message : "<No detailed error message>")

static void dup_sfont_properties (IpatchSF2 *src, IpatchSF2 *dest);
static IpatchSF2Preset *recursive_dup_preset (IpatchSF2 *sf,
					      IpatchSF2Preset *src,
					      GError **err);

int
main (int argc, char *argv[])
{
  IpatchFile *file;		/* file object */
  IpatchSF2File *new_file;	/* new SoundFont file object */
  IpatchSF2 *sfont, *newsf;	/* source and destination SoundFont objects */
  IpatchSF2Writer *sf2writer;	/* SoundFont writer object */
  IpatchSF2Preset *preset;	/* SoundFont preset pointer */
  IpatchSF2Preset *new_preset;	/* new preset */
  IpatchList *list;		/* an item list object */
  IpatchIter iter;		/* an iterator structure (on the stack) */
  IpatchItem *item;		/* generic item pointer */
  char *src_filename;		/* file name of source SoundFont */
  char *s, *s2;
  GError *err = NULL;
  int i;

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s [FILES]\n", argv[0]);
      fprintf (stderr, "Splits SoundFont files into individual presets\n");
      exit (1);
    }

  ipatch_init ();	      /* must initialize libInstPatch first */

  for (i = 1; i < argc; i++)	/* loop over file names */
    {
      file = ipatch_file_new ();	/* ++ ref new file */
      src_filename = argv[i];

      /* open the file in read mode */
      if (!ipatch_file_open (file, src_filename, "r", &err))
	{
	  fprintf (stderr, "Failed to open file '%s': %s\n",
		   src_filename, ERROR_MSG (err));
	  g_clear_error (&err);	/* free the error information */
	  g_object_unref (file); /* -- unref the file to free it */
	  continue;
	}

      /* check if its a SoundFont file */
      if (ipatch_file_identify (file, &err) != IPATCH_TYPE_SF2_FILE)
	{
	  if (err)
	    {
	      fprintf (stderr, "Error while identifying file '%s': %s\n",
		       src_filename, ERROR_MSG (err));
	      g_clear_error (&err);
	    }
	  else fprintf (stderr, "File '%s' is not a SoundFont\n",
			src_filename);
	  g_object_unref (file);
	  continue;
	}

      /* load the SoundFont file to an object */
      if (!(item = ipatch_file_load_object (file, &err))) /* ++ ref sfont */
	{
	  fprintf (stderr, "Failed to load SoundFont file '%s': %s\n",
		   src_filename, ERROR_MSG (err));
	  g_clear_error (&err);
	  g_object_unref (file);
	  continue;
	}

      sfont = IPATCH_SF2 (item); /* type cast the item to a SoundFont */

      g_object_unref (file);	/* -- unref file, we are done with it */

      /* ++ ref new list of preset children */
      list = ipatch_container_get_children (IPATCH_CONTAINER (sfont),
					    IPATCH_TYPE_SF2_PRESET);
      ipatch_list_init_iter (list, &iter); /* initialize iterator to list */
      preset = ipatch_sf2_preset_first (&iter);	/* first preset */

      /* loop over SoundFont presets */
      for (; preset; preset = ipatch_sf2_preset_next (&iter))
	{
	  newsf = ipatch_sf2_new (); /* ++ ref new destination SoundFont */
	  dup_sfont_properties (sfont, newsf); /* duplicate the properties */

	  /* duplicate the preset       (++ ref duplicate) */
	  if (!(new_preset = recursive_dup_preset (newsf, preset, &err)))
	    {
	      fprintf (stderr, "Failed to duplicate preset: %s\n",
		       ERROR_MSG (err));
	      g_clear_error (&err);
	      g_object_unref (newsf);
	      continue;
	    }

	  /* set bank and program # to 0 */
	  g_object_set (new_preset,
			"bank", 0,
			"program", 0,
			NULL);

	  /* add the new preset to the new SoundFont */
	  ipatch_container_add (IPATCH_CONTAINER (newsf),
				IPATCH_ITEM (new_preset));

	  g_object_unref (new_preset); /* -- unref new preset */

	  /* create new SoundFont file name <OldFile>-<PresetName>.sf2 */
	  g_object_get (preset, "name", &s, NULL); /* get preset name */
	  s2 = g_strconcat (src_filename, "-", s, ".sf2", NULL);
	  g_free (s);

	  /* create new SoundFont file and set its name */
	  new_file = ipatch_sf2_file_new (); /* ++ ref new file */
	  if (!ipatch_file_open (IPATCH_FILE (new_file), s2, "w", &err))
	    {
	      fprintf (stderr, "Failed to open file '%s' for writing: %s\n",
		       s2, ERROR_MSG (err));
	      g_clear_error (&err);
	      g_free (s2);
	      g_object_unref (new_file);
	      g_object_unref (newsf);
	      continue;
	    }

	  /* save the new SoundFont file */
	  sf2writer = ipatch_sf2_writer_new (newsf, new_file);
	  if (!ipatch_sf2_write (sf2writer, &err))
	    {
	      fprintf (stderr, "Failed to save new SoundFont '%s': %s\n",
		       s2, ERROR_MSG (err));
	      g_clear_error (&err);
	    }

	  g_object_unref (sf2writer);
	  g_free (s2);
	  g_object_unref (new_file);
	  g_object_unref (newsf);
	}

      g_object_unref (list); /* -- unref the list we are done with it */
      g_object_unref (sfont);	/* -- unref source SoundFont */
    } /* for each file */
}

/* duplicate text information between 2 SoundFont files (name, author, engine
   Version, etc) */
static void
dup_sfont_properties (IpatchSF2 *src, IpatchSF2 *dest)
{
  GParamSpec **props, **p; /* array of properties and an index pointer */
  GParamSpec *pspec;
  char *s;

  props = g_object_class_list_properties (G_OBJECT_GET_CLASS (src), NULL);
  for (p = props; *p; p++)	/* loop over all properties */
    {
      pspec = *p;

      /* string property? */
      if (G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_STRING)
	{
	  g_object_get (src, pspec->name, &s, NULL); /* get the value */

	  /* if the value is set then set it on the destination sfont */
	  if (s)
	    {
	      g_object_set (dest, pspec->name, s, NULL);
	      g_free (s);	/* free the value string */
	    }
	}
    }
}

/* YUCK! This is the messy part, need to come up with some nicer API for
   duplicating objects between toplevel patch files, this mess is also
   seen in the Swami paste routines */

/* recursively duplicate a SoundFont preset (i.e., the preset, its instruments
   and the samples of those instruments */
static IpatchSF2Preset *
recursive_dup_preset (IpatchSF2 *sf, IpatchSF2Preset *src, GError **err)
{
  IpatchItem *dup_preset;
  IpatchSF2Zone *pzone, *izone;
  IpatchItem *inst, *dup_inst, *sample, *dup_sample;
  IpatchList *pzone_list, *izone_list;
  IpatchIter pzone_iter, izone_iter;

  /* duplicate the preset (still references insts from old sfont) */
  if (!(dup_preset = ipatch_item_duplicate (IPATCH_ITEM (src), NULL, err)))
    return (NULL);

  /* ++ ref new list */
  pzone_list = ipatch_container_get_children (IPATCH_CONTAINER (dup_preset),
					      IPATCH_TYPE_SF2_PZONE);
  ipatch_list_init_iter (pzone_list, &pzone_iter);
  pzone = ipatch_sf2_zone_first (&pzone_iter);
  while (pzone)			/* loop over preset zones */
    { /* ++ ref the preset zone's instrument */
      inst = ipatch_sf2_zone_ref_item (pzone);
      if (inst)			/* not a global preset zone? */
	{ /* ++ ref new duplicate instrument */
	  if (!(dup_inst = ipatch_item_duplicate (inst, NULL, err)))
	    {
	      g_object_unref (inst);
	      g_object_unref (pzone_list);
	      g_object_unref (dup_preset);
	      return (NULL);
	    }
	  g_object_unref (inst); /* don't need inst no more */

	  /* point preset zone to the new instrument */
	  ipatch_sf2_zone_set_item (pzone, dup_inst);

	  /* add the instrument to the SoundFont */
	  ipatch_container_add (IPATCH_CONTAINER (sf), IPATCH_ITEM (dup_inst));

	  izone_list =		/* ++ ref new list */
	    ipatch_container_get_children (IPATCH_CONTAINER (dup_inst),
					   IPATCH_TYPE_SF2_IZONE);
	  ipatch_list_init_iter (izone_list, &izone_iter);
	  izone = ipatch_sf2_zone_first (&izone_iter);
	  while (izone)		/* loop over instrument zones */
	    { /* ++ ref instrument zone's sample */
	      sample = ipatch_sf2_zone_ref_item (izone);
	      if (sample)	/* not a global zone? */
		{ /* ++ ref new duplicate sample */
		  if (!(dup_sample = ipatch_item_duplicate (sample, NULL,err)))
		    {
		      g_object_unref (izone_list);
		      g_object_unref (dup_inst);
		      g_object_unref (pzone_list);
		      g_object_unref (dup_preset);
		      return (NULL);
		    }

		  g_object_unref (sample); /* -- don't need sample no more */

		  /* point instrument zone to the new sample */
		  ipatch_sf2_zone_set_item (izone, dup_sample);

		  /* add the sample to the SoundFont */
		  ipatch_container_add (IPATCH_CONTAINER (sf),
					IPATCH_ITEM (dup_sample));

		  g_object_unref (dup_sample); /* -- unref dup sample */
		}
	      izone = ipatch_sf2_zone_next (&izone_iter);
	    } /* while instrument zones */

	  g_object_unref (izone_list); /* -- unref instrument zone list */
	  g_object_unref (dup_inst); /* -- unref duplicate instrument */
	} /* if not global preset zone */

      pzone = ipatch_sf2_zone_next (&pzone_iter);
    }

  g_object_unref (pzone_list);	/* -- unref preset zone list */

  return (IPATCH_SF2_PRESET (dup_preset)); /* !! caller takes over reference */
}
