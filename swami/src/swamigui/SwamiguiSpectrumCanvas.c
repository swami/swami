/*
 * SwamiguiSpectrumCanvas.c - Spectrum frequency canvas item
 * A canvas item for displaying frequency spectrum data
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
#include <stdio.h>
#include <gtk/gtk.h>

#include "SwamiguiSpectrumCanvas.h"
#include "util.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_ADJUSTMENT,		/* adjustment control */
  PROP_X,			/* x position in pixels */
  PROP_Y,			/* y position in pixels */
  PROP_WIDTH,			/* width of view in pixels */
  PROP_HEIGHT,			/* height of view in pixels */
  PROP_START,			/* spectrum start index */
  PROP_ZOOM,			/* zoom value */
  PROP_ZOOM_AMPL		/* amplitude zoom */
};

static void
swamigui_spectrum_canvas_class_init (SwamiguiSpectrumCanvasClass *klass);
static void swamigui_spectrum_canvas_init (SwamiguiSpectrumCanvas *canvas);
static void swamigui_spectrum_canvas_finalize (GObject *object);
static void swamigui_spectrum_canvas_set_property (GObject *object,
						   guint property_id,
						   const GValue *value,
						   GParamSpec *pspec);
static void swamigui_spectrum_canvas_get_property (GObject *object,
						   guint property_id,
						   GValue *value,
						   GParamSpec *pspec);
static void swamigui_spectrum_canvas_update (GnomeCanvasItem *item,
					     double *affine,
					     ArtSVP *clip_path, int flags);
static void swamigui_spectrum_canvas_realize (GnomeCanvasItem *item);
static void swamigui_spectrum_canvas_unrealize (GnomeCanvasItem *item);
static void swamigui_spectrum_canvas_draw (GnomeCanvasItem *item,
					   GdkDrawable *drawable,
					   int x, int y, int width, int height);

static double swamigui_spectrum_canvas_point (GnomeCanvasItem *item,
					      double x, double y,
					      int cx, int cy,
					      GnomeCanvasItem **actual_item);
static void swamigui_spectrum_canvas_bounds (GnomeCanvasItem *item,
					     double *x1, double *y1,
					     double *x2, double *y2);
static void
swamigui_spectrum_canvas_cb_adjustment_value_changed (GtkAdjustment *adj,
						      gpointer user_data);
static void
swamigui_spectrum_canvas_update_adjustment (SwamiguiSpectrumCanvas *canvas);

static GObjectClass *parent_class = NULL;

GType
swamigui_spectrum_canvas_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static const GTypeInfo obj_info =
	{
	  sizeof (SwamiguiSpectrumCanvasClass), NULL, NULL,
	  (GClassInitFunc) swamigui_spectrum_canvas_class_init, NULL, NULL,
	  sizeof (SwamiguiSpectrumCanvas), 0,
	  (GInstanceInitFunc) swamigui_spectrum_canvas_init,
	};

      obj_type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM,
					 "SwamiguiSpectrumCanvas",
					 &obj_info, 0);
    }

  return (obj_type);
}

static void
swamigui_spectrum_canvas_class_init (SwamiguiSpectrumCanvasClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GnomeCanvasItemClass *item_class = GNOME_CANVAS_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = swamigui_spectrum_canvas_set_property;
  obj_class->get_property = swamigui_spectrum_canvas_get_property;
  obj_class->finalize = swamigui_spectrum_canvas_finalize;

  item_class->update = swamigui_spectrum_canvas_update;
  item_class->realize = swamigui_spectrum_canvas_realize;
  item_class->unrealize = swamigui_spectrum_canvas_unrealize;
  item_class->draw = swamigui_spectrum_canvas_draw;
  item_class->point = swamigui_spectrum_canvas_point;
  item_class->bounds = swamigui_spectrum_canvas_bounds;

  g_object_class_install_property (obj_class, PROP_ADJUSTMENT,
		g_param_spec_object ("adjustment", _("Adjustment"),
				     _("Adjustment control for scrolling"),
				     GTK_TYPE_ADJUSTMENT,
				     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_X,
		g_param_spec_int ("x", "X",
				  _("X position in pixels"),
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_Y,
		g_param_spec_int ("y", "Y",
				  _("Y position in pixels"),
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_WIDTH,
		g_param_spec_int ("width", _("Width"),
				  _("Width in pixels"),
				  0, G_MAXINT, 1,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_HEIGHT,
		g_param_spec_int ("height", _("Height"),
				  _("Height in pixels"),
				  0, G_MAXINT, 1,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_START,
		g_param_spec_uint ("start", _("View start"),
				   _("Start index of spectrum in view"),
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ZOOM,
		g_param_spec_double ("zoom", _("Zoom"),
				   _("Zoom factor in indexes per pixel"),
				   0.0, G_MAXDOUBLE, 1.0,
				   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ZOOM_AMPL,
		g_param_spec_double ("zoom-ampl", _("Zoom Amplitude"),
				   _("Amplitude zoom factor"),
				   0.0, G_MAXDOUBLE, 1.0,
				   G_PARAM_READWRITE));
  /*
  g_object_class_install_property (obj_class, PROP_COLOR,
		g_param_spec_string ("color", _("Color"),
				     _("Spectrum color"),
				     NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (obj_class, PROP_COLOR_RGBA,
		g_param_spec_uint ("color-rgba", _("Color RGBA"),
				   _("Spectrum color RGBA"),
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE));
  */
}

static void
swamigui_spectrum_canvas_init (SwamiguiSpectrumCanvas *canvas)
{
  canvas->adj =
    (GtkAdjustment *)gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  g_object_ref (canvas->adj);

  canvas->update_adj = TRUE;

  g_signal_connect (canvas->adj, "value-changed",
		    G_CALLBACK
		    (swamigui_spectrum_canvas_cb_adjustment_value_changed),
		    canvas);

  canvas->spectrum = NULL;
  canvas->max_value = 0.0;

  canvas->start = 0;
  canvas->zoom = 1.0;
  canvas->zoom_ampl = 1.0;
  canvas->x = 0;
  canvas->y = 0;
  canvas->width = 0;
  canvas->height = 0;
}

static void
swamigui_spectrum_canvas_finalize (GObject *object)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (object);

  if (canvas->spectrum && canvas->notify)
    canvas->notify (canvas->spectrum, canvas->spectrum_size);

  if (canvas->adj)
    {
      g_signal_handlers_disconnect_by_func
	(canvas->adj, G_CALLBACK
	 (swamigui_spectrum_canvas_cb_adjustment_value_changed), canvas);
      g_object_unref (canvas->adj);
    }

  if (canvas->bar_gc) g_object_unref (canvas->bar_gc);
  if (canvas->min_gc) g_object_unref (canvas->min_gc);
  if (canvas->max_gc) g_object_unref (canvas->max_gc);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
swamigui_spectrum_canvas_set_property (GObject *object, guint property_id,
				       const GValue *value, GParamSpec *pspec)
{
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (object);
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (object);
  GtkAdjustment *gtkadj;

  switch (property_id)
    {
    case PROP_ADJUSTMENT:
      gtkadj = g_value_get_object (value);
      g_return_if_fail (GTK_IS_ADJUSTMENT (gtkadj));

      g_signal_handlers_disconnect_by_func
	(canvas->adj, G_CALLBACK
	 (swamigui_spectrum_canvas_cb_adjustment_value_changed), canvas);
      g_object_unref (canvas->adj);

      canvas->adj = GTK_ADJUSTMENT (g_object_ref (gtkadj));
      g_signal_connect (canvas->adj, "value-changed",
			G_CALLBACK
			(swamigui_spectrum_canvas_cb_adjustment_value_changed),
			canvas);

      /* only initialize adjustment if updates enabled */
      if (canvas->update_adj)
	swamigui_spectrum_canvas_update_adjustment (canvas);
      break;
    case PROP_X:
      canvas->x = g_value_get_int (value);
      canvas->need_bbox_update = TRUE;
      gnome_canvas_item_request_update (item);
      break;
    case PROP_Y:
      canvas->y = g_value_get_int (value);
      canvas->need_bbox_update = TRUE;
      gnome_canvas_item_request_update (item);
      break;
    case PROP_WIDTH:
      canvas->width = g_value_get_int (value);
      canvas->need_bbox_update = TRUE;
      gnome_canvas_item_request_update (item);

      /* only update adjustment if updates enabled */
      if (canvas->update_adj)
	{
	  canvas->adj->page_size = canvas->width * canvas->zoom;
	  gtk_adjustment_changed (canvas->adj);
	}
      break;
    case PROP_HEIGHT:
      canvas->height = g_value_get_int (value);
      canvas->need_bbox_update = TRUE;
      gnome_canvas_item_request_update (item);
      break;
    case PROP_START:
      canvas->start = g_value_get_uint (value);
      gnome_canvas_item_request_update (item);

      /* only update adjustment if updates enabled */
      if (canvas->update_adj)
	{
	  canvas->adj->value = canvas->start;
	  g_signal_handlers_block_by_func (canvas->adj,
			    swamigui_spectrum_canvas_cb_adjustment_value_changed,
					   canvas);
	  gtk_adjustment_value_changed (canvas->adj);
	  g_signal_handlers_unblock_by_func (canvas->adj,
			    swamigui_spectrum_canvas_cb_adjustment_value_changed,
					     canvas);
	}
      break;
    case PROP_ZOOM:
      canvas->zoom = g_value_get_double (value);
      gnome_canvas_item_request_update (item);

      /* only update adjustment if updates enabled */
      if (canvas->update_adj)
	{
	  canvas->adj->page_size = canvas->width * canvas->zoom;
	  gtk_adjustment_changed (canvas->adj);
	}
      break;
    case PROP_ZOOM_AMPL:
      canvas->zoom_ampl = g_value_get_double (value);
      gnome_canvas_item_request_update (item);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;			/* return, to skip code below */
    }
}

static void
swamigui_spectrum_canvas_get_property (GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (object);

  switch (property_id)
    {
    case PROP_ADJUSTMENT:
      g_value_set_object (value, canvas->adj);
      break;
    case PROP_X:
      g_value_set_int (value, canvas->x);
      break;
    case PROP_Y:
      g_value_set_int (value, canvas->y);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, canvas->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, canvas->height);
      break;
    case PROP_START:
      g_value_set_uint (value, canvas->start);
      break;
    case PROP_ZOOM:
      g_value_set_double (value, canvas->zoom);
      break;
    case PROP_ZOOM_AMPL:
      g_value_set_double (value, canvas->zoom_ampl);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* Gnome canvas item update handler */
static void
swamigui_spectrum_canvas_update (GnomeCanvasItem *item, double *affine,
				 ArtSVP *clip_path, int flags)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (item);

  if (((flags & GNOME_CANVAS_UPDATE_VISIBILITY)
       && !(GTK_OBJECT_FLAGS (item) & GNOME_CANVAS_ITEM_VISIBLE))
      || (flags & GNOME_CANVAS_UPDATE_AFFINE)
      || canvas->need_bbox_update)
    {
      canvas->need_bbox_update = FALSE;
      gnome_canvas_update_bbox (item, canvas->x, canvas->y,
				canvas->x + canvas->width,
				canvas->y + canvas->height);
    }
  else gnome_canvas_request_redraw (item->canvas, canvas->x, canvas->y,
				    canvas->x + canvas->width,
				    canvas->y + canvas->height);

  if (GNOME_CANVAS_ITEM_CLASS (parent_class)->update)
    GNOME_CANVAS_ITEM_CLASS (parent_class)->update (item, affine, clip_path,
						    flags);
}

static void
swamigui_spectrum_canvas_realize (GnomeCanvasItem *item)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (item);
  GdkColor bar_color = { 0x0, 0xFFFF, 0, 0 };
  GdkColor min_color = { 0x0, 0, 0, 0xFFFF };
  GdkColor max_color = { 0x0, 0, 0xFFFF, 0 };

  if (GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)
    (* GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)(item);

  canvas->bar_gc = gdk_gc_new (item->canvas->layout.bin_window); /* ++ ref */
  gdk_gc_set_rgb_fg_color (canvas->bar_gc, &bar_color);

  canvas->min_gc = gdk_gc_new (item->canvas->layout.bin_window); /* ++ ref */
  gdk_gc_set_rgb_fg_color (canvas->min_gc, &min_color);

  canvas->max_gc = gdk_gc_new (item->canvas->layout.bin_window); /* ++ ref */
  gdk_gc_set_rgb_fg_color (canvas->max_gc, &max_color);
}

static void
swamigui_spectrum_canvas_unrealize (GnomeCanvasItem *item)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (item);

  if (canvas->bar_gc) gdk_gc_unref (canvas->bar_gc);
  if (canvas->min_gc) gdk_gc_unref (canvas->min_gc);
  if (canvas->max_gc) gdk_gc_unref (canvas->max_gc);

  canvas->bar_gc = NULL;
  canvas->min_gc = NULL;
  canvas->max_gc = NULL;

  if (GNOME_CANVAS_ITEM_CLASS (parent_class)->unrealize)
    GNOME_CANVAS_ITEM_CLASS (parent_class)->unrealize (item);
}

/* GnomeCanvas draws in 512 pixel squares, allocate some extra for overlap */
#define STATIC_POINTS 544

static void
swamigui_spectrum_canvas_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			       int x, int y, int width, int height)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (item);
  GdkSegment static_segments[STATIC_POINTS];
  GdkSegment *segments;
  GdkRectangle rect;
  int size, start, end, index, next_index;
  int height_1, height_1_ofs;
  double ampl_mul;
  double min, max, val;
  int xpos, ypos, xofs, yofs;

  if (!canvas->spectrum) return;

  /* set GC clipping rectangle */
  rect.x = 0;
  rect.y = 0;
  rect.width = width;
  rect.height = height;

  size = canvas->spectrum_size;

  xofs = x - canvas->x;		/* x co-ordinate relative to spectrum pos */
  yofs = y - canvas->y;		/* y co-ordinate relative to spectrum pos */

  /* calculate start index */
  start = canvas->start + xofs * canvas->zoom + 0.5;

  /* calculate end index */
  end = canvas->start + (xofs + width) * canvas->zoom + 0.5;

  /* no spectrum in area? */
  if (start >= size || end < 0) return;

  start = CLAMP (start, 0, size - 1);
  end = CLAMP (end, 0, size - 1);

  height_1 = canvas->height - 1;  /* height - 1 */
  height_1_ofs = height_1 - yofs; /* height - 1 corrected to current ofs */

  /* spectrum amplitude multiplier */
  ampl_mul = height_1 * (canvas->zoom_ampl / canvas->max_value);

  if (canvas->zoom >= 1.0)
    {
      gdk_gc_set_clip_origin (canvas->min_gc, 0, 0);
      gdk_gc_set_clip_rectangle (canvas->min_gc, &rect);

      gdk_gc_set_clip_origin (canvas->max_gc, 0, 0);
      gdk_gc_set_clip_rectangle (canvas->max_gc, &rect);

      /* use static segment array if there is enough, otherwise fall back on
	 malloc (shouldn't get used, but just in case) */
      if (width > STATIC_POINTS) segments = g_new (GdkSegment, width);
      else segments = static_segments;

      max = -G_MAXDOUBLE;
      next_index = canvas->start + (xofs + 1) * canvas->zoom + 0.5;

      /* draw maximum lines */
      for (xpos = 0, index = start; xpos < width; xpos++)
	{
	  for (; index < next_index && index < size; index++)
	    {
	      val = canvas->spectrum[index];
	      if (val > max) max = val;
	    }

	  segments[xpos].x1 = xpos;
	  segments[xpos].x2 = xpos;
	  segments[xpos].y1 = height_1_ofs - max * ampl_mul;
	  segments[xpos].y2 = height_1_ofs;

	  if (index >= size) break;

	  next_index = canvas->start + (xofs + xpos + 2) * canvas->zoom + 0.5;
	  max = -G_MAXDOUBLE;
	}

      gdk_draw_segments (drawable, canvas->max_gc, segments, xpos);

      min = G_MAXDOUBLE;
      next_index = canvas->start + (xofs + 1) * canvas->zoom + 0.5;

      /* draw minimum lines */
      for (xpos = 0, index = start; xpos < width; xpos++)
	{
	  for (; index < next_index && index < size; index++)
	    {
	      val = canvas->spectrum[index];
	      if (val < min) min = val;
	    }

	  segments[xpos].x1 = xpos;
	  segments[xpos].x2 = xpos;
	  segments[xpos].y1 = height_1_ofs - min * ampl_mul;
	  segments[xpos].y2 = height_1_ofs;

	  if (index >= size) break;

	  next_index = canvas->start + (xofs + xpos + 2) * canvas->zoom + 0.5;
	  min = G_MAXDOUBLE;
	}

      gdk_draw_segments (drawable, canvas->min_gc, segments, xpos);

      if (segments != static_segments) g_free (segments);
    }
  else				/* zoom < 1.0 (do bars) */
    {
      gdk_gc_set_clip_origin (canvas->bar_gc, 0, 0);
      gdk_gc_set_clip_rectangle (canvas->bar_gc, &rect);

      if (start > 0) start--;	/* draw previous bar for overlap */

      /* first xpos co-ordinate */
      xpos = (start - (int)canvas->start) / canvas->zoom - xofs + 0.5;

      for (index = start; index <= end; index++)
	{
	  ypos = height_1_ofs - canvas->spectrum[index] * ampl_mul;
	  val = (index - canvas->start + 1) / canvas->zoom - xofs + 0.5;

	  gdk_draw_rectangle (drawable, canvas->bar_gc, TRUE,
			      xpos, ypos, val - xpos + 1,
			      height_1_ofs - ypos + 1);
	  xpos = val;
	}
    }
}

static double
swamigui_spectrum_canvas_point (GnomeCanvasItem *item, double x, double y,
				int cx, int cy, GnomeCanvasItem **actual_item)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (item);
  double points[2*4];

  points[0] = canvas->x;
  points[1] = canvas->y;
  points[2] = canvas->x + canvas->width;
  points[3] = points[1];
  points[4] = points[0];
  points[5] = canvas->y + canvas->height;
  points[6] = points[2];
  points[7] = points[5];

  *actual_item = item;

  return (gnome_canvas_polygon_to_point (points, 4, cx, cy));
}

static void
swamigui_spectrum_canvas_bounds (GnomeCanvasItem *item, double *x1, double *y1,
				 double *x2, double *y2)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (item);

  *x1 = canvas->x;
  *y1 = canvas->y;
  *x2 = canvas->x + canvas->width;
  *y2 = canvas->y + canvas->height;
}

static void
swamigui_spectrum_canvas_cb_adjustment_value_changed (GtkAdjustment *adj,
						      gpointer user_data)
{
  SwamiguiSpectrumCanvas *canvas = SWAMIGUI_SPECTRUM_CANVAS (user_data);
  guint start;

  canvas->update_adj = FALSE;	/* disable adjustment updates to stop
				   adjustment loop */
  start = adj->value;
  g_object_set (canvas, "start", start, NULL);

  canvas->update_adj = TRUE;	/* re-enable adjustment updates */
}

/**
 * swamigui_spectrum_canvas_set_data:
 * @canvas: Spectrum data canvas item
 * @spectrum: Spectrum data pointer
 * @size: Size of @spectrum data (in values, not bytes)
 * @notify: Function callback for freeing @spectrum data when spectrum
 *   canvas doesn't need it anymore.
 *
 * Set the spectrum data of a spectrum canvas item.
 */
void
swamigui_spectrum_canvas_set_data (SwamiguiSpectrumCanvas *canvas,
				   double *spectrum, guint size,
				   SwamiguiSpectrumDestroyNotify notify)
{
  g_return_if_fail (SWAMIGUI_IS_SPECTRUM_CANVAS (canvas));
  g_return_if_fail (!spectrum || size > 0);
  double max = 0.0;
  int i;

  if (spectrum == canvas->spectrum)
    return;

  /* call destroy notify if spectrum and notify is set */
  if (canvas->spectrum && canvas->notify)
    canvas->notify (canvas->spectrum, canvas->spectrum_size);

  canvas->spectrum = spectrum;
  canvas->spectrum_size = spectrum ? size : 0;
  canvas->notify = notify;

  /* find maximum value of spectrum */
  for (i = size - 1; i >= 0; i--)
    {
      if (spectrum[i] > max)
	max = spectrum[i];
    }

  canvas->max_value = max;

  swamigui_spectrum_canvas_update_adjustment (canvas);

  gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (canvas));
}

static void
swamigui_spectrum_canvas_update_adjustment (SwamiguiSpectrumCanvas *canvas)
{
  canvas->adj->lower = 0.0;
  canvas->adj->upper = canvas->spectrum_size;
  canvas->adj->value = 0.0;
  canvas->adj->step_increment = canvas->spectrum_size / 400.0;
  canvas->adj->page_increment = canvas->spectrum_size / 50.0;
  canvas->adj->page_size = canvas->spectrum_size;

  gtk_adjustment_changed (canvas->adj);
  g_signal_handlers_block_by_func (canvas->adj,
			swamigui_spectrum_canvas_cb_adjustment_value_changed,
				   canvas);
  gtk_adjustment_value_changed (canvas->adj);
  g_signal_handlers_unblock_by_func (canvas->adj,
			swamigui_spectrum_canvas_cb_adjustment_value_changed,
				     canvas);
}

/**
 * swamigui_spectrum_canvas_pos_to_spectrum:
 * @canvas: Spectrum canvas item
 * @xpos: X pixel position
 *
 * Convert an X pixel position to spectrum index.
 *
 * Returns: Spectrum index or -1 if out of range.
 */
int
swamigui_spectrum_canvas_pos_to_spectrum (SwamiguiSpectrumCanvas *canvas,
					  int xpos)
{
  int index;

  g_return_val_if_fail (SWAMIGUI_IS_SPECTRUM_CANVAS (canvas), -1);

  index = canvas->start + canvas->zoom * xpos;

  if (index < 0 || index > canvas->spectrum_size)
    return (-1);

  return (index);
}

/**
 * swamigui_spectrum_canvas_spectrum_to_pos:
 * @canvas: Spectrum canvas item
 * @index: Spectrum index
 *
 * Convert a spectrum index to x pixel position.
 *
 * Returns: X position, or -1 if out of view.
 */
int
swamigui_spectrum_canvas_spectrum_to_pos (SwamiguiSpectrumCanvas *canvas,
					  int index)
{
  int xpos;

  g_return_val_if_fail (SWAMIGUI_IS_SPECTRUM_CANVAS (canvas), -1);

  if (index < canvas->start) return -1;	/* index before view start? */
  xpos = (index - canvas->start) / canvas->zoom + 0.5;
  return (xpos < canvas->width) ? xpos : -1; /* return if not after view end */
}
