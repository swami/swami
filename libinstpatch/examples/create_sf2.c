/*
 * Example of creating a SoundFont from multiple audio samples
 *
 * Updated on Oct 14, 2010
 * Some API changes and additional comments in code.
 *
 * Updated on Nov 12, 2004
 * Now uses more convenient converter functions.
 *
 * Joshua "Element" Green - Sep 14, 2004
 * Use this example as you please (public domain)
 */
#include <stdio.h>
#include <libinstpatch/libinstpatch.h>

int
main (int argc, char *argv[])
{
  IpatchSF2 *sf2;
  IpatchFileHandle *fhandle;
  IpatchSF2File *sffile;
  GObject *sample;	/* IpatchSF2Sample */
  IpatchSF2Inst *inst;
  IpatchSF2Preset *preset;
  GError *err = NULL;
  char *fname, *name;
  int i;

  if (argc < 2)
  {
    fprintf (stderr, "Usage: create_sf2 sample1.wav [sample2.wav sample3.aiff ..]\n");
    return (1);
  }

  /* initialize libInstPatch */
  ipatch_init ();

  sf2 = ipatch_sf2_new ();	/* ++ ref new SoundFont object */

  /* loop over file names (command line args) */
  for (i = 1; i < argc; i++)
  {
    fname = argv[i];

    /* ++ identify file type and open handle to file object */
    fhandle = ipatch_file_identify_open (fname, &err);

    if (!fhandle)
    {
      fprintf (stderr, "Failed to identify file '%s': %s\n", fname,
               ipatch_gerror_message (err));
      g_clear_error (&err);
      continue;
    }

    /* try to convert file to a SoundFont sample */
    sample = ipatch_convert_object_to_type (G_OBJECT (fhandle->file),
                                            IPATCH_TYPE_SF2_SAMPLE, &err);
    if (!sample)
    {
      fprintf (stderr, "Failed to convert file '%s' to SoundFont sample: %s\n",
               fname, ipatch_gerror_message (err));
      g_clear_error (&err);
      ipatch_file_close (fhandle);  /* -- close file */
      continue;
    }

    /* append sample to SoundFont (ensure that its name is unique) */
    ipatch_container_add_unique (IPATCH_CONTAINER (sf2), IPATCH_ITEM (sample));

    g_object_get (sample, "name", &name, NULL); /* get the sample's name */

    /* create new SoundFont instrument (++ ref) */
    inst = ipatch_sf2_inst_new ();
    g_object_set (inst, "name", name, NULL); /* set instrument name */

    /* create new instrument zone and link sample to it */
    ipatch_sf2_inst_new_zone (inst, IPATCH_SF2_SAMPLE (sample));

    g_object_unref (sample); /* -- unref SoundFont sample */

    /* append instrument to SoundFont (ensure that its name is unique) */
    ipatch_container_add_unique (IPATCH_CONTAINER (sf2), IPATCH_ITEM (inst));

    /* create new SoundFont preset (++ ref) */
    preset = ipatch_sf2_preset_new ();
    g_object_set (preset, "name", name, NULL); /* set preset name */

    /* create new preset zone and link instrument to it */
    ipatch_sf2_preset_new_zone (preset, inst);

    g_object_unref (inst);	/* -- unref SoundFont instrument */

    /* append preset to SoundFont (ensure name/bank/preset # are unique) */
    ipatch_container_add_unique (IPATCH_CONTAINER (sf2), IPATCH_ITEM (preset));
    g_object_unref (preset);

    g_free (name);  /* free the name (returned from g_object_get) */

    ipatch_file_close (fhandle);  /* -- close file */
  }


  /* create SoundFont file object, set its name and open for writing */
  sffile = ipatch_sf2_file_new ();
  ipatch_file_set_name (IPATCH_FILE (sffile), "output.sf2");

  /* Save SoundFont to file using converter system */
  if (!ipatch_convert_objects (G_OBJECT (sf2), G_OBJECT (sffile), &err))
  {
    fprintf (stderr, "Failed to save SoundFont to file: %s\n",
             ipatch_gerror_message (err));
    g_clear_error (&err);
  }

  g_object_unref (sffile);	/* -- unref SoundFont file */
  g_object_unref (sf2);		/* -- unref SoundFont object */

  return (0);			/* we done, yeah! :) */
}
