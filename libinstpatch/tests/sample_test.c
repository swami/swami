/*
 * sample_test.c - Sample audio conversion tests
 *
 * Tests every combination of audio format conversions (484 combinations).
 * This is done by creating a double audio format triangle waveform and then
 * for each transformation format pair, converting this waveform to the
 * first format, then the second, then back to double again and comparing
 * against the original.
 *
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */
#include <libinstpatch/libinstpatch.h>

#define DEFAULT_AUDIO_SIZE	(32 * 1024)	/* default test waveform size in samples */
#define WAVEFORM_PERIOD		1684		/* something to exercise LSB bytes too */
#define MAX_DIFF_ALLOWED	0.016		/* maximum difference allowed */

#define WAVEFORM_QUARTER	(WAVEFORM_PERIOD / 4)

/* all available sample format combinations */
int testformats[] = {
  IPATCH_SAMPLE_8BIT,
  IPATCH_SAMPLE_16BIT,
  IPATCH_SAMPLE_24BIT,
  IPATCH_SAMPLE_32BIT,
  IPATCH_SAMPLE_FLOAT,
  IPATCH_SAMPLE_DOUBLE,
  IPATCH_SAMPLE_REAL24BIT,

  IPATCH_SAMPLE_8BIT | IPATCH_SAMPLE_UNSIGNED,
  IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_UNSIGNED,
  IPATCH_SAMPLE_24BIT | IPATCH_SAMPLE_UNSIGNED,
  IPATCH_SAMPLE_32BIT | IPATCH_SAMPLE_UNSIGNED,
  IPATCH_SAMPLE_REAL24BIT | IPATCH_SAMPLE_UNSIGNED,

  IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_BENDIAN,
  IPATCH_SAMPLE_24BIT | IPATCH_SAMPLE_BENDIAN,
  IPATCH_SAMPLE_32BIT | IPATCH_SAMPLE_BENDIAN,
  IPATCH_SAMPLE_FLOAT | IPATCH_SAMPLE_BENDIAN,
  IPATCH_SAMPLE_DOUBLE | IPATCH_SAMPLE_BENDIAN,
  IPATCH_SAMPLE_REAL24BIT | IPATCH_SAMPLE_BENDIAN,

  IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_UNSIGNED | IPATCH_SAMPLE_BENDIAN,
  IPATCH_SAMPLE_24BIT | IPATCH_SAMPLE_UNSIGNED | IPATCH_SAMPLE_BENDIAN,
  IPATCH_SAMPLE_32BIT | IPATCH_SAMPLE_UNSIGNED | IPATCH_SAMPLE_BENDIAN,
  IPATCH_SAMPLE_REAL24BIT | IPATCH_SAMPLE_UNSIGNED | IPATCH_SAMPLE_BENDIAN
};

#define SAMPLE_FORMAT_COUNT  G_N_ELEMENTS (testformats)

static int test_size = DEFAULT_AUDIO_SIZE;
static gboolean verbose = FALSE;

static GOptionEntry entries[] =
{
  { "size", 's', 0, G_OPTION_ARG_INT, &test_size, "Size of test waveform in samples (default=32768)", NULL },
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },
  { NULL }
};

int main (int argc, char *argv[])
{
  IpatchSampleTransform *trans;
  gpointer srctrans, desttrans;
  double *dwave, *fintrans;
  guint periodpos;
  int srcform, destform;
  int isrc, idest;
  double d, maxdiff;
  int i, maxindex, failcount = 0;
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new ("- test libInstPatch sample format conversions");
  g_option_context_add_main_entries (context, entries, "sample_test");

  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    printf ("Failed to parse command line arguments: %s\n",
	    ipatch_gerror_message (error));
    return 1;
  }

  g_option_context_free (context);

  ipatch_init ();

  dwave = g_new (double, test_size);	/* allocate audio buffer (double format) */
  srctrans = g_malloc (8 * test_size);	/* source format transform buf */
  desttrans = g_malloc (8 * test_size);	/* destination format transform buf */
  fintrans = g_malloc (8 * test_size);	/* final transform buf */

  /* create sample transform object and allocate its buffer */
  trans = ipatch_sample_transform_new (0, 0, 0);
  ipatch_sample_transform_alloc_size (trans, 32 * 1024);

  /* generate triangle waveform */
  for (i = 0; i < test_size; i++)
  {
    periodpos = i % WAVEFORM_PERIOD;

    if (periodpos <= WAVEFORM_QUARTER)
      dwave[i] = periodpos / (double)WAVEFORM_QUARTER;
    else if (periodpos <= WAVEFORM_QUARTER * 3)
      dwave[i] = -((periodpos - WAVEFORM_QUARTER) / (double)WAVEFORM_QUARTER - 1.0);
    else dwave[i] = (periodpos - WAVEFORM_QUARTER * 3) / (double)WAVEFORM_QUARTER - 1.0;
  }

  /* Convert between all possible format pairs using the following steps:
   * - Convert double to first format of format pair
   * - Convert from first format to second format
   * - Convert second format back to double
   * - Calculate maximum sample difference between new double audio and original
   */

  for (isrc = 0; isrc < SAMPLE_FORMAT_COUNT; isrc++)
  {
    srcform = testformats[isrc];

    for (idest = 0; idest < SAMPLE_FORMAT_COUNT; idest++)
    {
      destform = testformats[idest];

      /* convert double waveform to source format */
      ipatch_sample_transform_set_formats (trans, IPATCH_SAMPLE_DOUBLE
					   | IPATCH_SAMPLE_ENDIAN_HOST, srcform,
                                           IPATCH_SAMPLE_UNITY_CHANNEL_MAP);
      ipatch_sample_transform_convert (trans, dwave, srctrans, test_size);

      /* convert source format to destination format */
      ipatch_sample_transform_set_formats (trans, srcform, destform,
                                           IPATCH_SAMPLE_UNITY_CHANNEL_MAP);
      ipatch_sample_transform_convert (trans, srctrans, desttrans, test_size);

      /* convert destination format to final double output */
      ipatch_sample_transform_set_formats (trans, destform, IPATCH_SAMPLE_DOUBLE
					   | IPATCH_SAMPLE_ENDIAN_HOST,
                                           IPATCH_SAMPLE_UNITY_CHANNEL_MAP);
      ipatch_sample_transform_convert (trans, desttrans, fintrans, test_size);

      /* compare final waveform against original */
      for (i = 0, maxdiff = 0.0, maxindex = 0; i < test_size; i++)
      {
	d = dwave[i] - fintrans[i];
	if (d < 0.0) d = -d;

	if (d > maxdiff)
	{
	  maxdiff = d;
	  maxindex = i;
	}
      }

      if (verbose)
	printf ("Convert format %03x to %03x%s: maxdiff=%0.6f, sample=%d\n",
		srcform, destform, maxdiff > MAX_DIFF_ALLOWED ? " FAILED" : "",
		maxdiff, maxindex);

      if (maxdiff > MAX_DIFF_ALLOWED)
      {
	if (!verbose)
	  printf ("Convert format %03x to %03x FAILED: maxdiff=%0.6f, sample=%d\n",
		  srcform, destform, maxdiff, maxindex);
	failcount++;
      }
    }
  }

  ipatch_sample_transform_free (trans);

  g_free (dwave);
  g_free (srctrans);
  g_free (desttrans);
  g_free (fintrans);

  if (failcount > 0)
  {
    printf ("%d of %d format conversions FAILED\n", failcount,
	    SAMPLE_FORMAT_COUNT * SAMPLE_FORMAT_COUNT);
    return 1;
  }

  printf ("All %d sample format conversions PASSED\n",
	  SAMPLE_FORMAT_COUNT * SAMPLE_FORMAT_COUNT);

  return 0;
}
