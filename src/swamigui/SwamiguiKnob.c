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


G_DEFINE_TYPE(SwamiguiKnob, swamigui_knob, GTK_TYPE_DRAWING_AREA);

static gboolean swamigui_knob_expose(GtkWidget *widget, GdkEventExpose *event);
static void swamigui_knob_update(SwamiguiKnob *knob);
static gboolean swamigui_knob_button_press_event(GtkWidget *widget,
        GdkEventButton *event);
static gboolean swamigui_knob_button_release_event(GtkWidget *widget,
        GdkEventButton *event);
static gboolean swamigui_knob_motion_notify_event(GtkWidget *widget,
        GdkEventMotion *event);
static void swamigui_knob_adj_value_changed(GtkAdjustment *adj,
        gpointer user_data);

/*
  knob_pixbuf is global and common to all SwamiguiKnob widgets.
  This allows to save memory.
  It must be freed when the application is complete
*/
static GdkPixbuf *knob_pixbuf = NULL;

/*--- Initialization / deinitialization -------------------------------------*/

/***
 * Initialization
 */
void _swamigui_knob_init(void)
{
    knob_pixbuf = NULL;
}

/***
 * Deinitialization: free memory
 */
void _swamigui_knob_deinit(void)
{
    g_object_unref(knob_pixbuf);
}

/*----- SwamiguiKnob object functions ---------------------------------------*/

static void
swamigui_knob_class_init(SwamiguiKnobClass *class)
{
    GtkWidgetClass *widget_class;
    widget_class = GTK_WIDGET_CLASS(class);
    widget_class->expose_event = swamigui_knob_expose;
    widget_class->button_press_event = swamigui_knob_button_press_event;
    widget_class->button_release_event = swamigui_knob_button_release_event;
    widget_class->motion_notify_event = swamigui_knob_motion_notify_event;
}

static void
swamigui_knob_init(SwamiguiKnob *knob)
{
    if(!knob_pixbuf)
    {
        GError *err = NULL;
        gchar *resdir, *filename;

        /* ++ alloc resdir */
        resdir = swamigui_util_get_resource_path(SWAMIGUI_RESOURCE_PATH_IMAGES);
        /* ++ alloc filename */
        filename = g_build_filename(resdir, "knob.png", NULL);
        g_free(resdir);  /* -- free resdir */

        knob_pixbuf = gdk_pixbuf_new_from_file(filename, &err);

        if(!knob_pixbuf)
        {
            g_critical("Failed to open SVG knob file '%s': %s",
                       filename, err ? err->message : "No error details");
            g_clear_error(&err);
        }

        g_free(filename);  /* -- free allocated filename */
    }

    gtk_widget_set_size_request(GTK_WIDGET(knob), KNOB_SIZE_REQ, KNOB_SIZE_REQ);

    knob->adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0,
                               0.01, 0.10, 0.0));
    knob->start_pos = -150.0 * M_PI / 180.0;
    knob->end_pos = -knob->start_pos;
    knob->rotation = knob->start_pos;
    knob->rotation_rate = DEFAULT_ROTATION_RATE;
    knob->rotation_rate_fine = DEFAULT_ROTATION_RATE_FINE;

    g_signal_connect(knob->adj, "value-changed",
                     G_CALLBACK(swamigui_knob_adj_value_changed), knob);

    gtk_widget_set_events(GTK_WIDGET(knob),
                          GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
                          | GDK_BUTTON_RELEASE_MASK);
}

static gboolean
swamigui_knob_expose(GtkWidget *widget, GdkEventExpose *event)
{
    SwamiguiKnob *knob = SWAMIGUI_KNOB(widget);
    static guint cache_width = 0, cache_height = 0;
    static cairo_surface_t *knob_background = NULL;
    static double radius = 0.0;
    double halfwidth, halfheight;
    cairo_t *cr;

    if(!knob_background || cache_width != widget->allocation.width
            || cache_height != widget->allocation.height)
    {
        if(knob_background)
        {
            cairo_surface_destroy(knob_background);
        }

        cache_width = widget->allocation.width;
        cache_height = widget->allocation.height;
        radius = (double)cache_width * RADIUS_WIDTH_SCALE;

        knob_background = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                          cache_width, cache_height);
        cr = cairo_create(knob_background);

        cairo_scale(cr, cache_width / (double)KNOB_WIDTH,
                    cache_height / (double)KNOB_HEIGHT);

        gdk_cairo_set_source_pixbuf(cr, knob_pixbuf, 0.0, 0.0);
        cairo_paint(cr);
        cairo_destroy(cr);
    }

    halfwidth = cache_width / 2.0;
    halfheight = cache_height / 2.0;

    cr = gdk_cairo_create(widget->window);	/* ++ create a cairo_t */

    /* clip to the exposed area */
    cairo_rectangle(cr, event->area.x, event->area.y,
                    event->area.width, event->area.height);
    cairo_clip(cr);

    /* draw knob background */
    cairo_set_source_surface(cr, knob_background, 0.0, 0.0);
    cairo_paint(cr);

    /* set line properties */
    cairo_set_line_width(cr, 2.0);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    /* draw the line */
    cairo_line_to(cr, halfwidth, halfheight);
    cairo_line_to(cr, halfwidth + radius * sin(knob->rotation),
                  halfheight - radius * cos(knob->rotation));

    cairo_stroke(cr);

    cairo_destroy(cr);	/* -- free cairo_t */

    return (FALSE);
}

/* update the knob by forcing an expose */
static void
swamigui_knob_update(SwamiguiKnob *knob)
{
    GdkRegion *region;
    GtkWidget *widget = GTK_WIDGET(knob);

    if(!widget->window)
    {
        return;
    }

    region = gdk_drawable_get_clip_region(widget->window);

    /* redraw the cairo canvas completely by exposing it */
    gdk_window_invalidate_region(widget->window, region, TRUE);
    gdk_window_process_updates(widget->window, TRUE);

    gdk_region_destroy(region);
}

static gboolean
swamigui_knob_button_press_event(GtkWidget *widget, GdkEventButton *event)
{
    SwamiguiKnob *knob = SWAMIGUI_KNOB(widget);

    if(event->type != GDK_BUTTON_PRESS || event->button != 1)
    {
        return (FALSE);
    }

    if(gdk_pointer_grab(widget->window, FALSE,
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                        | GDK_POINTER_MOTION_MASK, NULL, NULL, GDK_CURRENT_TIME)
            != GDK_GRAB_SUCCESS)
    {
        return (TRUE);
    }

    knob->rotation_active = TRUE;
    knob->xclick = event->x;
    knob->yclick = event->y;
    knob->click_rotation = knob->rotation;

    return (TRUE);
}

static gboolean
swamigui_knob_button_release_event(GtkWidget *widget, GdkEventButton *event)
{
    SwamiguiKnob *knob = SWAMIGUI_KNOB(widget);

    if(event->type != GDK_BUTTON_RELEASE || event->button != 1)
    {
        return (FALSE);
    }

    knob->rotation_active = FALSE;

    gdk_pointer_ungrab(GDK_CURRENT_TIME);

    return (TRUE);
}

static gboolean
swamigui_knob_motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
    SwamiguiKnob *knob = SWAMIGUI_KNOB(widget);
    double rotation, normval;

    if(!widget->window || !knob->rotation_active)
    {
        return (TRUE);
    }

    /* calculate rotation amount (SHIFT key causes a finer rotation) */
    rotation = knob->click_rotation
               + (knob->yclick - event->y)
               / ((event->state & GDK_SHIFT_MASK) ? knob->rotation_rate_fine
                  : knob->rotation_rate);

    rotation = CLAMP(rotation, knob->start_pos, knob->end_pos);

    if(rotation == knob->rotation)
    {
        return (TRUE);
    }

    knob->rotation = rotation;

    /* normalize rotation to a 0.0 to 1.0 position */
    normval = (rotation - knob->start_pos) / (knob->end_pos - knob->start_pos);

    /* update the adjustment value */
    knob->adj->value = normval * (knob->adj->upper - knob->adj->lower)
                       + knob->adj->lower;

    /* emit value-changed signal (block our own handler) */
    g_signal_handlers_block_by_func(knob->adj, swamigui_knob_adj_value_changed,
                                    knob);
    gtk_adjustment_value_changed(knob->adj);
    g_signal_handlers_unblock_by_func(knob->adj, swamigui_knob_adj_value_changed,
                                      knob);
    /* update knob visual */
    swamigui_knob_update(knob);

    return (TRUE);
}

/* callback which gets called when the knob adjustment value changes externally */
static void
swamigui_knob_adj_value_changed(GtkAdjustment *adj, gpointer user_data)
{
    SwamiguiKnob *knob = SWAMIGUI_KNOB(user_data);
    gdouble normval;

    /* normalize the adjustment value to 0.0 through 1.0 */
    normval = (adj->value - adj->lower) / (adj->upper - adj->lower);
    normval = CLAMP(normval, 0.0, 1.0);

    /* convert to a rotation amount */
    knob->rotation = normval * (knob->end_pos - knob->start_pos)
                     + knob->start_pos;

    swamigui_knob_update(knob);	/* update knob visual */
}

/**
 * swamigui_knob_new:
 *
 * Create a new knob widget.
 *
 * Returns: Knob widget
 */
GtkWidget *
swamigui_knob_new(void)
{
    return (g_object_new(SWAMIGUI_TYPE_KNOB, NULL));
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
swamigui_knob_get_adjustment(SwamiguiKnob *knob)
{
    g_return_val_if_fail(SWAMIGUI_IS_KNOB(knob), NULL);

    return (GTK_ADJUSTMENT(knob->adj));
}
