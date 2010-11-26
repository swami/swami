/*
 * SwamiLoopFinder.c - Sample loop finder object
 *
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
/*
 * Thanks to Luis Garrido for the original loop finder algorithm code and his
 * interest in creating this feature for Swami.
 */

#include <stdio.h>

#include <libinstpatch/libinstpatch.h>

#include "SwamiLoopFinder.h"
#include "SwamiLog.h"
#include "i18n.h"

#define DEFAULT_MAX_RESULTS	200
#define MAX_MAX_RESULTS		4000
#define DEFAULT_ANALYSIS_WINDOW	17
#define DEFAULT_MIN_LOOP_SIZE	10
#define DEFAULT_GROUP_POS_DIFF	20
#define DEFAULT_GROUP_SIZE_DIFF	5

/* Sample format used by loop finder */
#define SAMPLE_FORMAT   IPATCH_SAMPLE_FLOAT | IPATCH_SAMPLE_MONO | IPATCH_SAMPLE_ENDIAN_HOST

enum
{
  PROP_0,
  PROP_RESULTS,			/* SwamiLoopResults object */
  PROP_ACTIVE,			/* TRUE if find is in progress */
  PROP_CANCEL,			/* set to TRUE to cancel in progress find */
  PROP_PROGRESS,		/* current progress of find (0.0 - 1.0) */
  PROP_SAMPLE,		        /* sample data assigned to loop finder */
  PROP_MAX_RESULTS,		/* max results to return */
  PROP_ANALYSIS_WINDOW,		/* size in samples of analysis window */
  PROP_MIN_LOOP_SIZE,		/* minimum loop size */
  PROP_WINDOW1_START,	        /* window1 start position in samples */
  PROP_WINDOW1_END,	        /* window1 end position in samples */
  PROP_WINDOW2_START,		/* window2 start position in samples */
  PROP_WINDOW2_END,		/* window2 end position in samples */
  PROP_GROUP_POS_DIFF,		/* min pos diff of loops for separate groups */
  PROP_GROUP_SIZE_DIFF,		/* min size diff of loops for separate groups */
  PROP_EXEC_TIME		/* execution time in milliseconds of find */
};

static void swami_loop_finder_set_property (GObject *object,
					    guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void swami_loop_finder_get_property (GObject *object,
					    guint property_id,
					    GValue *value,
					    GParamSpec *pspec);
static void swami_loop_finder_init (SwamiLoopFinder *editor);
static void swami_loop_finder_finalize (GObject *object);
static void swami_loop_finder_real_set_sample (SwamiLoopFinder *finder,
					       IpatchSample *sample);
static void find_loop (SwamiLoopFinder *finder, SwamiLoopMatch *matches);


G_DEFINE_TYPE (SwamiLoopFinder, swami_loop_finder, SWAMI_TYPE_LOCK);


static void
swami_loop_finder_class_init (SwamiLoopFinderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->set_property = swami_loop_finder_set_property;
  obj_class->get_property = swami_loop_finder_get_property;
  obj_class->finalize = swami_loop_finder_finalize;

  g_object_class_install_property (obj_class, PROP_RESULTS,
		g_param_spec_object ("results", _("Results"),
				     _("Loop results object"),
				     SWAMI_TYPE_LOOP_RESULTS, G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_ACTIVE,
		g_param_spec_boolean ("active", _("Active"), _("Active"),
				      FALSE, G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_CANCEL,
		g_param_spec_boolean ("cancel", _("Cancel"), _("Cancel"),
				      FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PROGRESS,
		g_param_spec_float ("progress", _("Progress"), _("Progress"),
				    0.0, 1.0, 0.0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAMPLE,
		g_param_spec_object ("sample", _("Sample"), _("Sample data object"),
				     IPATCH_TYPE_SAMPLE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MAX_RESULTS,
		g_param_spec_int ("max-results", _("Max results"),
				  _("Max results"),
				  1, MAX_MAX_RESULTS, DEFAULT_MAX_RESULTS,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ANALYSIS_WINDOW,
		g_param_spec_int ("analysis-window", _("Analysis window"),
				  _("Size of analysis window"),
				  1, G_MAXINT, DEFAULT_ANALYSIS_WINDOW,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MIN_LOOP_SIZE,
		g_param_spec_int ("min-loop-size", _("Min loop size"),
				  _("Minimum size of matching loops"),
				  1, G_MAXINT, DEFAULT_MIN_LOOP_SIZE,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_WINDOW1_START,
		g_param_spec_int ("window1-start", _("First window start position"),
				  _("First window start position"),
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_WINDOW1_END,
		g_param_spec_int ("window1-end", _("First window end position"),
				  _("First window end position"),
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_WINDOW2_START,
		g_param_spec_int ("window2-start", _("Second window start position"),
				  _("Second window start position"),
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_WINDOW2_END,
		g_param_spec_int ("window2-end", _("Second window end position"),
				  _("Second window end position"),
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_GROUP_POS_DIFF,
		g_param_spec_int ("group-pos-diff", _("Group pos diff"),
				  _("Min difference of loop position of separate groups"),
				  0, G_MAXINT, DEFAULT_GROUP_POS_DIFF,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_GROUP_SIZE_DIFF,
		g_param_spec_int ("group-size-diff", _("Group size diff"),
				  _("Min difference of loop size of separate groups"),
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_EXEC_TIME,
		g_param_spec_uint ("exec-time", _("Exec time"),
				   _("Execution time in milliseconds"),
				   0, G_MAXUINT, 0, G_PARAM_READABLE));
}

static void
swami_loop_finder_set_property (GObject *object, guint property_id,
				const GValue *value, GParamSpec *pspec)
{
  SwamiLoopFinder *finder = SWAMI_LOOP_FINDER (object);
  GObject *obj;

  switch (property_id)
    {
    case PROP_CANCEL:
      if (g_value_get_boolean (value))
	finder->cancel = TRUE;
      break;
    case PROP_PROGRESS:
      finder->progress = g_value_get_float (value);
      break;
    case PROP_SAMPLE:
      obj = g_value_get_object (value);
      swami_loop_finder_real_set_sample (finder, obj ? IPATCH_SAMPLE (obj) : NULL);
      break;
    case PROP_MAX_RESULTS:
      finder->max_results = g_value_get_int (value);
      break;
     case PROP_ANALYSIS_WINDOW:
      finder->analysis_window = g_value_get_int (value);
      break;
     case PROP_MIN_LOOP_SIZE:
      finder->min_loop_size = g_value_get_int (value);
      break;
    case PROP_WINDOW1_START:
      finder->window1_start = g_value_get_int (value);
      break;
    case PROP_WINDOW1_END:
      finder->window1_end = g_value_get_int (value);
      break;
    case PROP_WINDOW2_START:
      finder->window2_start = g_value_get_int (value);
      break;
    case PROP_WINDOW2_END:
      finder->window2_end = g_value_get_int (value);
      break;
    case PROP_GROUP_POS_DIFF:
      finder->group_pos_diff = g_value_get_int (value);
      break;
    case PROP_GROUP_SIZE_DIFF:
      finder->group_size_diff = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swami_loop_finder_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  SwamiLoopFinder *finder = SWAMI_LOOP_FINDER (object);

  switch (property_id)
    {
    case PROP_RESULTS:
      SWAMI_LOCK_READ (finder);
      g_value_set_object (value, finder->results);
      SWAMI_UNLOCK_READ (finder);
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, finder->active);
      break;
    case PROP_CANCEL:
      g_value_set_boolean (value, finder->cancel);
      break;
    case PROP_PROGRESS:
      g_value_set_float (value, finder->progress);
      break;
    case PROP_SAMPLE:
      g_value_set_object (value, finder->sample);
      break;
    case PROP_MAX_RESULTS:
      g_value_set_int (value, finder->max_results);
      break;
    case PROP_ANALYSIS_WINDOW:
      g_value_set_int (value, finder->analysis_window);
      break;
    case PROP_MIN_LOOP_SIZE:
      g_value_set_int (value, finder->min_loop_size);
      break;
    case PROP_WINDOW1_START:
      g_value_set_int (value, finder->window1_start);
      break;
    case PROP_WINDOW1_END:
      g_value_set_int (value, finder->window1_end);
      break;
    case PROP_WINDOW2_START:
      g_value_set_int (value, finder->window2_start);
      break;
    case PROP_WINDOW2_END:
      g_value_set_int (value, finder->window2_end);
      break;
    case PROP_EXEC_TIME:
      g_value_set_uint (value, finder->exectime);
      break;
    case PROP_GROUP_POS_DIFF:
      g_value_set_int (value, finder->group_pos_diff);
      break;
    case PROP_GROUP_SIZE_DIFF:
      g_value_set_int (value, finder->group_size_diff);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swami_loop_finder_init (SwamiLoopFinder *finder)
{
  finder->max_results = DEFAULT_MAX_RESULTS;
  finder->analysis_window = DEFAULT_ANALYSIS_WINDOW;
  finder->min_loop_size = DEFAULT_MIN_LOOP_SIZE;
  finder->group_pos_diff = DEFAULT_GROUP_POS_DIFF;
  finder->group_size_diff = DEFAULT_GROUP_SIZE_DIFF;
}

static void
swami_loop_finder_finalize (GObject *object)
{
  SwamiLoopFinder *finder = SWAMI_LOOP_FINDER (object);

  if (finder->sample) g_object_unref (finder->sample);

  if (finder->sample_handle)  /* Close and free old cached sample handle if any */
  {
    ipatch_sample_handle_close (finder->sample_handle);
    g_slice_free (IpatchSampleHandle, finder->sample_handle);
  }

  if (finder->results) g_object_unref (finder->results);

  if (G_OBJECT_CLASS (swami_loop_finder_parent_class)->finalize)
    G_OBJECT_CLASS (swami_loop_finder_parent_class)->finalize (object);
}

/**
 * swami_loop_finder_new:
 *
 * Create a new sample loop finder object.
 *
 * Returns: New object of type #SwamiLoopFinder
 */
SwamiLoopFinder *
swami_loop_finder_new (void)
{
  return (SWAMI_LOOP_FINDER (g_object_new (SWAMI_TYPE_LOOP_FINDER, NULL)));
}

static void
swami_loop_finder_real_set_sample (SwamiLoopFinder *finder, IpatchSample *sample)
{
  IpatchSampleData *sampledata;
  IpatchSampleHandle *old_handle;
  gpointer old_sample;
  guint new_sample_size = 0;

  if (sample == finder->sample) return;

  if (sample)
  {
    g_object_get (sample, "sample-data", &sampledata, NULL);    /* ++ ref sample data */

    if (sampledata)
    {
      g_object_get (sample, "sample-size", &new_sample_size, NULL);
      g_object_ref (sample);      /* ++ ref for loop finder */
      g_object_unref (sampledata);      /* -- unref sample data */
    }
    else sample = NULL; /* no sample-data property? - No dice */
  }

  SWAMI_LOCK_WRITE (finder);

  old_sample = finder->sample;
  old_handle = finder->sample_handle;
  finder->sample = sample;
  finder->sample_handle = NULL;
  finder->sample_size = new_sample_size;
  finder->sample_data = NULL;

  SWAMI_UNLOCK_WRITE (finder);

  if (sample) swami_loop_finder_full_search (finder);

  if (old_sample) g_object_unref (old_sample);  /* -- unref old sample (if any) */

  if (old_handle)       /* Close and free old cached sample handle if any */
  {
    ipatch_sample_handle_close (old_handle);
    g_slice_free (IpatchSampleHandle, old_handle);
  }
}

/**
 * swami_loop_finder_full_search:
 * @finder: Loop finder object
 *
 * Configures a loop finder to do a full loop search of the assigned sample.
 * Note that a sample must have already been assigned and the "analysis-window"
 * and "min-loop-size" parameters should be valid for it.  This means:
 * (finder->analysis_window <= finder->sample_size) &&
 * (finder->sample_size - finder->analysis_window) >= finder->min_loop_size.
 */
void
swami_loop_finder_full_search (SwamiLoopFinder *finder)
{
  int max_loop_size;

  g_return_if_fail (SWAMI_IS_LOOP_FINDER (finder));
  g_return_if_fail (finder->sample != NULL);

  max_loop_size = finder->sample_size - finder->analysis_window;

  /* silently fail if analysis window or loop size is out of wack */
  if (finder->analysis_window > finder->sample_size
      || max_loop_size < finder->min_loop_size)
    return;

  finder->window1_start = finder->analysis_window / 2;
  finder->window1_end = finder->window1_start + finder->sample_size
    - finder->analysis_window;
  finder->window2_start = finder->window1_start;
  finder->window2_end = finder->window1_end;

  g_object_notify (G_OBJECT (finder), "window1-start");
  g_object_notify (G_OBJECT (finder), "window1-end");
  g_object_notify (G_OBJECT (finder), "window2-start");
  g_object_notify (G_OBJECT (finder), "window2-end");
}

/**
 * swami_loop_finder_verify_params:
 * @finder: Loop finder object
 * @nudge: %TRUE to nudge start and end loop search windows into compliance
 * @err: Location to store error message or %NULL to ignore
 *
 * Verify a loop finder's parameters. If the @nudge parameter is %TRUE then
 * corrections will be made to start and end loop search windows if necessary
 * (changed to fit within sample and analysis window and swapped if
 *  windows are backwards).
 *
 * Note that the other parameters will not be corrected and will still cause
 * errors if wacked.  Nudged values are only changed if %TRUE is returned.
 *
 * Returns: %TRUE on success, %FALSE if there is a problem with the loop
 *   finder parameters (error may be stored in @err).
 */
gboolean
swami_loop_finder_verify_params (SwamiLoopFinder *finder, gboolean nudge,
				 GError **err)
{
  int halfwin, ohalfwin;
  int win1start, win1end;	/* stores window1 start/end */
  int win2start, win2end;	/* stores window2 start/end */
  int tmp;

  g_return_val_if_fail (SWAMI_IS_LOOP_FINDER (finder), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* Swap start/ends as needed */
  win1start = MIN (finder->window1_start, finder->window1_end);
  win1end = MAX (finder->window1_start, finder->window1_end);
  win2start = MIN (finder->window2_start, finder->window2_end);
  win2end = MAX (finder->window2_start, finder->window2_end);

  /* Swap ranges if needed, so that window1 is start of loop */
  if (win1start > win2start)
  {
    tmp = win1start;
    win1start = win2start;
    win2start = tmp;

    tmp = win1end;
    win1end = win2end;
    win2end = tmp;
  }

  /* calculate first and second half of analysis window */
  halfwin = finder->analysis_window / 2;
  ohalfwin = finder->analysis_window - halfwin;	/* other half */

  /* analysis window is sane? */
  if (finder->analysis_window > finder->sample_size)
  {
    g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_INVALID,
		 _("Analysis window is too large for sample"));
    return (FALSE);
  }

  /* nudge loop search windows if needed */
  if (nudge)
  {
    if (win1start < halfwin) win1start = halfwin;
    if (win1end > (finder->sample_size - ohalfwin))
      win1end = finder->sample_size - ohalfwin;

    if (win2start < halfwin) win2start = halfwin;
    if (win2end > (finder->sample_size - ohalfwin))
      win2end = finder->sample_size - ohalfwin;
  }
  else
  {
    /* window1 is valid? */
    if (win1start < halfwin || win1end > (finder->sample_size - ohalfwin))
    {
      g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_INVALID,
		   _("Loop start search window is invalid"));
      return (FALSE);
    }

    /* loop end search window is valid? */
    if (win2start < halfwin || win2end > (finder->sample_size - ohalfwin))
    {
      g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_INVALID,
		   _("Loop end search window is invalid"));
      return (FALSE);
    }
  }

  /* make sure min_loop_size isn't impossible to satisfy */
  if (win2end - win1start + 1 < finder->min_loop_size)
  {
    g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_INVALID,
		 _("Impossible to satisfy minimum loop size"));
    return (FALSE);
  }

  if (nudge)	/* modify parameters if they have been nudged */
  {
    if (win1start != finder->window1_start)
      g_object_set (finder, "window1-start", win1start, NULL);
    if (win1end != finder->window1_end)
      g_object_set (finder, "window1-end", win1end, NULL);
    if (win2start != finder->window2_start)
      g_object_set (finder, "window2-start", win2start, NULL);
    if (win2end != finder->window2_end)
      g_object_set (finder, "window2-end", win2end, NULL);
  }

  return (TRUE);
}

/**
 * swami_loop_finder_find:
 * @finder: Loop finder object
 * @err: Location to store error info or %NULL to ignore
 *
 * Execute the loop find operation.  This function blocks until operation is
 * finished or the "cancel" property is assigned %TRUE.  The "progress"
 * parameter is updated periodically with the current progress value (between
 * 0.0 and 1.0).
 *
 * Note: Not thread safe.  Properties should be assigned, this function
 * called and the results retrieved with swami_loop_finder_get_results() in
 * a serialized fashion.  As long as this is ensured, creating a separate
 * thread to call this function will be OK.  The results can only be accessed
 * up until the next call to this function.
 *
 * Returns: TRUE on success, FALSE if the find parameter values are invalid
 *   or operation was cancelled (in which case @err may be set).
 */
gboolean
swami_loop_finder_find (SwamiLoopFinder *finder, GError **err)
{
  IpatchSampleData *sampledata;
  IpatchSampleStore *store;
  SwamiLoopResults *results;
  SwamiLoopMatch *matches;
  GTimeVal start, end;
  int i;

  /* make sure parameter are sane */
  if (!swami_loop_finder_verify_params (finder, FALSE, err)) return (FALSE);

  /* change active state */
  finder->active = TRUE;
  g_object_notify (G_OBJECT (finder), "active");

  /* sample data converted to float yet? */
  if (!finder->sample_data)
  {
    g_object_get (finder->sample, "sample-data", &sampledata, NULL);    /* ++ ref sample data */
    g_return_val_if_fail (sampledata != NULL, FALSE);

    /* ++ ref sample store in floating point format */
    store = ipatch_sample_data_get_cache_sample (sampledata, SAMPLE_FORMAT,
                                                 IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err);
    if (!store)
    {
      g_object_unref (sampledata);      /* -- unref sample data */
      return (FALSE);
    }

    g_object_unref (sampledata);      /* -- unref sample data */

    /* FIXME - What to do about stereo? */

    /* Allocate a sample handle and open the store */
    finder->sample_handle = g_slice_new (IpatchSampleHandle);
    if (!ipatch_sample_handle_open (IPATCH_SAMPLE (store), finder->sample_handle,
                                    'r', 0, IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err))
    {
      g_object_unref (store);   /* -- unref store */
      return (FALSE);
    }

    finder->sample_data = ((IpatchSampleStoreCache *)store)->location;

    g_object_unref (store);     /* -- unref store */
  }

  matches = g_new (SwamiLoopMatch, finder->max_results);

  g_get_current_time (&start);

  /* run the loop finder processing algorithm */
  find_loop (finder, matches);

  g_get_current_time (&end);

  finder->exectime = (end.tv_sec - start.tv_sec) * 1000;
  finder->exectime += (end.tv_usec - start.tv_usec + 500) / 1000;

/* quit_it: */

  if (finder->cancel)	/* find was canceled? */
  {
    g_free (matches);

    SWAMI_LOCK_WRITE (finder);
    if (finder->results)
    {
      g_object_unref (finder->results);
      finder->results = NULL;
    }
    SWAMI_UNLOCK_WRITE (finder);

    finder->cancel = FALSE;
    g_object_notify (G_OBJECT (finder), "cancel");

    finder->active = FALSE;
    g_object_notify (G_OBJECT (finder), "active");

    g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_CANCELED,
		 _("Find operation canceled"));
    return (FALSE);
  }

  /* search for first uninitialized result, to calculate result count */
  for (i = 0; i < finder->max_results; i++)
    if (matches[i].start == 0 && matches[i].end == 0)
      break;

  if (i > 0)
  {
    results = swami_loop_results_new ();	/* ++ ref (taken by finder) */
    results->count = i;
    results->values = matches;
  }
  else results = NULL;

  SWAMI_LOCK_WRITE (finder);
  if (finder->results) g_object_unref (finder->results);
  finder->results = results;
  SWAMI_UNLOCK_WRITE (finder);

  g_object_notify (G_OBJECT (finder), "results");

  finder->active = FALSE;
  g_object_notify (G_OBJECT (finder), "active");

  return (TRUE);
}

/**
 * swami_loop_finder_get_results:
 * @finder: Loop finder object
 *
 * Get the loop results object from a loop finder.
 *
 * Returns: Loop results object or %NULL if none (if swami_loop_finder_find()
 *   has not yet been called or was canceled).  The caller owns a reference
 *   to the returned object and should unref it when completed.
 */
SwamiLoopResults *
swami_loop_finder_get_results (SwamiLoopFinder *finder)
{
  SwamiLoopResults *results;

  g_return_val_if_fail (SWAMI_IS_LOOP_FINDER (finder), NULL);

  SWAMI_LOCK_READ (finder);
  results = finder->results ? g_object_ref (finder->results) : NULL;
  SWAMI_UNLOCK_READ (finder);

  return (results);
}

/* the loop finder algorithm, parameters should be varified before calling. */
static void
find_loop (SwamiLoopFinder *finder, SwamiLoopMatch *matches)
{
  float *sample_data = finder->sample_data;             /* Pointer to sample data */
  int max_results = finder->max_results;                /* Maximum results to return */
  int analysis_window = finder->analysis_window;        /* Analysis window size */
  int min_loop_size = finder->min_loop_size;            /* Minimum loop size */
  int win1start, win1end, win1size, win2start, win2end, win2size;  /* Search window parameters */
  int group_pos_diff = finder->group_pos_diff;          /* Minimum result group position diff */
  int group_size_diff = finder->group_size_diff;        /* Minimum result group size diff */
  int half_window = analysis_window / 2;                /* First half of analysis window */
  guint64 progress_step, progress_count;                /* Progress update vars */
  SwamiLoopMatch *match, *cmpmatch;
  float *anwin_factors;
  GList *match_list = NULL;
  GList *match_list_last = NULL;
  int match_list_size = 0;
  float match_list_worst = 1.0;
  int win1, win2;
  int startpos, endpos;
  int pos_diff, size_diff, loop_diff;
  float quality, diff;
  GList *p, *link, *insert, *tmp;
  int fract, pow2;
  int i;

  /* Swap start/ends as needed */
  win1start = MIN (finder->window1_start, finder->window1_end);
  win1end = MAX (finder->window1_start, finder->window1_end);
  win2start = MIN (finder->window2_start, finder->window2_end);
  win2end = MAX (finder->window2_start, finder->window2_end);

  /* Swap ranges if needed, so that window1 is loop start search */
  if (win1start > win2start)
  {
    int tmp;

    tmp = win1start;
    win1start = win2start;
    win2start = tmp;

    tmp = win1end;
    win1end = win2end;
    win2end = tmp;
  }
  
  win1size = win1end - win1start + 1;
  win2size = win2end - win2start + 1;

  /* Control of progress update */
  progress_step = ((float)win1size * (float)win2size) / 1000.0;      /* Max. 1000 progress callbacks */
  progress_count = progress_step;

  finder->progress = 0.0;
  g_object_notify ((GObject *)finder, "progress");

  /* Create analysis window factors array.  All values in array add up to
   * 0.5 which when multiplied times maximum sample value difference of
   * 2.0 (1 - -1), gives a maximum quality value (worse quality) of 1.0.
   * Each neighboring factor towards the center point is twice the value of
   * it's outer neighbor. */
  anwin_factors = g_new (float, analysis_window);

  /* Calculate fraction divisor */
  for (i = 0, fract = 0, pow2 = 1; i <= half_window; i++, pow2 *= 2)
  {
    fract += pow2;
    if (i < half_window) fract += pow2;
  }

  /* Even windows are asymetrical, subtract 1 */
  if (!(analysis_window & 1)) fract--;

  /* Calculate values for 1st half of window and center of window */
  for (i = 0, pow2 = 1; i <= half_window; i++, pow2 *= 2)
    anwin_factors[i] = pow2 * 0.5 / fract;

  /* Copy values for 2nd half of window */
  for (i = 0; half_window + i + 1 < analysis_window; i++)
    anwin_factors[half_window + i + 1] = anwin_factors[half_window - i - 1];

  for (win1 = 0; win1 < win1size; win1++)
  {
    startpos = win1start + win1;

    for (win2 = 0; win2 < win2size; win2++)
    {
      endpos = win2start + win2;

      if (finder->cancel)  /* if cancel flag has been set, return */
      {
        for (p = match_list; p; p = g_list_delete_link (p, p))
          g_slice_free (SwamiLoopMatch, p->data);

        g_free (anwin_factors);
	return;
      }

      /* progress management */
      progress_count--;
      if (progress_count == 0)
      {
        progress_count = progress_step;

	finder->progress = ((float)win1 * win2size + win2) / ((float)win1size * win2size);
	g_object_notify ((GObject *)finder, "progress");
      }

      if (startpos >= endpos || endpos - startpos + 1 < min_loop_size)
        continue;

      for (i = 0, quality = 0.0; i < analysis_window; i++)
      {
        diff = sample_data[startpos + i - half_window]
          - sample_data[endpos + i - half_window];
        if (diff < 0) diff = -diff;
        quality += diff * anwin_factors[i];
      }

      /* Skip if worse than the worst and result list already full */
      if (quality >= match_list_worst && match_list_size == max_results)
        continue;

      loop_diff = endpos - startpos;
      insert = NULL;
      link = NULL;

      /* Look through existing matches for insert position, check if new
       * match is a part of an existing group and discard new match if worse
       * than existing group match or remove old group matches if worse quality */
      for (p = match_list; p; )
      {
        cmpmatch = (SwamiLoopMatch *)(p->data);

        /* Calculate position and size differences of new match and cmpmatch */
        pos_diff = startpos - cmpmatch->start;
        size_diff = loop_diff - (cmpmatch->end - cmpmatch->start);
        if (pos_diff < 0) pos_diff = -pos_diff;
        if (size_diff < 0) size_diff = -size_diff;

        /* Same match group? */
        if (pos_diff < group_pos_diff && size_diff < group_size_diff)
        {
          /* New match is worse? - Discard new */
          if (quality >= cmpmatch->quality)
            break;

          /* New match is better - Discard old */

          if (p == match_list_last)
          {
            match_list_last = p->prev;

            if (match_list_last)
            {
              match = (SwamiLoopMatch *)(match_list_last->data);
              match_list_worst = match->quality;
            }
          }

          if (!link)
          { /* Re-use list nodes */
            link = p;
            p = p->next;
            match_list = g_list_remove_link (match_list, link);
          }
          else
          {
            tmp = p;
            p = p->next;
            g_slice_free (SwamiLoopMatch, tmp->data);
            match_list = g_list_delete_link (match_list, tmp);
          }

          match_list_size--;
          continue;
        }

        if (!insert && quality < cmpmatch->quality)
          insert = p;

        p = p->next;
      }

      /* Discard new match? */
      if (p) continue;

      /* max results reached? */
      if (match_list_size == max_results)
      {
        if (insert == match_list_last) insert = NULL;

        if (!link)
        { /* Re-use list nodes */
          link = match_list_last;
          match_list_last = match_list_last->prev;
          match_list = g_list_remove_link (match_list, link);
        }
        else
        {
          tmp = match_list_last;
          match_list_last = match_list_last->prev;
          g_slice_free (SwamiLoopMatch, tmp->data);
          match_list = g_list_delete_link (match_list, tmp);
        }

        match = (SwamiLoopMatch *)(match_list_last->data);
        match_list_worst = match->quality;
        match_list_size--;
      }

      if (!link)
      {
        match = g_slice_new (SwamiLoopMatch);
        link = g_list_append (NULL, match);
      }
      else match = link->data;

      match_list_size++;

      match->start = startpos;
      match->end = endpos;
      match->quality = quality;

      if (insert)
      {
        link->prev = insert->prev;
        link->next = insert;

        if (insert->prev) insert->prev->next = link;
        else match_list = link;

        insert->prev = link;
      }
      else      /* Append */
      {
        if (match_list_last)
        {
          match_list_last->next = link;
          link->prev = match_list_last;
        }
        else match_list = link;

        match_list_last = link;
        match = (SwamiLoopMatch *)(match_list_last->data);
        match_list_worst = match->quality;
      }
    }
  }

  for (p = match_list, i = 0; p; p = g_list_delete_link (p, p), i++)
  {
    match = (SwamiLoopMatch *)(p->data);

#if 0   // Debugging output of results
    printf ("Quality: %0.4f start: %d end: %d\n", match->quality,
            match->start, match->end);

    for (i2 = 0; i2 < analysis_window; i2++)
    {
      float f;

      diff = sample_data[match->start - half_window + i2]
        - sample_data[match->end - half_window + i2];
      if (diff < 0.0) diff = -diff;
      f = (float)diff * anwin_factors[i2];
      printf ("  %d diff:%0.8f * factor:%0.8f = %0.8f\n",
              i2, diff, anwin_factors[i2], f);
    }
#endif

    matches[i].start = match->start;
    matches[i].end = match->end;
    matches[i].quality = match->quality;

    g_slice_free (SwamiLoopMatch, match);
  }

  for (; i < max_results; i++)
  {
    matches[i].start = 0;
    matches[i].end = 0;
    matches[i].quality = 1.0;
  }

  finder->progress = 1.0;
  g_object_notify ((GObject *)finder, "progress");

  g_free (anwin_factors);
}
