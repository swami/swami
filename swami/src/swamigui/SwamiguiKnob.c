/*
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
#include <gtk/gtk.h>
#include <math.h>
#include "config.h"

#include "SwamiguiKnob.h"
#include "util.h"

/* PNG image dimensions */
#define KNOB_WIDTH	40
#define KNOB_HEIGHT	40

#define KNOB_SIZE_REQ	40	/* Default knob request size (width and height) */

/* Indicator scale to knob width */
#define RADIUS_WIDTH_SCALE	(1.0/3.0)

/* default rotation rates in pixels/radians */
#define DEFAULT_ROTATION_RATE		(140.0 / (2 * M_PI))
#define DEFAULT_ROTATION_RATE_FINE	(1000.0 / (2 * M_PI))


G_DEFINE_TYPE (SwamiguiKnob, swamigui_knob, GTK_TYPE_DRAWING_AREA);

static gboolean swamigui_knob_draw (GtkWidget *widget, cairo_t *cr);
static gboolean swamigui_knob_event (GtkWidget *widget, GdkEvent *event);
static void swamigui_knob_adj_value_changed (GtkAdjustment *adj,
					     gpointer user_data);

GdkPixbuf *knob_pixbuf = NULL;


static void
swamigui_knob_class_init (SwamiguiKnobClass *class)
{
  GtkWidgetClass *widget_class;
  widget_class = GTK_WIDGET_CLASS (class);
  widget_class->draw = swamigui_knob_draw;
  widget_class->event = swamigui_knob_event;
}

static void
swamigui_knob_init (SwamiguiKnob *knob)
{
  if (!knob_pixbuf)
  {
    GError *err = NULL;
    gchar *resdir, *filename;

    /* ++ alloc resdir */
    resdir = swamigui_util_get_resource_path (SWAMIGUI_RESOURCE_PATH_IMAGES);
    /* ++ alloc filename */
    filename = g_build_filename (resdir, "knob.png", NULL);
    g_free (resdir); /* -- free resdir */

    knob_pixbuf = gdk_pixbuf_new_from_file (filename, &err);

    if (!knob_pixbuf)
    {
      g_critical ("Failed to open knob image file '%s': %s",
		  filename, err ? err->message : "No error details");
      g_clear_error (&err);
    }

    g_free (filename); /* -- free allocated filename */
  }

  gtk_widget_set_size_request (GTK_WIDGET (knob), KNOB_SIZE_REQ, KNOB_SIZE_REQ);

  knob->adj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1.0,
						  0.01, 0.10, 0.0));
  knob->start_pos = -150.0 * M_PI / 180.0;
  knob->end_pos = -knob->start_pos;
  knob->rotation = knob->start_pos;
  knob->rotation_rate = DEFAULT_ROTATION_RATE;
  knob->rotation_rate_fine = DEFAULT_ROTATION_RATE_FINE;

  g_signal_connect (knob->adj, "value-changed",
		    G_CALLBACK (swamigui_knob_adj_value_changed), knob);

  gtk_widget_add_events (GTK_WIDGET (knob), GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
}

static gboolean
swamigui_knob_draw (GtkWidget *widget, cairo_t *cr)
{
  SwamiguiKnob *knob = SWAMIGUI_KNOB (widget);
  GtkAllocation allocation;
  double halfwidth, halfheight;
  double radius;

  gtk_widget_get_allocation (widget, &allocation);

  radius = (double)allocation.width * RADIUS_WIDTH_SCALE;

  cairo_scale (cr, allocation.width / (double)KNOB_WIDTH, allocation.height / (double)KNOB_HEIGHT);

  gdk_cairo_set_source_pixbuf (cr, knob_pixbuf, 0.0, 0.0);
  cairo_paint (cr);

  halfwidth = allocation.width / 2.0;
  halfheight = allocation.height / 2.0;

  /* set line properties */
  cairo_set_line_width (cr, 2.0);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

  /* draw the line */
  cairo_line_to (cr, halfwidth, halfheight);
  cairo_line_to (cr, halfwidth + radius * sin (knob->rotation),
		 halfheight - radius * cos (knob->rotation));

  cairo_stroke (cr);

  return TRUE;
}

static gboolean
swamigui_knob_event (GtkWidget *widget, GdkEvent *event)
{
  SwamiguiKnob *knob = SWAMIGUI_KNOB (widget);
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  double rotation, normval, lower, upper;

  switch (event->type)
  {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *)event;
      if (bevent->button != 1) break;

      knob->rotation_active = TRUE;
      knob->xclick = bevent->x;
      knob->yclick = bevent->y;
      knob->click_rotation = knob->rotation;

      return (TRUE);
    case GDK_BUTTON_RELEASE:
      knob->rotation_active = FALSE;
      break;
    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *)event;

      if (!knob->rotation_active) return (TRUE);

      /* calculate rotation amount (SHIFT key causes a finer rotation) */
      rotation = knob->click_rotation + (knob->yclick - mevent->y)
        / ((mevent->state & GDK_SHIFT_MASK) ? knob->rotation_rate_fine : knob->rotation_rate);

      rotation = CLAMP (rotation, knob->start_pos, knob->end_pos);

      if (rotation == knob->rotation) return (TRUE);

      knob->rotation = rotation;
      normval = (rotation - knob->start_pos) / (knob->end_pos - knob->start_pos);       /* normalize rotation to a 0.0 to 1.0 position */
      g_object_get (knob->adj, "lower", &lower, "upper", &upper, NULL);

      g_signal_handlers_block_by_func (knob->adj, swamigui_knob_adj_value_changed, knob);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (knob->adj), normval * (upper - lower) + lower);         /* update the adjustment value */
      g_signal_handlers_unblock_by_func (knob->adj, swamigui_knob_adj_value_changed, knob);

      gtk_widget_queue_draw (GTK_WIDGET (knob));    /* update knob visual */
      break;
    default:
      break;
  }

  return (FALSE);
}

/* callback which gets called when the knob adjustment value changes externally */
static void
swamigui_knob_adj_value_changed (GtkAdjustment *adj, gpointer user_data)
{
  SwamiguiKnob *knob = SWAMIGUI_KNOB (user_data);
  double value, lower, upper, normval;

  g_object_get (adj, "value", &value, "lower", &lower, "upper", &upper, NULL);

  /* normalize the adjustment value to 0.0 through 1.0 */
  normval = (value - lower) / (upper - lower);
  normval = CLAMP (normval, 0.0, 1.0);

  /* convert to a rotation amount */
  knob->rotation = normval * (knob->end_pos - knob->start_pos) + knob->start_pos;
  gtk_widget_queue_draw (GTK_WIDGET (knob));    /* update knob visual */
}

/**
 * swamigui_knob_new:
 *
 * Create a new knob widget.
 *
 * Returns: Knob widget
 */
GtkWidget *
swamigui_knob_new (void)
{
  return (g_object_new (SWAMIGUI_TYPE_KNOB, NULL));
}

/**
 * swamigui_knob_get_adjustment:
 * @knob: Swamigui knob widget
 *
 * Get the #GtkAdjustment associated with a knob.
 *
 * Returns: The #GtkAdjustment for the knob
 */
GtkAdjustment *
swamigui_knob_get_adjustment (SwamiguiKnob *knob)
{
  g_return_val_if_fail (SWAMIGUI_IS_KNOB (knob), NULL);

  return (GTK_ADJUSTMENT (knob->adj));
}
