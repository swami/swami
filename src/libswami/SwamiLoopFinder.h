/*
 * SwamiLoopFinder.h - Sample loop finder object
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
#ifndef __SWAMI_LOOP_FINDER_H__
#define __SWAMI_LOOP_FINDER_H__

#include <libinstpatch/libinstpatch.h>
#include <libswami/SwamiLoopResults.h>
#include <libswami/SwamiLock.h>

typedef struct _SwamiLoopFinder SwamiLoopFinder;
typedef struct _SwamiLoopFinderClass SwamiLoopFinderClass;

#define SWAMI_TYPE_LOOP_FINDER   (swami_loop_finder_get_type ())
#define SWAMI_LOOP_FINDER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_LOOP_FINDER, \
   SwamiLoopFinder))
#define SWAMI_IS_LOOP_FINDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_LOOP_FINDER))

/* Loop finder object */
struct _SwamiLoopFinder
{
    SwamiLock parent_instance;

    IpatchSample *sample;	        /* sample assigned to loop finder */
    IpatchSampleHandle *sample_handle;    /* Open handle to cached sample data (allocated) */
    guint sample_size;		/* size of sample */
    float *sample_data;		/* converted sample data */

    gboolean active;		/* TRUE if find is currently active */
    gboolean cancel;		/* set to TRUE to cancel current find */
    float progress;		/* if active is TRUE, progress 0.0 - 1.0 */
    int max_results;		/* max SwamiLoopMatch result entries */
    int analysis_window;		/* width in samples of analysis window */
    int min_loop_size;		/* minimum loop size */
    int window1_start;            /* sample start position of window1 search */
    int window1_end;	        /* sample end position of window1 search */
    int window2_start;		/* sample start position of window1 search */
    int window2_end;		/* sample end position of window1 search */
    int group_pos_diff;		/* min pos diff of loops for separate groups */
    int group_size_diff;		/* min size diff of loops for separate groups */
    guint exectime;		/* execution time in milliseconds */

    SwamiLoopResults *results;	/* results object */
};

/* Loop finder class */
struct _SwamiLoopFinderClass
{
    SwamiLockClass parent_class;
};

GType swami_loop_finder_get_type(void);
SwamiLoopFinder *swami_loop_finder_new(void);
void swami_loop_finder_full_search(SwamiLoopFinder *finder);
gboolean swami_loop_finder_verify_params(SwamiLoopFinder *finder,
        gboolean nudge, GError **err);
gboolean swami_loop_finder_find(SwamiLoopFinder *finder, GError **err);
SwamiLoopResults *swami_loop_finder_get_results(SwamiLoopFinder *finder);

#endif
