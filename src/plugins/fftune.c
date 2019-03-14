/*
 * fftune.c - Fast Fourier Transform sample tuning plugin
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
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
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <glib.h>
#include <gmodule.h>

#include <fftw3.h>		/* part of the FFTW package */

#include <libinstpatch/libinstpatch.h>

#include <libswami/SwamiPlugin.h>
#include <libswami/version.h>

#include "fftune_i18n.h"

#include "fftune.h"

#define DEFAULT_THRESHOLD   0.1
#define DEFAULT_SEPARATION  20.0
#define DEFAULT_MIN_FREQ    20.0
#define DEFAULT_MAX_FREQ    14000.0
#define DEFAULT_MAX_TUNINGS 10

#define MAX_ALLOWED_TUNINGS 1024	/* absolute max tunings allowed */

/* Sample format used internally */
#define SAMPLE_FORMAT   IPATCH_SAMPLE_FLOAT | IPATCH_SAMPLE_MONO | IPATCH_SAMPLE_ENDIAN_HOST

enum
{
  PROP_0,
  PROP_ACTIVE,	/* when TRUE: spectrum and tuning suggestions are calculated
		   any changes in parameters cause recalc of tuning suggestions */
  PROP_SAMPLE,		/* sample object to calculate spectrum on */
  PROP_SAMPLE_MODE,	/* sample calculation mode enum */
  PROP_SAMPLE_START,	/* start position in sample */
  PROP_SAMPLE_END,	/* end position in sample */
  PROP_LIMIT,           /* maximum samples to process or 0 for unlimited */
  PROP_THRESHOLD,	/* power threshold for tuning suggestions */
  PROP_SEPARATION,	/* min freq between consecutive tuning suggestions */
  PROP_MIN_FREQ,	/* minimum frequency for tuning suggestions */
  PROP_MAX_FREQ,	/* maximum frequency for tuning suggestions */
  PROP_MAX_TUNINGS,	/* max number of tuning suggestions */

  /* properties for selecting and retrieving tuning suggestions */
  PROP_TUNE_SELECT,	/* for selecting tuning suggestions */
  PROP_TUNE_COUNT,	/* count of available tuning suggestions */
  PROP_TUNE_INDEX,	/* index in spectrum data for this tuning suggestion */
  PROP_TUNE_POWER,	/* power of current tuning (0.0 if no more suggestions) */
  PROP_TUNE_FREQ,	/* frequency of current tuning */
  PROP_ENABLE_WINDOW,   /* Enable Hann window of sample data */
  PROP_ELLAPSED_TIME    /* Ellapsed time of last execution in seconds */
};

enum
{
  SPECTRUM_CHANGE,	/* spectrum data change signal */
  TUNINGS_CHANGE,	/* tuning suggestions changed signal */
  SIGNAL_COUNT
};

#define ERRMSG_MALLOC_1  "Failed to allocate %u bytes in FFTune plugin"

static gboolean plugin_fftune_init (SwamiPlugin *plugin, GError **err);
static GType sample_mode_register_type (SwamiPlugin *plugin);
static void fftune_spectra_finalize (GObject *object);
static void fftune_spectra_set_property (GObject *object, guint property_id,
					 const GValue *value, GParamSpec *pspec);
static void fftune_spectra_get_property (GObject *object, guint property_id,
					 GValue *value, GParamSpec *pspec);
static gboolean fftune_spectra_calc_spectrum (FFTuneSpectra *spectra);
static double *fftune_spectra_run_fftw (void *data, int *dsize);
static gboolean fftune_spectra_calc_tunings (FFTuneSpectra *spectra);
static gint tuneval_compare_func (gconstpointer a, gconstpointer b);

/* set plugin information */
SWAMI_PLUGIN_INFO (plugin_fftune_init, NULL);

/* define FFTuneSpectra type */
G_DEFINE_TYPE (FFTuneSpectra, fftune_spectra, G_TYPE_OBJECT);


static GType sample_mode_enum_type;

static guint obj_signals[SIGNAL_COUNT];


static gboolean
plugin_fftune_init (SwamiPlugin *plugin, GError **err)
{
  /* bind the gettext domain */
#if defined(ENABLE_NLS)
  bindtextdomain ("SwamiPlugin-fftune", LOCALEDIR);
#endif

  g_object_set (plugin,
		"name", "FFTune",
		"version", "1.0",
		"author", "Element Green",
		"copyright", "Copyright (C) 2004-2014",
	"descr", N_("Fast Fourier Transform sample tuner"),
		"license", "GPL",
		NULL);

  /* register types */
  sample_mode_enum_type = sample_mode_register_type (plugin);
  fftune_spectra_get_type ();

  return (TRUE);
}

static GType
sample_mode_register_type (SwamiPlugin *plugin)
{
  static const GEnumValue values[] =
    {
      { FFTUNE_MODE_SELECTION, "FFTUNE_MODE_SELECTION", "Selection" },
      { FFTUNE_MODE_LOOP, "FFTUNE_MODE_LOOP", "Loop" },
      { 0, NULL, NULL }
    };
  static GTypeInfo enum_info = { 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL };

  /* initialize the type info structure for the enum type */
  g_enum_complete_type_info (G_TYPE_ENUM, &enum_info, values);

  return (g_type_module_register_type (G_TYPE_MODULE (plugin), G_TYPE_ENUM,
				       "FFTuneSampleMode", &enum_info, 0));
}

static void
fftune_spectra_class_init (FFTuneSpectraClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->set_property = fftune_spectra_set_property;
  obj_class->get_property = fftune_spectra_get_property;
  obj_class->finalize = fftune_spectra_finalize;

  /* This signal gets emitted when spectrum data changes.  The pointer argument
     is the new spectrum data (array of doubles), after this signal is emitted
     the old spectrum data should no longer be accessed.  The unsigned int is
     the array size. */
  obj_signals[SPECTRUM_CHANGE] =
    g_signal_new ("spectrum-change", G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST, 0, NULL, NULL,
		  g_cclosure_marshal_VOID__UINT_POINTER,
		  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);

  /* signal emitted when tuning suggestions change, first parameter is the
     count of tuning suggestions (equivalent to "tune-count") */
  obj_signals[TUNINGS_CHANGE] =
    g_signal_new ("tunings-change", G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST, 0, NULL, NULL,
		  g_cclosure_marshal_VOID__UINT,
		  G_TYPE_NONE, 1, G_TYPE_UINT);

  g_object_class_install_property (obj_class, PROP_ACTIVE,
	    g_param_spec_boolean ("active", _("Active"), _("Active"),
				  FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAMPLE,
	    g_param_spec_object ("sample", _("Sample"), _("Sample"),
				 IPATCH_TYPE_SAMPLE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAMPLE_MODE,
	    g_param_spec_enum ("sample-mode", _("Sample mode"),
			       _("Sample spectrum mode"),
			       sample_mode_enum_type, FFTUNE_MODE_SELECTION,
			       G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAMPLE_START,
	    g_param_spec_uint ("sample-start", _("Sample start"), _("Sample start"),
			       0, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAMPLE_END,
	    g_param_spec_uint ("sample-end", _("Sample end"), _("Sample end"),
			       0, G_MAXUINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LIMIT,
	    g_param_spec_uint ("limit", _("Limit"), _("Limit"),
			       0, G_MAXUINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_THRESHOLD,
	    g_param_spec_float ("threshold", _("Threshold"),
				_("Min ratio to max power of tuning suggestions"),
				0.0, 1.0, DEFAULT_THRESHOLD,
				G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SEPARATION,
	    g_param_spec_float ("separation", _("Separation"),
				_("Min frequency separation between tunings"),
				0.0, 24000.0, DEFAULT_SEPARATION,
				G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MIN_FREQ,
	    g_param_spec_float ("min-freq", _("Min frequency"),
				_("Min frequency of tuning suggestions"),
				0.0, 24000.0, DEFAULT_MIN_FREQ,
				G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MAX_FREQ,
	    g_param_spec_float ("max-freq", _("Max frequency"),
				_("Max frequency of tuning suggestions"),
				0.0, 24000.0, DEFAULT_MAX_FREQ,
				G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MAX_TUNINGS,
	    g_param_spec_int ("max-tunings", _("Max tunings"),
			      _("Max tuning suggestions"),
			      0, MAX_ALLOWED_TUNINGS, DEFAULT_MAX_TUNINGS,
			      G_PARAM_READWRITE));

  g_object_class_install_property (obj_class, PROP_TUNE_SELECT,
	    g_param_spec_int ("tune-select", _("Tune select"),
			      _("Select tuning suggestion by index"),
			      0, MAX_ALLOWED_TUNINGS, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TUNE_COUNT,
	    g_param_spec_int ("tune-count", _("Tune count"),
			      _("Count of tuning suggestions"),
			      0, MAX_ALLOWED_TUNINGS, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TUNE_INDEX,
	    g_param_spec_int ("tune-index", _("Tune index"),
			      _("Spectrum index of current tuning"),
			      0, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TUNE_POWER,
	    g_param_spec_double ("tune-power", _("Tune power"),
				 _("Power of current tuning"),
				 0.0, G_MAXDOUBLE, 0.0, G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_TUNE_FREQ,
	    g_param_spec_double ("tune-freq", _("Tune frequency"),
				 _("Frequency of current tuning"),
				 0.0, G_MAXDOUBLE, 0.0, G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_ENABLE_WINDOW,
	    g_param_spec_boolean ("enable-window", _("Enable window"), _("Enable window"),
				  FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ELLAPSED_TIME,
	    g_param_spec_float ("ellapsed-time", _("Ellapsed time"),
				_("Ellapsed time of last execution in seconds"),
				0.0, G_MAXFLOAT, 0.0, G_PARAM_READABLE));
}

static void
fftune_spectra_init (FFTuneSpectra *spectra)
{
  spectra->sample_mode = FFTUNE_MODE_SELECTION;
  spectra->threshold = DEFAULT_THRESHOLD;
  spectra->separation = DEFAULT_SEPARATION;
  spectra->min_freq = DEFAULT_MIN_FREQ;
  spectra->max_freq = DEFAULT_MAX_FREQ;
  spectra->max_tunings = DEFAULT_MAX_TUNINGS;
}

static void
fftune_spectra_finalize (GObject *object)
{
  FFTuneSpectra *spectra = FFTUNE_SPECTRA (object);

  if (spectra->spectrum) fftwf_free (spectra->spectrum);
  if (spectra->sample) g_object_unref (spectra->sample);
  if (spectra->tunevals) 
    g_free (spectra->tunevals);
}

static void
fftune_spectra_set_property (GObject *object, guint property_id,
			     const GValue *value, GParamSpec *pspec)
{
  FFTuneSpectra *spectra = FFTUNE_SPECTRA (object);
  gboolean spectrum_update = FALSE;
  gboolean tunings_update = FALSE;

  switch (property_id)
    {
    case PROP_ACTIVE:
      if (!spectra->active && g_value_get_boolean (value))  /* activating? */
	spectrum_update = TRUE;

      spectra->active = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE:
      if (g_value_get_object (value) == spectra->sample) break;

      if (spectra->sample) g_object_unref (spectra->sample);
      spectra->sample = g_value_get_object (value);
      if (spectra->sample) g_object_ref (spectra->sample);

      spectrum_update = TRUE;
      break;
    case PROP_SAMPLE_MODE:
      if (g_value_get_enum (value) == spectra->sample_mode) break;

      spectra->sample_mode = g_value_get_enum (value);
      spectrum_update = TRUE;
      break;
    case PROP_SAMPLE_START:
      if (g_value_get_uint (value) == spectra->sample_start) break;

      spectra->sample_start = g_value_get_uint (value);
      spectrum_update = TRUE;
      break;
    case PROP_SAMPLE_END:
      if (g_value_get_uint (value) == spectra->sample_end) break;

      spectra->sample_end = g_value_get_uint (value);
      spectrum_update = TRUE;
      break;
    case PROP_LIMIT:
      if (g_value_get_uint (value) == spectra->limit) break;

      spectra->limit = g_value_get_uint (value);
      spectrum_update = TRUE;
      break;
    case PROP_THRESHOLD:
      if (g_value_get_float (value) == spectra->threshold) break;

      spectra->threshold = g_value_get_float (value);
      tunings_update = TRUE;
      break;
    case PROP_SEPARATION:
      if (g_value_get_float (value) == spectra->separation) break;

      spectra->separation = g_value_get_float (value);
      tunings_update = TRUE;
      break;
    case PROP_MIN_FREQ:
      if (g_value_get_float (value) == spectra->min_freq) break;

      spectra->min_freq = g_value_get_float (value);
      tunings_update = TRUE;
      break;
    case PROP_MAX_FREQ:
      if (g_value_get_float (value) == spectra->max_freq) break;

      spectra->max_freq = g_value_get_float (value);
      tunings_update = TRUE;
      break;
    case PROP_MAX_TUNINGS:
      if (g_value_get_int (value) == spectra->max_tunings) break;

      spectra->max_tunings = g_value_get_int (value);
      tunings_update = TRUE;
      break;
    case PROP_TUNE_SELECT:
      spectra->tune_select = g_value_get_int (value);
      break;
    case PROP_ENABLE_WINDOW:
      if (g_value_get_boolean (value) == spectra->enable_window) break;

      spectra->enable_window = g_value_get_boolean (value);
      spectrum_update = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (spectra->active && spectra->sample)
  {
    if (spectrum_update)	/* spectrum need update? */
      {
	if (fftune_spectra_calc_spectrum (spectra))
	  fftune_spectra_calc_tunings (spectra);
      }
    else if (tunings_update)	/* tuning values need update? */
      fftune_spectra_calc_tunings (spectra);
  }
}

static void
fftune_spectra_get_property (GObject *object, guint property_id,
			     GValue *value, GParamSpec *pspec)
{
  FFTuneSpectra *spectra = FFTUNE_SPECTRA (object);
  int index;

  switch (property_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, spectra->active);
      break;
    case PROP_SAMPLE:
      g_value_set_object (value, spectra->sample);
      break;
    case PROP_SAMPLE_MODE:
      g_value_set_enum (value, spectra->sample_mode);
      break;
    case PROP_SAMPLE_START:
      g_value_set_uint (value, spectra->sample_start);
      break;
    case PROP_SAMPLE_END:
      g_value_set_uint (value, spectra->sample_end);
      break;
    case PROP_LIMIT:
      g_value_set_uint (value, spectra->limit);
      break;
    case PROP_THRESHOLD:
      g_value_set_float (value, spectra->threshold);
      break;
    case PROP_SEPARATION:
      g_value_set_float (value, spectra->separation);
      break;
    case PROP_MIN_FREQ:
      g_value_set_float (value, spectra->min_freq);
      break;
    case PROP_MAX_FREQ:
      g_value_set_float (value, spectra->max_freq);
      break;
    case PROP_MAX_TUNINGS:
      g_value_set_int (value, spectra->max_tunings);
      break;
    case PROP_TUNE_SELECT:
      g_value_set_int (value, spectra->tune_select);
      break;
    case PROP_TUNE_COUNT:
      g_value_set_int (value, spectra->n_tunevals);
      break;
    case PROP_TUNE_INDEX:
      if (spectra->tunevals && spectra->tune_select < spectra->n_tunevals)
	g_value_set_int (value, spectra->tunevals[spectra->tune_select]);
      else g_value_set_int (value, 0);
      break;
    case PROP_TUNE_POWER:
      if (spectra->tunevals && spectra->tune_select < spectra->n_tunevals
	  && spectra->spectrum)
	{
	  index = spectra->tunevals[spectra->tune_select];
	  g_value_set_double (value, spectra->spectrum[index]);
	}
      else g_value_set_double (value, 0.0);
      break;
    case PROP_TUNE_FREQ:
      if (spectra->tunevals && spectra->tune_select < spectra->n_tunevals
	  && spectra->spectrum)
	{
	  index = spectra->tunevals[spectra->tune_select];
	  g_value_set_double (value, spectra->freqres * index);
	}
      else g_value_set_double (value, 0.0);
      break;
    case PROP_ENABLE_WINDOW:
      g_value_set_boolean (value, spectra->enable_window);
      break;
    case PROP_ELLAPSED_TIME:
      g_value_set_float (value, spectra->ellapsed_time);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
fftune_spectra_calc_spectrum (FFTuneSpectra *spectra)
{
  guint start, sample_size, loop_start, loop_end;
  IpatchSampleHandle handle;
  GError *err = NULL;
  void *data;           /* Stores sample data (floats) */
  double *result;       /* FFT power spectrum result */
  int dsize, result_size;
  int count, i;
  GTimeVal start_time, end_time;
  float ellapsed;

  g_return_val_if_fail (spectra->sample != NULL, FALSE);

  g_object_get (spectra->sample,
		"sample-size", &sample_size,
		"loop-start", &loop_start,
		"loop-end", &loop_end,
		NULL);

  if (spectra->sample_mode == FFTUNE_MODE_LOOP)	/* loop mode? */
    {
      start = loop_start;

      count = loop_end - loop_start;   /* number of samples in loop */
      dsize = count * 2 + 1;	/* 2 iterations + 1 (first sample appended) */
    }
  else	/* selection mode */
    {
      if (spectra->sample_start == 0 && spectra->sample_end == 0)
	{
	  start = 0;
	  count = sample_size;
	}
      else
	{
	  if (spectra->sample_start <= spectra->sample_end)
	    {
	      start = spectra->sample_start;
	      count = spectra->sample_end - spectra->sample_start + 1;
	    }
	  else	/* swap start/end if backwards */
	    {
	      start = spectra->sample_end;
	      count = spectra->sample_start - spectra->sample_end + 1;
	    }
	}

      if (spectra->limit && count > spectra->limit) count = spectra->limit;

      dsize = count;
    }

  /* Allocate sample/result buffer, sample data is stored as floats and result
   * is stored as doubles (dsize / 2 + 1 in length). */
  data = g_try_malloc (sizeof (double) * (dsize / 2 + 1)); /* allocate transform array */

  if (!data)
  {
    g_critical (_(ERRMSG_MALLOC_1), (guint)(sizeof (double) * (dsize / 2 + 1)));
    return (FALSE);
  }

  if (!ipatch_sample_handle_open (spectra->sample, &handle, 'r', SAMPLE_FORMAT,
                                  IPATCH_SAMPLE_UNITY_CHANNEL_MAP, &err))
  {
    g_critical ("Failed to open sample in FFTune plugin: %s",
                ipatch_gerror_message (err));
    g_error_free (err);
    g_free (data);
    return (FALSE);
  }

  /* load the sample data */
  if (!ipatch_sample_handle_read (&handle, start, count, data, &err))
  {
    g_critical ("Failed to read sample data in FFTune plugin: %s",
                ipatch_gerror_message (err));
    g_error_free (err);
    ipatch_sample_handle_close (&handle);
    g_free (data);
    return (FALSE);
  }

  ipatch_sample_handle_close (&handle);

  if (spectra->sample_mode == FFTUNE_MODE_LOOP)	/* do 2 iterations for loops */
  {
    for (i=0; i < count; i++)
      ((float *)data)[i + count] = ((float *)data)[i];

    /* append first sample to end to complete the 2 cycles */
    ((float *)data)[dsize - 1] = ((float *)data)[0];
  }

  result_size = dsize;

  g_get_current_time (&start_time);

  if (spectra->enable_window)
  {
    for (i = 0; i < dsize; i++)
      ((float *)data)[i] *= 0.5 * (1.0 - cos (2.0 * G_PI * ((float)i / (dsize - 1)))); 
  }

  /* Calculate FFT power spectrum of sample data. Result is returned in the same array. */
  result = fftune_spectra_run_fftw (data, &result_size);

  g_get_current_time (&end_time);

  ellapsed = (end_time.tv_sec - start_time.tv_sec)
    + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

  spectra->ellapsed_time = ellapsed;

  if (!result)
  {
    g_free (data);
    return (FALSE);
  }

  /* emit spectrum-change signal */
  g_signal_emit (spectra, obj_signals[SPECTRUM_CHANGE], 0, result_size, result);

  g_free (spectra->spectrum);
  spectra->spectrum = result;
  spectra->spectrum_size = result_size;

  return (TRUE);
}

static double *
fftune_spectra_run_fftw (void *data, int *dsize)
{
  fftwf_plan fftplan;
  float *outdata;
  int size = *dsize;
  int outsize;
  int i, x;

  outsize = size / 2 + 1;
  outdata = fftwf_malloc (sizeof (float) * size); /* allocate output transform array */

  if (!outdata)
  {
    g_critical (_(ERRMSG_MALLOC_1), (guint)(sizeof (float) * size));
    return (NULL);
  }

  /* create FFTW plan (real to half complex, in place transform) */
  fftplan = fftwf_plan_r2r_1d (size, data, outdata, FFTW_R2HC, FFTW_ESTIMATE);
  fftwf_execute (fftplan);	/* do the FFT calculation */
  fftwf_destroy_plan (fftplan);

  /* compute power spectrum (its stored over the output array) */
  ((double *)data)[0] = outdata[0] * outdata[0];	/* DC component */

  x = (size+1) / 2;
  for (i=1; i < x; ++i)	/* (i < size/2 rounded up) */
    ((double *)data)[i] = outdata[i] * outdata[i] + outdata[size - i] * outdata[size - i];

  if (size % 2 == 0) /* count is even? */
    ((double *)data)[size / 2] = outdata[size / 2] * outdata[size / 2]; /* Nyquist freq. */

  fftwf_free (outdata);

  *dsize = outsize;

  /* Resize data array to size of FFT power data */
  data = realloc (data, sizeof (double) * outsize);

  return (data);
}

/* typebag for temporary sorted GList of tunings.  Wouldn't need it if
 * g_list_insert_sorted had a user data parameter -:( */
typedef struct
{
  double *spectrum;
  int index;
} TuneBag;

static gboolean
fftune_spectra_calc_tunings (FFTuneSpectra *spectra)
{
  GList *sugvals = NULL, *p;	/* list of spectrum indexes */
  double *spectrum;
  TuneBag *bag;
  int sugcount = 0;
  int tolndx, tolcount;
  double max, val, full_max, threshold;
  int i, start, stop, maxndx;
  int sample_rate;

  g_return_val_if_fail (spectra->sample != NULL, FALSE);
  g_return_val_if_fail (spectra->spectrum != NULL, FALSE);

  spectrum = spectra->spectrum;
  threshold = spectra->threshold;

  g_object_get (spectra->sample,
		"sample-rate", &sample_rate,
		NULL);

  /* frequency resolution (frequency difference between array indexes) */
  spectra->freqres = (double)sample_rate / ((spectra->spectrum_size - 1) * 2);

  /* separation amount in array index units for selecting unique tunings */
  tolndx = spectra->separation / spectra->freqres + 0.5;

  /* don't care about anything below min_freq */
  start = (int)(spectra->min_freq / spectra->freqres) + 1;
  start = CLAMP (start, 0, spectra->spectrum_size - 1);

  /* don't care about anything above max_freq */
  stop = spectra->max_freq / spectra->freqres;
  stop = CLAMP (stop, 0, spectra->spectrum_size - 1);

  /* get maximum power in frequency range (full_max) */
  for (i = start, full_max = 0.0; i <= stop; i++)
    {
      val = spectrum[i];
      if (val > full_max) full_max = val;
    }

  if (full_max == 0.0) full_max = 1.0;	/* div/0 protection */

/*
  printf ("spectrum_size = %d, freqres = %0.2f, tolndx = %d, start = %d, stop = %d\n",
	  spectra->spectrum_size, spectra->freqres, tolndx, start, stop);
*/

  tolcount = tolndx;		/* separation counter */
  max = 0.0;			/* current maximum power */
  maxndx = -1;			/* spectrum index of current maximum */

  for (i = start; i <= stop; i++)
    {
      val = spectra->spectrum[i];

      /* power of frequency exceeds full_max threshold ratio? */
      if (val / full_max >= threshold)
	{
	  if (val > max) /* find frequency with most power */
	    {			/* new maximum */
	      max = val;
	      maxndx = i;
	    }
	  tolcount = tolndx;	/* reset separation counter */
	}

      /* Do we have a frequency suggestion and no threshold, last index or
	 separation counter expired? */
      if (maxndx >= 0 && (threshold == 0.0 || tolcount-- <= 0 || i == stop - 1))
	{
	  /* if max tunevals exceeded, remove the least powerful one
	     i.e. the little guy gets sacked. */
	  if (sugcount == spectra->max_tunings)
	    {
	      bag = sugvals->data;	/* steal the TuneBag */
	      sugvals = g_list_delete_link (sugvals, sugvals);
	    }
	  else
	    {
	      bag = g_new (TuneBag, 1);
	      bag->spectrum = spectra->spectrum;
	      sugcount++;
	    }

	  bag->index = maxndx;

	  /* add to tuning suggestion list using sort function (reverse sort) */
	  sugvals = g_list_insert_sorted (sugvals, bag,
					  (GCompareFunc)tuneval_compare_func);

	  /* reset variables */
	  tolcount = tolndx;
	  max = 0.0;
	  maxndx = -1;
	}
    }

  spectra->n_tunevals = sugcount;

  if (sugcount > 0)
    {
      spectra->tunevals = g_realloc (spectra->tunevals, sizeof (int) * sugcount);

      /* list is reverse sorted, so copy to descending array indexes */
      for (i = sugcount - 1, p = sugvals; i >= 0; i--)
	{
	  bag = (TuneBag *)(p->data);
	  spectra->tunevals[i] = bag->index;
	  g_free (bag);
	  p = g_list_delete_link (p, p);
	}
    }
  else
    {
      g_free (spectra->tunevals);
      spectra->tunevals = NULL;
    }

  spectra->tune_select = 0;

  /* emit tunings-change signal */
  g_signal_emit (spectra, obj_signals[TUNINGS_CHANGE], 0, sugcount);

  return (TRUE);
}

/* reverse sorted, so that tuneval with smallest value is always first for
   quick removal */
static gint
tuneval_compare_func (gconstpointer a, gconstpointer b)
{
  TuneBag *abag = (TuneBag *)a, *bbag = (TuneBag *)b;
  double f;

  f = abag->spectrum[abag->index] - bbag->spectrum[bbag->index];
  if (f < 0.0) return -1;
  if (f > 0.0) return 1;
  return 0;
}
