/*
 * SwamiguiSampleCanvas.c - Sample data canvas widget
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
#include <stdio.h>
#include <gtk/gtk.h>

#include "SwamiguiSampleCanvas.h"
#include "util.h"
#include "i18n.h"

/* Sample format used internally */
#define SAMPLE_FORMAT   IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_ENDIAN_HOST | IPATCH_SAMPLE_MONO

enum
{
    PROP_0,
    PROP_SAMPLE,			/* sample to display */
    PROP_RIGHT_CHAN,	/* if sample data is stereo: use right channel? */
    PROP_LOOP_MODE,		/* set loop mode */
    PROP_LOOP_START,		/* start of loop */
    PROP_LOOP_END,		/* end of loop */
    PROP_ADJUSTMENT,		/* adjustment control */
    PROP_UPDATE_ADJ,		/* update adjustment values? */
    PROP_X,			/* x position in pixels */
    PROP_Y,			/* y position in pixels */
    PROP_WIDTH,			/* width of view in pixels */
    PROP_HEIGHT,			/* height of view in pixels */
    PROP_START,			/* sample start position */
    PROP_ZOOM,			/* zoom value */
    PROP_ZOOM_AMPL,		/* amplitude zoom */
    PROP_PEAK_LINE_COLOR,		/* color of peak sample lines */
    PROP_LINE_COLOR,		/* color of connecting sample lines */
    PROP_POINT_COLOR,		/* color of sample points */
    PROP_LOOP_START_COLOR,	/* color of sample points for start of loop */
    PROP_LOOP_END_COLOR		/* color of sample points for end of loop */
};

#define DEFAULT_PEAK_LINE_COLOR		GNOME_CANVAS_COLOR (63, 69, 255)
#define DEFAULT_LINE_COLOR		GNOME_CANVAS_COLOR (63, 69, 255)
#define DEFAULT_POINT_COLOR		GNOME_CANVAS_COLOR (170, 170, 255)
#define DEFAULT_LOOP_START_COLOR	GNOME_CANVAS_COLOR (0, 255, 0)
#define DEFAULT_LOOP_END_COLOR		GNOME_CANVAS_COLOR (255, 0, 0)


static void swamigui_sample_canvas_finalize(GObject *object);
static void swamigui_sample_canvas_set_property(GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec);
static void swamigui_sample_canvas_get_property(GObject *object,
        guint property_id,
        GValue *value, GParamSpec *pspec);
static void swamigui_sample_canvas_update(GnomeCanvasItem *item,
        double *affine,
        ArtSVP *clip_path, int flags);
static void swamigui_sample_canvas_realize(GnomeCanvasItem *item);
static void set_gc_rgb(GdkGC *gc, guint color);
static void swamigui_sample_canvas_draw(GnomeCanvasItem *item,
                                        GdkDrawable *drawable,
                                        int x, int y, int width, int height);
static inline void
swamigui_sample_canvas_draw_loop(SwamiguiSampleCanvas *canvas,
                                 GdkDrawable *drawable,
                                 int x, int y, int width, int height);
static inline void
swamigui_sample_canvas_draw_points(SwamiguiSampleCanvas *canvas,
                                   GdkDrawable *drawable,
                                   int x, int y, int width, int height);
static inline void
swamigui_sample_canvas_draw_segments(SwamiguiSampleCanvas *canvas,
                                     GdkDrawable *drawable,
                                     int x, int y, int width, int height);

static double swamigui_sample_canvas_point(GnomeCanvasItem *item,
        double x, double y,
        int cx, int cy,
        GnomeCanvasItem **actual_item);
static void swamigui_sample_canvas_bounds(GnomeCanvasItem *item,
        double *x1, double *y1,
        double *x2, double *y2);
static void
swamigui_sample_canvas_cb_adjustment_value_changed(GtkAdjustment *adj,
        gpointer user_data);
static gboolean
swamigui_sample_canvas_real_set_sample(SwamiguiSampleCanvas *canvas,
                                       IpatchSampleData *sample);
static void
swamigui_sample_canvas_update_adjustment(SwamiguiSampleCanvas *canvas);

G_DEFINE_TYPE(SwamiguiSampleCanvas, swamigui_sample_canvas, GNOME_TYPE_CANVAS_ITEM)

static void
swamigui_sample_canvas_class_init(SwamiguiSampleCanvasClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);
    GnomeCanvasItemClass *item_class = GNOME_CANVAS_ITEM_CLASS(klass);

    obj_class->set_property = swamigui_sample_canvas_set_property;
    obj_class->get_property = swamigui_sample_canvas_get_property;
    obj_class->finalize = swamigui_sample_canvas_finalize;

    item_class->update = swamigui_sample_canvas_update;
    item_class->realize = swamigui_sample_canvas_realize;
    item_class->draw = swamigui_sample_canvas_draw;
    item_class->point = swamigui_sample_canvas_point;
    item_class->bounds = swamigui_sample_canvas_bounds;

    /* FIXME - Is there a better way to define an interface object property? */
    g_object_class_install_property(obj_class, PROP_SAMPLE,
                                    g_param_spec_object("sample", _("Sample"), _("Sample object"),
                                            IPATCH_TYPE_SAMPLE_DATA, G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_RIGHT_CHAN,
                                    g_param_spec_boolean("right-chan", _("Right Channel"),
                                            _("Use right channel of stereo samples"),
                                            FALSE, G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_LOOP_MODE,
                                    g_param_spec_boolean("loop-mode", _("Loop Mode"),
                                            _("Enable/disable loop mode"),
                                            FALSE, G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_LOOP_START,
                                    g_param_spec_uint("loop-start", _("Loop Start"),
                                            _("Start of loop in samples"),
                                            0, G_MAXUINT, 0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_LOOP_END,
                                    g_param_spec_uint("loop-end", _("Loop end"),
                                            _("End of loop in samples"),
                                            0, G_MAXUINT, 0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_ADJUSTMENT,
                                    g_param_spec_object("adjustment", _("Adjustment"),
                                            _("Adjustment control for scrolling"),
                                            GTK_TYPE_ADJUSTMENT,
                                            G_PARAM_READWRITE));

    /* FIXME - better way to handle this?? Problems with recursive updates when
       multiple SwamiguiSampleCanvas items are on a canvas. */
    g_object_class_install_property(obj_class, PROP_UPDATE_ADJ,
                                    g_param_spec_boolean("update-adj", _("Update adjustment"),
                                            _("Update adjustment object"),
                                            FALSE, G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_X,
                                    g_param_spec_int("x", "X",
                                            _("X position in pixels"),
                                            0, G_MAXINT, 0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_Y,
                                    g_param_spec_int("y", "Y",
                                            _("Y position in pixels"),
                                            0, G_MAXINT, 0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_WIDTH,
                                    g_param_spec_int("width", _("Width"),
                                            _("Width in pixels"),
                                            0, G_MAXINT, 1,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_HEIGHT,
                                    g_param_spec_int("height", _("Height"),
                                            _("Height in pixels"),
                                            0, G_MAXINT, 1,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_START,
                                    g_param_spec_uint("start", _("View Start"),
                                            _("Start of view in samples"),
                                            0, G_MAXUINT, 0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_ZOOM,
                                    g_param_spec_double("zoom", _("Zoom"),
                                            _("Zoom factor in samples per pixel"),
                                            0.0, G_MAXDOUBLE, 1.0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_ZOOM_AMPL,
                                    g_param_spec_double("zoom-ampl", _("Zoom Amplitude"),
                                            _("Amplitude zoom factor"),
                                            0.0, G_MAXDOUBLE, 1.0,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_PEAK_LINE_COLOR,
                                    ipatch_param_set(g_param_spec_uint("peak-line-color", _("Peak line color"),
                                            _("Color of peak sample lines"),
                                            0, G_MAXUINT, DEFAULT_PEAK_LINE_COLOR,
                                            G_PARAM_READWRITE),
                                            "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
    g_object_class_install_property(obj_class, PROP_LINE_COLOR,
                                    ipatch_param_set(g_param_spec_uint("line-color", _("Line color"),
                                            _("Color of sample connecting lines"),
                                            0, G_MAXUINT, DEFAULT_LINE_COLOR,
                                            G_PARAM_READWRITE),
                                            "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
    g_object_class_install_property(obj_class, PROP_POINT_COLOR,
                                    ipatch_param_set(g_param_spec_uint("point-color", _("Point color"),
                                            _("Color of sample points"),
                                            0, G_MAXUINT, DEFAULT_POINT_COLOR,
                                            G_PARAM_READWRITE),
                                            "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
    g_object_class_install_property(obj_class, PROP_LOOP_START_COLOR,
                                    ipatch_param_set(g_param_spec_uint("loop-start-color", _("Loop start color"),
                                            _("Color of loop start sample points"),
                                            0, G_MAXUINT, DEFAULT_LOOP_START_COLOR,
                                            G_PARAM_READWRITE),
                                            "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
    g_object_class_install_property(obj_class, PROP_LOOP_END_COLOR,
                                    ipatch_param_set(g_param_spec_uint("loop-end-color", _("Loop end color"),
                                            _("Color of loop end sample points"),
                                            0, G_MAXUINT, DEFAULT_LOOP_END_COLOR,
                                            G_PARAM_READWRITE),
                                            "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
}

static void
swamigui_sample_canvas_init(SwamiguiSampleCanvas *canvas)
{
    canvas->adj =
        (GtkAdjustment *)gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    g_object_ref(canvas->adj);

    canvas->update_adj = FALSE;

    g_signal_connect(canvas->adj, "value-changed",
                     G_CALLBACK
                     (swamigui_sample_canvas_cb_adjustment_value_changed),
                     canvas);

    canvas->sample = NULL;
    canvas->sample_size = 0;
    canvas->right_chan = FALSE;
    canvas->loop_mode = FALSE;
    canvas->loop_start = 0;
    canvas->loop_end = 1;

    canvas->start = 0;
    canvas->zoom = 1.0;
    canvas->zoom_ampl = 1.0;
    canvas->x = 0;
    canvas->y = 0;
    canvas->width = 0;
    canvas->height = 0;

    canvas->peak_line_color = DEFAULT_PEAK_LINE_COLOR;
    canvas->line_color = DEFAULT_LINE_COLOR;
    canvas->point_color = DEFAULT_POINT_COLOR;
    canvas->loop_start_color = DEFAULT_LOOP_START_COLOR;
    canvas->loop_end_color = DEFAULT_LOOP_END_COLOR;
}

static void
swamigui_sample_canvas_finalize(GObject *object)
{
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(object);

    if(canvas->sample)
    {
        ipatch_sample_handle_close(&canvas->handle);
        g_object_unref(canvas->sample);
    }

    if(canvas->adj)
    {
        g_signal_handlers_disconnect_by_func
        (canvas->adj, G_CALLBACK
         (swamigui_sample_canvas_cb_adjustment_value_changed), canvas);
        g_object_unref(canvas->adj);
    }

    if(canvas->peak_line_gc)
    {
        g_object_unref(canvas->peak_line_gc);
    }

    if(canvas->line_gc)
    {
        g_object_unref(canvas->line_gc);
    }

    if(canvas->point_gc)
    {
        g_object_unref(canvas->point_gc);
    }

    if(canvas->loop_start_gc)
    {
        g_object_unref(canvas->loop_start_gc);
    }

    if(canvas->loop_end_gc)
    {
        g_object_unref(canvas->loop_end_gc);
    }

    if(G_OBJECT_CLASS(swamigui_sample_canvas_parent_class)->finalize)
    {
        (*G_OBJECT_CLASS(swamigui_sample_canvas_parent_class)->finalize)(object);
    }
}

static void
swamigui_sample_canvas_set_property(GObject *object, guint property_id,
                                    const GValue *value, GParamSpec *pspec)
{
    GnomeCanvasItem *item = GNOME_CANVAS_ITEM(object);
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(object);
    IpatchSampleData *sample;
    GtkAdjustment *gtkadj;
    gboolean b;

    switch(property_id)
    {
    case PROP_SAMPLE:
        swamigui_sample_canvas_real_set_sample(canvas, IPATCH_SAMPLE_DATA
                                               (g_value_get_object(value)));
        break;

    case PROP_RIGHT_CHAN:
        b = g_value_get_boolean(value);

        if(b != canvas->right_chan)
        {
            canvas->right_chan = b;

            if(canvas->sample)      /* Unset and then re-assign the sample */
            {
                sample = g_object_ref(canvas->sample);        /* ++ temp ref */
                swamigui_sample_canvas_real_set_sample(canvas, NULL);
                swamigui_sample_canvas_real_set_sample(canvas, sample);
                g_object_unref(canvas->sample);               /* -- temp unref */
            }
        }

        break;

    case PROP_LOOP_MODE:
        canvas->loop_mode = g_value_get_boolean(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_LOOP_START:
        canvas->loop_start = g_value_get_uint(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_LOOP_END:
        canvas->loop_end = g_value_get_uint(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_ADJUSTMENT:
        gtkadj = g_value_get_object(value);
        g_return_if_fail(GTK_IS_ADJUSTMENT(gtkadj));

        g_signal_handlers_disconnect_by_func
        (canvas->adj, G_CALLBACK
         (swamigui_sample_canvas_cb_adjustment_value_changed), canvas);
        g_object_unref(canvas->adj);

        canvas->adj = GTK_ADJUSTMENT(g_object_ref(gtkadj));
        g_signal_connect(canvas->adj, "value-changed",
                         G_CALLBACK
                         (swamigui_sample_canvas_cb_adjustment_value_changed),
                         canvas);

        /* only initialize adjustment if updates enabled */
        if(canvas->update_adj)
        {
            swamigui_sample_canvas_update_adjustment(canvas);
        }

        break;

    case PROP_UPDATE_ADJ:
        canvas->update_adj = g_value_get_boolean(value);
        break;

    case PROP_X:
        canvas->x = g_value_get_int(value);
        canvas->need_bbox_update = TRUE;
        gnome_canvas_item_request_update(item);
        break;

    case PROP_Y:
        canvas->y = g_value_get_int(value);
        canvas->need_bbox_update = TRUE;
        gnome_canvas_item_request_update(item);
        break;

    case PROP_WIDTH:
        canvas->width = g_value_get_int(value);
        canvas->need_bbox_update = TRUE;
        gnome_canvas_item_request_update(item);

        /* only update adjustment if updates enabled */
        if(canvas->update_adj)
        {
            canvas->adj->page_size = canvas->width * canvas->zoom;
            gtk_adjustment_changed(canvas->adj);
        }

        break;

    case PROP_HEIGHT:
        canvas->height = g_value_get_int(value);
        canvas->need_bbox_update = TRUE;
        gnome_canvas_item_request_update(item);
        break;

    case PROP_START:
        canvas->start = g_value_get_uint(value);
        gnome_canvas_item_request_update(item);

        /* only update adjustment if updates enabled */
        if(canvas->update_adj)
        {
            canvas->adj->value = canvas->start;
            g_signal_handlers_block_by_func(canvas->adj,
                                            swamigui_sample_canvas_cb_adjustment_value_changed,
                                            canvas);
            gtk_adjustment_value_changed(canvas->adj);
            g_signal_handlers_unblock_by_func(canvas->adj,
                                              swamigui_sample_canvas_cb_adjustment_value_changed,
                                              canvas);
        }

        break;

    case PROP_ZOOM:
        canvas->zoom = g_value_get_double(value);
        gnome_canvas_item_request_update(item);

        /* only update adjustment if updates enabled */
        if(canvas->update_adj)
        {
            canvas->adj->page_size = canvas->width * canvas->zoom;
            gtk_adjustment_changed(canvas->adj);
        }

        break;

    case PROP_ZOOM_AMPL:
        canvas->zoom_ampl = g_value_get_double(value);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_PEAK_LINE_COLOR:
        canvas->peak_line_color = g_value_get_uint(value);
        set_gc_rgb(canvas->peak_line_gc, canvas->peak_line_color);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_LINE_COLOR:
        canvas->line_color = g_value_get_uint(value);
        set_gc_rgb(canvas->line_gc, canvas->line_color);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_POINT_COLOR:
        canvas->point_color = g_value_get_uint(value);
        set_gc_rgb(canvas->point_gc, canvas->point_color);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_LOOP_START_COLOR:
        canvas->loop_start_color = g_value_get_uint(value);
        set_gc_rgb(canvas->loop_start_gc, canvas->loop_start_color);
        gnome_canvas_item_request_update(item);
        break;

    case PROP_LOOP_END_COLOR:
        canvas->loop_end_color = g_value_get_uint(value);
        set_gc_rgb(canvas->loop_end_gc, canvas->loop_end_color);
        gnome_canvas_item_request_update(item);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        return;			/* return, to skip code below */
    }
}

static void
swamigui_sample_canvas_get_property(GObject *object, guint property_id,
                                    GValue *value, GParamSpec *pspec)
{
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(object);

    switch(property_id)
    {
    case PROP_SAMPLE:
        g_value_set_object(value, canvas->sample);
        break;

    case PROP_RIGHT_CHAN:
        g_value_set_boolean(value, canvas->right_chan);
        break;

    case PROP_LOOP_MODE:
        g_value_set_boolean(value, canvas->loop_mode);
        break;

    case PROP_LOOP_START:
        g_value_set_uint(value, canvas->loop_start);
        break;

    case PROP_LOOP_END:
        g_value_set_uint(value, canvas->loop_end);
        break;

    case PROP_ADJUSTMENT:
        g_value_set_object(value, canvas->adj);
        break;

    case PROP_UPDATE_ADJ:
        g_value_set_boolean(value, canvas->update_adj);
        break;

    case PROP_X:
        g_value_set_int(value, canvas->x);
        break;

    case PROP_Y:
        g_value_set_int(value, canvas->y);
        break;

    case PROP_WIDTH:
        g_value_set_int(value, canvas->width);
        break;

    case PROP_HEIGHT:
        g_value_set_int(value, canvas->height);
        break;

    case PROP_START:
        g_value_set_uint(value, canvas->start);
        break;

    case PROP_ZOOM:
        g_value_set_double(value, canvas->zoom);
        break;

    case PROP_ZOOM_AMPL:
        g_value_set_double(value, canvas->zoom_ampl);
        break;

    case PROP_PEAK_LINE_COLOR:
        g_value_set_uint(value, canvas->peak_line_color);
        break;

    case PROP_LINE_COLOR:
        g_value_set_uint(value, canvas->line_color);
        break;

    case PROP_POINT_COLOR:
        g_value_set_uint(value, canvas->point_color);
        break;

    case PROP_LOOP_START_COLOR:
        g_value_set_uint(value, canvas->loop_start_color);
        break;

    case PROP_LOOP_END_COLOR:
        g_value_set_uint(value, canvas->loop_end_color);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

/* Gnome canvas item update handler */
static void
swamigui_sample_canvas_update(GnomeCanvasItem *item, double *affine,
                              ArtSVP *clip_path, int flags)
{
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(item);

    if(((flags & GNOME_CANVAS_UPDATE_VISIBILITY)
            && !(GTK_OBJECT_FLAGS(item) & GNOME_CANVAS_ITEM_VISIBLE))
            || (flags & GNOME_CANVAS_UPDATE_AFFINE)
            || canvas->need_bbox_update)
    {
        canvas->need_bbox_update = FALSE;
        gnome_canvas_update_bbox(item, canvas->x, canvas->y,
                                 canvas->x + canvas->width,
                                 canvas->y + canvas->height);
    }
    else
        gnome_canvas_request_redraw(item->canvas, canvas->x, canvas->y,
                                    canvas->x + canvas->width,
                                    canvas->y + canvas->height);

    if(GNOME_CANVAS_ITEM_CLASS(swamigui_sample_canvas_parent_class)->update)
        GNOME_CANVAS_ITEM_CLASS(swamigui_sample_canvas_parent_class)->update
        (item, affine, clip_path, flags);
}

static void
swamigui_sample_canvas_realize(GnomeCanvasItem *item)
{
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(item);
    GdkDrawable *drawable;

    if(GNOME_CANVAS_ITEM_CLASS(swamigui_sample_canvas_parent_class)->realize)
    {
        GNOME_CANVAS_ITEM_CLASS(swamigui_sample_canvas_parent_class)->realize(item);
    }

    if(canvas->peak_line_gc)
    {
        return;    /* return if GCs already created */
    }

    drawable = item->canvas->layout.bin_window;

    canvas->peak_line_gc = gdk_gc_new(drawable);  /* ++ ref */
    set_gc_rgb(canvas->peak_line_gc, canvas->peak_line_color);

    canvas->line_gc = gdk_gc_new(drawable);  /* ++ ref */
    set_gc_rgb(canvas->line_gc, canvas->line_color);

    canvas->point_gc = gdk_gc_new(drawable); /* ++ ref */
    set_gc_rgb(canvas->point_gc, canvas->point_color);

    canvas->loop_start_gc = gdk_gc_new(drawable);  /* ++ ref */
    set_gc_rgb(canvas->loop_start_gc, canvas->loop_start_color);

    canvas->loop_end_gc = gdk_gc_new(drawable);  /* ++ ref */
    set_gc_rgb(canvas->loop_end_gc, canvas->loop_end_color);
    gdk_gc_set_function(canvas->loop_end_gc, GDK_XOR);
}

/* sets fg color of a Graphics Context to 32 bit GnomeCanvas style RGB value */
static void
set_gc_rgb(GdkGC *gc, guint color)
{
    GdkColor gdkcolor;

    gdkcolor.pixel = 0;
    gdkcolor.red = (((color >> 24) & 0xFF) * 65535) / 255;
    gdkcolor.green = (((color >> 16) & 0xFF) * 65535) / 255;
    gdkcolor.blue = (((color >> 8) & 0xFF) * 65535) / 255;

    gdk_gc_set_rgb_fg_color(gc, &gdkcolor);
}

/* GnomeCanvas draws in 512 pixel squares, allocate some extra for overlap */
#define STATIC_POINTS 544

static void
swamigui_sample_canvas_draw(GnomeCanvasItem *item, GdkDrawable *drawable,
                            int x, int y, int width, int height)
{
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(item);
    GdkRectangle rect;

    if(!canvas->sample)
    {
        return;
    }

    /* set GC clipping rectangle */
    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height;

    gdk_gc_set_clip_rectangle(canvas->peak_line_gc, &rect);
    gdk_gc_set_clip_rectangle(canvas->line_gc, &rect);
    gdk_gc_set_clip_rectangle(canvas->point_gc, &rect);
    gdk_gc_set_clip_rectangle(canvas->loop_start_gc, &rect);
    gdk_gc_set_clip_rectangle(canvas->loop_end_gc, &rect);

    if(canvas->loop_mode)
    {
        swamigui_sample_canvas_draw_loop(canvas, drawable, x, y, width, height);
    }
    else if(canvas->zoom <= 1.0)
    {
        swamigui_sample_canvas_draw_points(canvas, drawable, x, y, width, height);
    }
    else
    {
        swamigui_sample_canvas_draw_segments(canvas, drawable, x, y, width, height);
    }
}

/* loop mode display "connect the dots", zoom clamped <= 1.0.
   Overlaps display of start and end points of a loop */
static inline void
swamigui_sample_canvas_draw_loop(SwamiguiSampleCanvas *canvas,
                                 GdkDrawable *drawable,
                                 int x, int y, int width, int height)
{
    gint16 *i16buf;
    int sample_ofs, sample_count, this_size;
    int hcenter, height_1, point_width = 0, h_point_width;
    int loop_index, start_index, end_index, start_ofs, end_ofs;
    int sample_size;
    double sample_mul;
    int xpos, ypos, i;
    GdkGC *gc;
    gboolean do_loopend_bool = TRUE;

    if(canvas->width < 6)
    {
        return;    /* hackish? */
    }

    sample_size = (int)canvas->sample_size;

    hcenter = canvas->width / 2 + canvas->x; /* horizontal center */
    height_1 = canvas->height - 1; /* height - 1 */
    sample_mul = height_1 / (double)65535.0; /* sample amplitude multiplier */

    /* use larger squares for sample points? */
    if(canvas->zoom < 1.0 / 6.0)
    {
        point_width = 5;
    }
    else if(canvas->zoom < 1.0 / 4.0)
    {
        point_width = 3;
    }

    h_point_width = point_width >> 1;

    /* calculate start and end offset, in sample points, from center to the
       left and right edges of the rectangular drawing area (-/+ 1 for tiling) */
    start_ofs = (int)((x - hcenter) * canvas->zoom - 1);
    end_ofs = (int)((x + width - hcenter) * canvas->zoom + 1);

    /* do loop start point first, jump to 'do_loopend' label will occur for
       loop end point */
    loop_index = canvas->loop_start;
    gc = canvas->loop_start_gc;

do_loopend:		       /* jump label for end loop point run */

    /* calculate loop point start/end sample indexes in view */
    start_index = loop_index + start_ofs;
    end_index = loop_index + end_ofs;

    /* are there any sample points in view for loop start? */
    if(start_index < sample_size && end_index >= 0)
    {
        start_index = CLAMP(start_index, 0, sample_size - 1);
        end_index = CLAMP(end_index, 0, sample_size - 1);
        sample_count = end_index - start_index + 1;

        this_size = canvas->max_frames;
        sample_ofs = start_index;

        while(sample_count > 0)
        {
            if(sample_count < this_size)
            {
                this_size = sample_count;
            }

            if(!(i16buf = ipatch_sample_handle_read(&canvas->handle, sample_ofs,
                                                    this_size, NULL, NULL)))
            {
                return;    /* FIXME - Error reporting?? */
            }

            for(i = 0; i < this_size; i++, sample_ofs++)
            {
                /* calculate pixel offset from center */
                xpos = (int)((sample_ofs - loop_index) / canvas->zoom + 0.5);
                xpos += hcenter - x; /* adjust to drawable coordinates */

                /* calculate amplitude ypos */
                ypos = (int)(height_1 - (((int)i16buf[i]) + 32768) * sample_mul - y
                             + canvas->y);

                if(point_width)
                    gdk_draw_rectangle(drawable, gc, TRUE,
                                       xpos - h_point_width, ypos - h_point_width,
                                       point_width, point_width);
                else
                {
                    gdk_draw_point(drawable, gc, xpos, ypos);
                }
            }

            sample_count -= this_size;
        }
    }

    if(do_loopend_bool)
    {
        loop_index = canvas->loop_end;
        gc = canvas->loop_end_gc;
        do_loopend_bool = FALSE;
        goto do_loopend;
    }
}

/* "connect the dots" drawing for zooms <= 1.0 */
static inline void
swamigui_sample_canvas_draw_points(SwamiguiSampleCanvas *canvas,
                                   GdkDrawable *drawable,
                                   int x, int y, int width, int height)
{
    GdkPoint static_points[STATIC_POINTS];
    GdkPoint *points;
    int sample_start, sample_end,  point_index;
    guint sample_count;
    int height_1;
    guint sample_ofs, size_left, this_size;
    double sample_mul;
    gint16 *i16buf;
    guint i;
    int sample_size = canvas->sample_size;

    /* calculate start sample (-1 for tiling) */
    sample_start = (int)(canvas->start + x * canvas->zoom);

    /* calculate end sample (+1 for tiling) */
    sample_end = (int)(canvas->start + ((x + width) * canvas->zoom) + 1);

    /* no samples in area? */
    if(sample_start >= sample_size || sample_end < 0)
    {
        return;
    }

    sample_start = CLAMP(sample_start, 0, sample_size - 1);
    sample_end = CLAMP(sample_end, 0, sample_size - 1);
    sample_count = sample_end - sample_start + 1;

    height_1 = canvas->height - 1; /* height - 1 */
    sample_mul = height_1 / (double)65535.0; /* sample amplitude multiplier */

    /* use static point array if there is enough, otherwise fall back on
       malloc (shouldn't get used, but just in case) */
    if(sample_count > STATIC_POINTS)
    {
        points = g_new(GdkPoint, sample_count);
    }
    else
    {
        points = static_points;
    }

    sample_ofs = sample_start;
    size_left = sample_count;
    this_size = canvas->max_frames;
    point_index = 0;

    while(size_left > 0)
    {
        if(size_left < this_size)
        {
            this_size = size_left;
        }

        if(!(i16buf = ipatch_sample_handle_read(&canvas->handle, sample_ofs,
                                                this_size, NULL, NULL)))
        {
            if(points != static_points)
            {
                g_free(points);
            }

            return;		/* FIXME - Error reporting?? */
        }

        /* loop over each sample */
        for(i = 0; i < this_size; i++, sample_ofs++, point_index++)
        {
            /* calculate xpos */
            points[point_index].x
                = (int)((sample_ofs - canvas->start) / canvas->zoom + 0.5) - x
                  + canvas->x;

            /* calculate amplitude ypos */
            points[point_index].y = (gint)(height_1 - (((int)i16buf[i]) + 32768)
                                           * sample_mul - y + canvas->y);
        }

        size_left -= this_size;
    }

    /* draw the connected lines between points */
    gdk_draw_lines(drawable, canvas->line_gc, points, sample_count);

    if(canvas->zoom < 1.0 / 4.0) /* use larger squares for points? */
    {
        int w, half_w;

        if(canvas->zoom < 1.0 / 6.0)
        {
            w = 5;
        }
        else
        {
            w = 3;
        }

        half_w = w >> 1;

        for(i = 0; i < sample_count; i++)
            gdk_draw_rectangle(drawable, canvas->point_gc, TRUE,
                               points[i].x - half_w, points[i].y - half_w,
                               w, w);
    } /* just use pixels for dots */
    else
    {
        gdk_draw_points(drawable, canvas->point_gc, points, sample_count);
    }

    if(points != static_points)
    {
        g_free(points);
    }
}

/* peak line segment drawing for zooms > 1.0 */
static inline void
swamigui_sample_canvas_draw_segments(SwamiguiSampleCanvas *canvas,
                                     GdkDrawable *drawable,
                                     int x, int y, int width, int height)
{
    GdkSegment static_segments[STATIC_POINTS];
    GdkSegment *segments;
    int sample_start, sample_end, sample_count, sample_ofs;
    int segment_index, next_index;
    int height_1;
    guint size_left, this_size;
    double sample_mul;
    gint16 *i16buf;
    gint16 min, max;
    guint i;
    int sample_size = canvas->sample_size;

    /* calculate start sample */
    sample_start = (int)(canvas->start + x * canvas->zoom + 0.5);

    /* calculate end sample */
    sample_end = (int)(canvas->start + (x + width) * canvas->zoom + 0.5);

    /* no samples in area? */
    if(sample_start >= sample_size || sample_end < 0)
    {
        return;
    }

    sample_start = CLAMP(sample_start, 0, sample_size - 1);
    sample_end = CLAMP(sample_end, 0, sample_size - 1);
    sample_count = sample_end - sample_start + 1;

    height_1 = canvas->height - 1; /* height - 1 */
    sample_mul = height_1 / (double)65535.0; /* sample amplitude multiplier */

    /* use static point array if there is enough, otherwise fall back on
       malloc (shouldn't get used, but just in case) */
    if(width > STATIC_POINTS)
    {
        segments = g_new(GdkSegment, width);
    }
    else
    {
        segments = static_segments;
    }

    sample_ofs = sample_start;
    size_left = sample_count;
    this_size = canvas->max_frames;
    segment_index = 0;
    min = max = 0;
    next_index = (int)(canvas->start + (x + 1) * canvas->zoom + 0.5);

    while(size_left > 0)
    {
        if(size_left < this_size)
        {
            this_size = size_left;
        }

        if(!(i16buf = ipatch_sample_handle_read(&canvas->handle, sample_ofs,
                                                this_size, NULL, NULL)))
        {
            if(segments != static_segments)
            {
                g_free(segments);
            }

            return;		/* FIXME - Error reporting?? */
        }

        /* look for min/max sample values */
        for(i = 0; i < this_size; i++, sample_ofs++)
        {
            if(sample_ofs >= next_index)
            {
                segments[segment_index].x1 = segment_index + canvas->x;
                segments[segment_index].x2 = segments[segment_index].x1;
                segments[segment_index].y1
                    = (gint)(height_1 - (((int)max) + 32768) * sample_mul - y + canvas->y);
                segments[segment_index].y2
                    = (gint)(height_1 - (((int)min) + 32768) * sample_mul - y + canvas->y);

                min = max = 0;
                segment_index++;
                next_index = (int)(canvas->start
                                   + (x + segment_index + 1) * canvas->zoom + 0.5);
            }

            if(i16buf[i] < min)
            {
                min = i16buf[i];
            }

            if(i16buf[i] > max)
            {
                max = i16buf[i];
            }
        }

        size_left -= this_size;
    }

    gdk_draw_segments(drawable, canvas->peak_line_gc, segments, segment_index);

    if(segments != static_segments)
    {
        g_free(segments);
    }
}

static double
swamigui_sample_canvas_point(GnomeCanvasItem *item, double x, double y,
                             int cx, int cy, GnomeCanvasItem **actual_item)
{
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(item);
    double points[2 * 4];

    points[0] = canvas->x;
    points[1] = canvas->y;
    points[2] = canvas->x + canvas->width;
    points[3] = points[1];
    points[4] = points[0];
    points[5] = canvas->y + canvas->height;
    points[6] = points[2];
    points[7] = points[5];

    *actual_item = item;

    return (gnome_canvas_polygon_to_point(points, 4, cx, cy));
}

static void
swamigui_sample_canvas_bounds(GnomeCanvasItem *item, double *x1, double *y1,
                              double *x2, double *y2)
{
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(item);

    *x1 = canvas->x;
    *y1 = canvas->y;
    *x2 = canvas->x + canvas->width;
    *y2 = canvas->y + canvas->height;
}

static void
swamigui_sample_canvas_cb_adjustment_value_changed(GtkAdjustment *adj,
        gpointer user_data)
{
    SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS(user_data);
    guint start;
    gboolean update_adj;

    update_adj = canvas->update_adj; /* store old value of update adjustment */

    canvas->update_adj = FALSE;	/* disable adjustment updates to stop
				   adjustment loop */
    start = (guint)adj->value;
    g_object_set(canvas, "start", start, NULL);

    canvas->update_adj = update_adj; /* restore update adjustment setting */
}

/**
 * swamigui_sample_canvas_set_sample:
 * @canvas: Sample data canvas item
 * @sample: Sample data to assign to the canvas
 *
 * Set the sample data source of a sample canvas item.
 */
void
swamigui_sample_canvas_set_sample(SwamiguiSampleCanvas *canvas,
                                  IpatchSampleData *sample)
{
    if(swamigui_sample_canvas_real_set_sample(canvas, sample))
    {
        g_object_notify(G_OBJECT(canvas), "sample");
    }
}

/* the real set sample function, returns TRUE if changed,
   FALSE otherwise */
static gboolean
swamigui_sample_canvas_real_set_sample(SwamiguiSampleCanvas *canvas,
                                       IpatchSampleData *sample)
{
    GError *err = NULL;
    int channel_map;

    g_return_val_if_fail(SWAMIGUI_IS_SAMPLE_CANVAS(canvas), FALSE);
    g_return_val_if_fail(!sample || IPATCH_IS_SAMPLE_DATA(sample), FALSE);

    if(sample == canvas->sample)
    {
        return (FALSE);
    }

    /* close previous source and unref sample */
    if(canvas->sample)
    {
        ipatch_sample_handle_close(&canvas->handle);
        g_object_unref(canvas->sample);
    }

    canvas->sample = NULL;

    if(sample)
    {
        g_object_get(sample, "sample-size", &canvas->sample_size, NULL);

        /* use right channel of stereo if right_chan is set (and stereo data) */
        if(canvas->right_chan && IPATCH_SAMPLE_FORMAT_GET_CHANNELS
                (ipatch_sample_data_get_native_format(sample)) == IPATCH_SAMPLE_STEREO)
        {
            channel_map = IPATCH_SAMPLE_MAP_CHANNEL(0, IPATCH_SAMPLE_RIGHT);
        }
        else
        {
            channel_map = IPATCH_SAMPLE_MAP_CHANNEL(0, IPATCH_SAMPLE_LEFT);
        }

        if(!ipatch_sample_data_open_cache_sample(sample, &canvas->handle, SAMPLE_FORMAT,
                channel_map, &err))
        {
            g_critical(_("Error opening cached sample data in sample canvas: %s"),
                       ipatch_gerror_message(err));
            g_error_free(err);
            return (FALSE);
        }

        canvas->sample = g_object_ref(sample);    /* ++ ref sample for canvas */
        canvas->max_frames = ipatch_sample_handle_get_max_frames(&canvas->handle);
    }
    else
    {
        canvas->sample_size = 0;
    }

    gnome_canvas_item_request_update(GNOME_CANVAS_ITEM(canvas));

    return (TRUE);
}

static void
swamigui_sample_canvas_update_adjustment(SwamiguiSampleCanvas *canvas)
{
    canvas->adj->lower = 0.0;
    canvas->adj->upper = canvas->sample_size;
    canvas->adj->value = 0.0;
    canvas->adj->step_increment = canvas->sample_size / 400.0;
    canvas->adj->page_increment = canvas->sample_size / 50.0;
    canvas->adj->page_size = canvas->sample_size;

    gtk_adjustment_changed(canvas->adj);
    g_signal_handlers_block_by_func(canvas->adj,
                                    swamigui_sample_canvas_cb_adjustment_value_changed,
                                    canvas);
    gtk_adjustment_value_changed(canvas->adj);
    g_signal_handlers_unblock_by_func(canvas->adj,
                                      swamigui_sample_canvas_cb_adjustment_value_changed,
                                      canvas);
}

/**
 * swamigui_sample_canvas_xpos_to_sample:
 * @canvas: Sample canvas item
 * @xpos: X pixel position
 * @onsample: Output: Pointer to store value indicating if given @xpos is
 *   within sample (0 if within sample, -1 if less than 0 or no active sample,
 *   1 if off the end, 2 if last value after sample - useful for loop end
 *   which is valid up to the position following the data), %NULL to ignore
 *
 * Convert an X pixel position to sample index.
 *
 * Returns: Sample index, index may be out of range of sample, use @onsample
 * parameter to determine that.
 */
int
swamigui_sample_canvas_xpos_to_sample(SwamiguiSampleCanvas *canvas, int xpos,
                                      int *onsample)
{
    int index, sample_size;

    if(onsample)
    {
        *onsample = -1;
    }

    g_return_val_if_fail(SWAMIGUI_IS_SAMPLE_CANVAS(canvas), 0);

    index = (int)(canvas->start + canvas->zoom * xpos);
    sample_size = canvas->sample_size;

    if(onsample && canvas->sample)
    {
        if(index < 0)
        {
            *onsample = -1;
        }
        else if(index > sample_size)
        {
            *onsample = 1;
        }
        else if(index == sample_size)
        {
            *onsample = 2;
        }
        else
        {
            *onsample = 0;
        }
    }

    return (index);
}

/**
 * swamigui_sample_canvas_sample_to_xpos:
 * @canvas: Sample canvas item
 * @index: Sample index
 * @inview: Output: Pointer to store value indicating if given sample @index
 *   is in view (0 if in view, -1 if too low, 1 if too high).
 *
 * Convert a sample index to x pixel position.
 *
 * Returns: X position.  Note that values outside of current view may be
 *   returned (including negative numbers), @inview can be used to determine
 *   if value is in view or not.
 */
int
swamigui_sample_canvas_sample_to_xpos(SwamiguiSampleCanvas *canvas, int index,
                                      int *inview)
{
    int xpos, start;

    g_return_val_if_fail(SWAMIGUI_IS_SAMPLE_CANVAS(canvas), 0);
    start = canvas->start;
    xpos = (int)((index - start) / canvas->zoom + 0.5);

    if(inview)
    {
        /* set inview output parameter as appropriate */
        if(index < start)
        {
            *inview = -1;
        }
        else if(xpos >= canvas->width)
        {
            *inview = 1;
        }
        else
        {
            *inview = 0;
        }
    }

    return (xpos);
}
