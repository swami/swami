/*
 * sample_list_test.c - Tests sample edit lists and virtual sample store
 *
 * - Creates double stereo format test waveform with triangle wave in left
 *   channel and sine wave in right channel
 * - Re-constructs both channels using 2 sample lists using various list
 *   operations
 * - Creates virtual sample store using double stereo format with left
 *   and right sample lists
 * - Duplicates virtual sample store to a double stereo format store
 * - Compares final duplicated store to original waveform and makes sure its
 *   the same
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
#include <math.h>

#define DEFAULT_AUDIO_SIZE	(32 * 1024)	/* default test waveform size in samples */
#define WAVEFORM_PERIOD		1684		/* something to exercise LSB bytes too */
#define MAX_DIFF_ALLOWED	0.0		/* maximum difference allowed */

#define WAVEFORM_QUARTER	(WAVEFORM_PERIOD / 4)


static int test_size = DEFAULT_AUDIO_SIZE;
static gboolean verbose = FALSE;

static GOptionEntry entries[] =
{
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },
  { NULL }
};

int main (int argc, char *argv[])
{
  IpatchSampleStore *store, *vstore, *finstore;
  IpatchSampleList *rlist, *llist;
  IpatchSampleData *data, *vdata;
  double *dwave, *findata;
  guint periodpos;
  double d, maxdiff;
  int i, maxindex;
  GError *error = NULL;
  GOptionContext *context;
  int test_size_q;

  context = g_option_context_new ("- test libInstPatch sample edit list functions");
  g_option_context_add_main_entries (context, entries, "sample_list_test");

  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    printf ("Failed to parse command line arguments: %s\n",
	    ipatch_gerror_message (error));
    return 1;
  }

  g_option_context_free (context);

  test_size_q = test_size / 4;	/* test size quarter */

  ipatch_init ();

  /* allocate audio buffer (double format stereo) */
  dwave = g_new (double, test_size * 2);	/* ++ alloc */

  /* generate triangle waveform for left channel and sine wave for right */
  for (i = 0; i < test_size * 2; i += 2)
  {
    periodpos = i % WAVEFORM_PERIOD;

    if (periodpos <= WAVEFORM_QUARTER)
      dwave[i] = periodpos / (double)WAVEFORM_QUARTER;
    else if (periodpos <= WAVEFORM_QUARTER * 3)
      dwave[i] = -((periodpos - WAVEFORM_QUARTER) / (double)WAVEFORM_QUARTER - 1.0);
    else dwave[i] = (periodpos - WAVEFORM_QUARTER * 3) / (double)WAVEFORM_QUARTER - 1.0;

    dwave[i + 1] = sin ((periodpos / (double)WAVEFORM_PERIOD) * G_PI * 2.0);
  }

  data = ipatch_sample_data_new (test_size);	/* ++ ref */

  /* ++ ref - create sample store object for original waveform */
  store = ipatch_sample_store_new (IPATCH_TYPE_SAMPLE_STORE_RAM, data,
				   "format", IPATCH_SAMPLE_DOUBLE
				   | IPATCH_SAMPLE_STEREO
				   | IPATCH_SAMPLE_ENDIAN_HOST,
				   "location", dwave,
				   "active", TRUE,
				   NULL);

  rlist = ipatch_sample_list_new ();

  /* re-construct right channel in a very inefficient way ;) */
  ipatch_sample_list_prepend (rlist, store, test_size_q * 2, test_size_q,
			      IPATCH_SAMPLE_LIST_CHAN_RIGHT);
  ipatch_sample_list_prepend (rlist, store, 0, test_size_q,
			      IPATCH_SAMPLE_LIST_CHAN_RIGHT);
  ipatch_sample_list_insert_index (rlist, 1, store, test_size_q, test_size_q,
				   IPATCH_SAMPLE_LIST_CHAN_RIGHT);
  ipatch_sample_list_append (rlist, store, test_size_q * 3, test_size_q,
			     IPATCH_SAMPLE_LIST_CHAN_RIGHT);
  /* cut a segment out which overlaps segments and then re-insert the cut seg */
  ipatch_sample_list_cut (rlist, test_size_q + test_size_q / 2, test_size_q);
  ipatch_sample_list_insert (rlist, test_size_q + test_size_q / 2,
			     store, test_size_q + test_size_q / 2,
			     test_size_q, IPATCH_SAMPLE_LIST_CHAN_RIGHT);

  llist = ipatch_sample_list_new ();

  /* have fun with left channel too */
  ipatch_sample_list_append (llist, store, 0, test_size,
			     IPATCH_SAMPLE_LIST_CHAN_LEFT);
  ipatch_sample_list_cut (llist, test_size_q, test_size_q);
  ipatch_sample_list_insert (llist, test_size_q, store, test_size_q,
			     test_size_q, IPATCH_SAMPLE_LIST_CHAN_LEFT);

  vdata = ipatch_sample_data_new (test_size);	/* ++ ref */

  /* ++ ref - create virtual store from left and right sample lists */
  vstore = ipatch_sample_store_new (IPATCH_TYPE_SAMPLE_STORE_VIRTUAL, vdata,
				    "format", IPATCH_SAMPLE_DOUBLE
				    | IPATCH_SAMPLE_STEREO
				    | IPATCH_SAMPLE_ENDIAN_HOST,
				    NULL);
  ipatch_sample_store_virtual_set_list (vstore, 0, llist);
  ipatch_sample_store_virtual_set_list (vstore, 1, rlist);
  ipatch_sample_store_activate (vstore);

  /* ++ ref - Duplicate store to render final waveform */
  finstore = ipatch_sample_store_duplicate (vstore, IPATCH_TYPE_SAMPLE_STORE_RAM,
					    IPATCH_SAMPLE_DOUBLE
					    | IPATCH_SAMPLE_STEREO
					    | IPATCH_SAMPLE_ENDIAN_HOST, &error);
  if (!finstore)
  {
    printf ("Failed to create new duplicate sample store: %s",
	    ipatch_gerror_message (error));
    return 1;
  }

  findata = ipatch_sample_store_RAM_get_location (finstore);

  /* compare final waveform against original */
  for (i = 0, maxdiff = 0.0, maxindex = 0; i < test_size * 2; i++)
  {
    d = dwave[i] - findata[i];
    if (d < 0.0) d = -d;

    if (d > maxdiff)
    {
      maxdiff = d;
      maxindex = i;
    }
  }

  /* free up objects and data (for memcheck) */
  g_object_unref (data);
  g_object_unref (vdata);
  g_object_unref (store);
  g_object_unref (vstore);
  g_object_unref (finstore);
  g_free (dwave);

  if (maxdiff > MAX_DIFF_ALLOWED)
  {
    printf ("Sample list test failed: maxdiff=%0.16f index=%d\n",
	    maxdiff, maxindex);
    return 1;
  }

  if (verbose)
    printf ("Sample list test passed: maxdiff=%0.16f index=%d\n",
	    maxdiff, maxindex);
  else printf ("Sample list and virtual sample store test passed\n");

  return 0;
}
