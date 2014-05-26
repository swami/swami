/*
 * fftune.h - Header file for FFTW sample tuning plugin
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
 */
#ifndef __PLUGIN_FFTUNE_H__
#define __PLUGIN_FFTUNE_H__

#include <glib.h>
#include <libinstpatch/libinstpatch.h>

#define FFTUNE_TYPE_SPECTRA   (fftune_spectra_get_type ())
#define FFTUNE_SPECTRA(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FFTUNE_TYPE_SPECTRA, FFTuneSpectra))
#define FFTUNE_SPECTRA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), FFTUNE_TYPE_SPECTRA, FFTuneSpectraClass))
#define FFTUNE_IS_SPECTRA(obj) \
  (G_TYPE_CHECK_INSTANCE ((obj), FFTUNE_TYPE_SPECTRA))
#define FFTUNE_IS_SPECTRA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS ((klass), FFTUNE_TYPE_SPECTRA))

/* size in bytes for sample copy buffer */
#define FFTUNE_SAMPLE_COPY_BUFFER_SIZE  (32 * 1024)

/* FFT power spectrum object */
typedef struct
{
  GObject parent;

  gboolean active;	/* if TRUE, then change of params will update spectrum */
  IpatchSample *sample;	/* sample to calculate spectrum on */
  int sample_mode;	/* FFTUNE_MODE_* */
  guint sample_start;	/* start of selection (FFTUNE_MODE_SELECTION) */
  guint sample_end;	/* end of selection (start/end=0 will use entire sample) */
  guint limit;          /* maximum number of samples to process or 0 for unlimited */

  double *spectrum;	/* spectrum power data */
  int spectrum_size;	/* size of spectrum array */
  double freqres;	/* freq difference between consecutive spectrum indexes */
  double max_index;	/* index in spectrum of maximum power */

  int *tunevals;	/* array of spectrum indexes for tuning suggestions */
  int n_tunevals;	/* size of tunevals */
  int tune_select;	/* selected index in tunevals */

  float threshold;	/* power threshold for potential tuning suggestions */
  float separation;	/* min freq spread between suggestions */
  float min_freq;	/* min freq for tuning suggestions */
  float max_freq;	/* max freq for tuning suggestions */
  int max_tunings;	/* maximum tuning suggestions to find */
  int enable_window;    /* Enable Hann windowing of sample data */
  float ellapsed_time;  /* Ellapsed time of last execution in seconds */
} FFTuneSpectra;

typedef struct
{
  GObjectClass parent_class;
} FFTuneSpectraClass;

/* sample calculation mode enum */
enum
{
  FFTUNE_MODE_SELECTION,/* sample start/end selection (entire sample if 0/0) */
  FFTUNE_MODE_LOOP	/* sample loop */
};


GType fftune_spectra_get_type (void);
FFTuneSpectra *fftune_spectra_new (void);

#endif
