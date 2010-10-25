/*
 * SwamiLoopResults.h - Sample loop finder results object
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
#ifndef __SWAMI_LOOP_RESULTS_H__
#define __SWAMI_LOOP_RESULTS_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _SwamiLoopResults SwamiLoopResults;
typedef struct _SwamiLoopResultsClass SwamiLoopResultsClass;

#define SWAMI_TYPE_LOOP_RESULTS   (swami_loop_results_get_type ())
#define SWAMI_LOOP_RESULTS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_LOOP_RESULTS, \
   SwamiLoopResults))
#define SWAMI_IS_LOOP_RESULTS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_LOOP_RESULTS))

/* loop match structure */
typedef struct
{
  guint start;
  guint end;
  float quality;
} SwamiLoopMatch;

/* Loop results object */
struct _SwamiLoopResults
{
  GObject parent_instance;

  /*< public >*/
  SwamiLoopMatch *values;	/* loop match result values */
  int count;			/* number of entries in values */
};

/* Loop finder class */
struct _SwamiLoopResultsClass
{
  GObjectClass parent_class;
};

GType swami_loop_results_get_type (void);
SwamiLoopResults *swami_loop_results_new (void);
SwamiLoopMatch *swami_loop_results_get_values (SwamiLoopResults *results,
					       guint *count);
#endif
