/*
 * SwamiLoopResults.c - Sample loop finder results object
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Thanks to Luis Garrido for the loop finder algorithm code and his
 * interest in creating this feature for Swami.
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
#include <stdio.h>

#include "SwamiLoopResults.h"

static void swami_loop_results_init (SwamiLoopResults *results);
static void swami_loop_results_finalize (GObject *object);


G_DEFINE_TYPE (SwamiLoopResults, swami_loop_results, G_TYPE_OBJECT);


static void
swami_loop_results_class_init (SwamiLoopResultsClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->finalize = swami_loop_results_finalize;
}

static void
swami_loop_results_init (SwamiLoopResults *results)
{
}

static void
swami_loop_results_finalize (GObject *object)
{
  SwamiLoopResults *results = SWAMI_LOOP_RESULTS (object);

  g_free (results->values);

  if (G_OBJECT_CLASS (swami_loop_results_parent_class)->finalize)
    G_OBJECT_CLASS (swami_loop_results_parent_class)->finalize (object);
}

/**
 * swami_loop_results_new:
 *
 * Create a new sample loop finder results object.  Used for storing results
 * of a #SwamiLoopFinder find operation.
 *
 * Returns: New object of type #SwamiLoopResults
 */
SwamiLoopResults *
swami_loop_results_new (void)
{
  return (SWAMI_LOOP_RESULTS (g_object_new (SWAMI_TYPE_LOOP_RESULTS, NULL)));
}

/**
 * swami_loop_results_get_values:
 * @results: Loop finder results object
 * @length: Output: location to store length of returned values array
 *
 * Get the loop match values array from a loop results object.
 *
 * Returns: Array of loop results or %NULL if none.  The array is internal,
 *   should not be modified or freed and should be used only up to the point
 *   where @results is destroyed (no more references held).
 */
SwamiLoopMatch *
swami_loop_results_get_values (SwamiLoopResults *results,
			       guint *count)
{
  g_return_val_if_fail (SWAMI_IS_LOOP_RESULTS (results), NULL);
  g_return_val_if_fail (count != NULL, NULL);

  *count = results->count;
  return (results->values);
}
