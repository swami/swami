/*
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
 
#ifndef __SWAMIGUI_KNOB_H__
#define __SWAMIGUI_KNOB_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SWAMIGUI_TYPE_KNOB	(swamigui_knob_get_type ())
#define SWAMIGUI_KNOB(obj)		\
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_KNOB, SwamiguiKnob))
#define SWAMIGUI_KNOB_CLASS(obj)	\
  (G_TYPE_CHECK_CLASS_CAST ((obj), SWAMIGUI_TYPE_KNOB, SwamiguiKnobClass))
#define SWAMIGUI_IS_KNOB(obj)		\
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_KNOB))

typedef struct _SwamiguiKnob		SwamiguiKnob;
typedef struct _SwamiguiKnobClass	SwamiguiKnobClass;

struct _SwamiguiKnob
{
  GtkDrawingArea parent;
  GtkAdjustment *adj;

  /* < private > */
  gboolean rotation_active;		/* mouse rotation is active? */
  double start_pos, end_pos;		/* start and end rotation in radians */
  double rotation;			/* current rotation in radians */
  double rotation_rate;			/* rotation rate in pixels/radians */
  double rotation_rate_fine;		/* rotation rate (fine) in pixels/rads */
  double xclick, yclick;		/* position of mouse click */
  double click_rotation;		/* rotation of knob upon click */
};

struct _SwamiguiKnobClass
{
  GtkDrawingAreaClass parent_class;
};

GType swamigui_knob_get_type (void);
GtkWidget *swamigui_knob_new (void);
GtkAdjustment *swamigui_knob_get_adjustment (SwamiguiKnob *knob);

G_END_DECLS

#endif
