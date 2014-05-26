/*
 * SwamiguiLoopFinder.h - Sample loop finder widget
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
#ifndef __SWAMIGUI_LOOP_FINDER_H__
#define __SWAMIGUI_LOOP_FINDER_H__

#include <gtk/gtk.h>
#include <libswami/SwamiLoopFinder.h>

typedef struct _SwamiguiLoopFinder SwamiguiLoopFinder;
typedef struct _SwamiguiLoopFinderClass SwamiguiLoopFinderClass;

#define SWAMIGUI_TYPE_LOOP_FINDER   (swamigui_loop_finder_get_type ())
#define SWAMIGUI_LOOP_FINDER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_LOOP_FINDER, \
   SwamiguiLoopFinder))
#define SWAMIGUI_IS_LOOP_FINDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_LOOP_FINDER))

/* Loop finder object */
struct _SwamiguiLoopFinder
{
  GtkVBox parent_instance;		/* derived from GtkVBox */

  GtkListStore *store;		/* list store for results */
  GtkWidget *glade_widg;	/* the embedded glade widget */
  guint orig_loop_start;	/* original loop start of current sample */
  guint orig_loop_end;		/* original loop end of current sample */
  float prev_progress;		/* progress value caching */

  /*< public >*/
  SwamiLoopFinder *loop_finder;	/* loop finder object instance */
};

/* Loop finder widget class */
struct _SwamiguiLoopFinderClass
{
  GtkVBoxClass parent_class;
};

GType swamigui_loop_finder_get_type (void);
GtkWidget *swamigui_loop_finder_new (void);
void swamigui_loop_finder_clear_results (SwamiguiLoopFinder *finder);

#endif
