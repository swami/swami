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
  PROP_START,			/* sample start position */
  PROP_ZOOM,			/* zoom value */
  PROP_ZOOM_AMPL,		/* amplitude zoom */
  PROP_PEAK_LINE_COLOR,		/* color of peak sample lines */
  PROP_LINE_COLOR,		/* color of connecting sample lines */
  PROP_POINT_COLOR,		/* color of sample points */
  PROP_LOOP_START_COLOR,	/* color of sample points for start of loop */
  PROP_LOOP_END_COLOR		/* color of sample points for end of loop */
};

#define DEFAULT_PEAK_LINE_COLOR		SWAMIGUI_RGB (63, 69, 255)
#define DEFAULT_LINE_COLOR		SWAMIGUI_RGB (63, 69, 255)
#define DEFAULT_POINT_COLOR		SWAMIGUI_RGB (170, 170, 255)
#define DEFAULT_LOOP_START_COLOR	SWAMIGUI_RGB (0, 255, 0)
#define DEFAULT_LOOP_END_COLOR		SWAMIGUI_RGB (255, 0, 0)

static void swamigui_sample_canvas_finalize (GObject *object);
static void swamigui_sample_canvas_set_property (GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec);
static void swamigui_sample_canvas_get_property (GObject *object,
						guint property_id,
					 GValue *value, GParamSpec *pspec);

static gboolean swamigui_sample_canvas_draw (GtkWidget *widget, cairo_t *cr);
static void swamigui_sample_canvas_draw_loop (SwamiguiSampleCanvas *canvas, cairo_t *cr);
static void swamigui_sample_canvas_draw_points (SwamiguiSampleCanvas *canvas, cairo_t *cr);
static void swamigui_sample_canvas_draw_segments (SwamiguiSampleCanvas *canvas, cairo_t *cr);

static gboolean
swamigui_sample_canvas_real_set_sample (SwamiguiSampleCanvas *canvas,
					IpatchSampleData *sample);

G_DEFINE_TYPE (SwamiguiSampleCanvas, swamigui_sample_canvas, GTK_TYPE_DRAWING_AREA)

static void
swamigui_sample_canvas_class_init (SwamiguiSampleCanvasClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widg_class = GTK_WIDGET_CLASS (klass);

  obj_class->set_property = swamigui_sample_canvas_set_property;
  obj_class->get_property = swamigui_sample_canvas_get_property;
  obj_class->finalize = swamigui_sample_canvas_finalize;

  widg_class->draw = swamigui_sample_canvas_draw;

  /* FIXME - Is there a better way to define an interface object property? */
  g_object_class_install_property (obj_class, PROP_SAMPLE,
		g_param_spec_object ("sample", _("Sample"), _("Sample object"),
				     IPATCH_TYPE_SAMPLE_DATA, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_RIGHT_CHAN,
		g_param_spec_boolean ("right-chan", _("Right Channel"),
				      _("Use right channel of stereo samples"),
				      FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOOP_MODE,
		g_param_spec_boolean ("loop-mode", _("Loop Mode"),
				      _("Enable/disable loop mode"),
				      FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOOP_START,
		g_param_spec_uint ("loop-start", _("Loop Start"),
				   _("Start of loop in samples"),
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOOP_END,
		g_param_spec_uint ("loop-end", _("Loop end"),
				   _("End of loop in samples"),
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_START,
		g_param_spec_uint ("start", _("View Start"),
				   _("Start of view in samples"),
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ZOOM,
		g_param_spec_double ("zoom", _("Zoom"),
				   _("Zoom factor in samples per pixel"),
				   0.0, G_MAXDOUBLE, 1.0,
				   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ZOOM_AMPL,
		g_param_spec_double ("zoom-ampl", _("Zoom Amplitude"),
				   _("Amplitude zoom factor"),
				   0.0, G_MAXDOUBLE, 1.0,
				   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PEAK_LINE_COLOR,
    ipatch_param_set (g_param_spec_uint ("peak-line-color", _("Peak line color"),
					 _("Color of peak sample lines"),
					 0, G_MAXUINT, DEFAULT_PEAK_LINE_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_LINE_COLOR,
    ipatch_param_set (g_param_spec_uint ("line-color", _("Line color"),
					 _("Color of sample connecting lines"),
					 0, G_MAXUINT, DEFAULT_LINE_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_POINT_COLOR,
    ipatch_param_set (g_param_spec_uint ("point-color", _("Point color"),
					 _("Color of sample points"),
					 0, G_MAXUINT, DEFAULT_POINT_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_LOOP_START_COLOR,
    ipatch_param_set (g_param_spec_uint ("loop-start-color", _("Loop start color"),
					 _("Color of loop start sample points"),
					 0, G_MAXUINT, DEFAULT_LOOP_START_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_LOOP_END_COLOR,
    ipatch_param_set (g_param_spec_uint ("loop-end-color", _("Loop end color"),
					 _("Color of loop end sample points"),
					 0, G_MAXUINT, DEFAULT_LOOP_END_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
}

static void
swamigui_sample_canvas_init (SwamiguiSampleCanvas *canvas)
{
  canvas->sample = NULL;
  canvas->sample_size = 0;
  canvas->right_chan = FALSE;
  canvas->loop_mode = FALSE;
  canvas->loop_start = 0;
  canvas->loop_end = 1;

  canvas->start = 0;
  canvas->zoom = 1.0;
  canvas->zoom_ampl = 1.0;

  canvas->peak_line_color = DEFAULT_PEAK_LINE_COLOR;
  canvas->line_color = DEFAULT_LINE_COLOR;
  canvas->point_color = DEFAULT_POINT_COLOR;
  canvas->loop_start_color = DEFAULT_LOOP_START_COLOR;
  canvas->loop_end_color = DEFAULT_LOOP_END_COLOR;
}

static void
swamigui_sample_canvas_finalize (GObject *object)
{
  SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS (object);

  if (canvas->sample)
    {
      ipatch_sample_handle_close (&canvas->handle);
      g_object_unref (canvas->sample);
    }

  if (G_OBJECT_CLASS (swamigui_sample_canvas_parent_class)->finalize)
    (*G_OBJECT_CLASS (swamigui_sample_canvas_parent_class)->finalize) (object);
}

static void
swamigui_sample_canvas_set_property (GObject *object, guint property_id,
				     const GValue *value, GParamSpec *pspec)
{
  SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS (object);
  IpatchSampleData *sample;
  gboolean b;

  switch (property_id)
    {
    case PROP_SAMPLE:
      swamigui_sample_canvas_real_set_sample (canvas, IPATCH_SAMPLE_DATA
					      (g_value_get_object (value)));
      break;
    case PROP_RIGHT_CHAN:
      b = g_value_get_boolean (value);

      if (b != canvas->right_chan)
      {
        canvas->right_chan = b;

	if (canvas->sample)     /* Unset and then re-assign the sample */
        {
          sample = g_object_ref (canvas->sample);       /* ++ temp ref */
          swamigui_sample_canvas_real_set_sample (canvas, NULL);
          swamigui_sample_canvas_real_set_sample (canvas, sample);
          g_object_unref (canvas->sample);              /* -- temp unref */
        }
      }
      break;
    case PROP_LOOP_MODE:
      canvas->loop_mode = g_value_get_boolean (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_LOOP_START:
      canvas->loop_start = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_LOOP_END:
      canvas->loop_end = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_START:
      canvas->start = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_ZOOM:
      canvas->zoom = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_ZOOM_AMPL:
      canvas->zoom_ampl = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_PEAK_LINE_COLOR:
      canvas->peak_line_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_LINE_COLOR:
      canvas->line_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_POINT_COLOR:
      canvas->point_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_LOOP_START_COLOR:
      canvas->loop_start_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    case PROP_LOOP_END_COLOR:
      canvas->loop_end_color = g_value_get_uint (value);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;			/* return, to skip code below */
    }
}

static void
swamigui_sample_canvas_get_property (GObject *object, guint property_id,
				    GValue *value, GParamSpec *pspec)
{
  SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS (object);

  switch (property_id)
    {
    case PROP_SAMPLE:
      g_value_set_object (value, canvas->sample);
      break;
    case PROP_RIGHT_CHAN:
      g_value_set_boolean (value, canvas->right_chan);
      break;
    case PROP_LOOP_MODE:
      g_value_set_boolean (value, canvas->loop_mode);
      break;
    case PROP_LOOP_START:
      g_value_set_uint (value, canvas->loop_start);
      break;
    case PROP_LOOP_END:
      g_value_set_uint (value, canvas->loop_end);
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
    case PROP_PEAK_LINE_COLOR:
      g_value_set_uint (value, canvas->peak_line_color);
      break;
    case PROP_LINE_COLOR:
      g_value_set_uint (value, canvas->line_color);
      break;
    case PROP_POINT_COLOR:
      g_value_set_uint (value, canvas->point_color);
      break;
    case PROP_LOOP_START_COLOR:
      g_value_set_uint (value, canvas->loop_start_color);
      break;
    case PROP_LOOP_END_COLOR:
      g_value_set_uint (value, canvas->loop_end_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
swamigui_sample_canvas_draw (GtkWidget *widget, cairo_t *cr)
{
  SwamiguiSampleCanvas *canvas = SWAMIGUI_SAMPLE_CANVAS (widget);

  if (!canvas->sample) return FALSE;

  if (canvas->loop_mode)
    swamigui_sample_canvas_draw_loop (canvas, cr);
  else if (canvas->zoom <= 1.0)
    swamigui_sample_canvas_draw_points (canvas, cr);
  else
    swamigui_sample_canvas_draw_segments(canvas, cr);

  return FALSE;
}

/* loop mode display "connect the dots", zoom clamped <= 1.0.
   Overlaps display of start and end points of a loop */
static void
swamigui_sample_canvas_draw_loop (SwamiguiSampleCanvas *canvas, cairo_t *cr)
{
  gint16 *i16buf;
  int sample_ofs, sample_count, this_size;
  int hcenter, height_1;
  int loop_index, start_index, end_index, start_ofs, end_ofs;
  double xpos, ypos, sample_mul, point_width, point_ofs;
  int width, height;
  int sample_size;
  int i;
  gboolean do_loopend_bool = TRUE;

  width = gtk_widget_get_allocated_width ((GtkWidget *)canvas);
  height = gtk_widget_get_allocated_height ((GtkWidget *)canvas);

  sample_size = (int)canvas->sample_size;

  hcenter = width / 2;                  /* horizontal center */
  height_1 = height - 1;                /* height - 1 */
  sample_mul = height_1 / (double)65535.0; /* sample amplitude multiplier */

  /* Set size of sample points based on zoom level */
  if (canvas->zoom < 1.0 / 6.0) point_width = 5.0;
  else if (canvas->zoom < 1.0 / 4.0) point_width = 3.0;
  else point_width = 1.0;

  point_ofs = point_width / 2.0 - 0.5;

  /* calculate start and end offset, in sample points, from center to the
     left and right edges of the rectangular drawing area (-/+ 1 for tiling) */
  start_ofs = hcenter * canvas->zoom - 1;
  end_ofs = (width - hcenter) * canvas->zoom + 1;

  /* do loop start point first, jump to 'do_loopend' label will occur for
     loop end point */
  loop_index = canvas->loop_start;

 do_loopend:		       /* jump label for end loop point run */

  /* calculate loop point start/end sample indexes in view */
  start_index = loop_index + start_ofs;
  end_index = loop_index + end_ofs;

  /* are there any sample points in view for loop start? */
  if (start_index < sample_size && end_index >= 0)
    {
      start_index = CLAMP (start_index, 0, sample_size - 1);
      end_index = CLAMP (end_index, 0, sample_size - 1);
      sample_count = end_index - start_index + 1;

      this_size = canvas->max_frames;
      sample_ofs = start_index;

      while (sample_count > 0)
	{
	  if (sample_count < this_size) this_size = sample_count;

	  if (!(i16buf = ipatch_sample_handle_read (&canvas->handle, sample_ofs, this_size, NULL, NULL)))
	    return;		/* FIXME - Error reporting?? */

	  for (i = 0; i < this_size; i++, sample_ofs++)
	    {
	      xpos = (sample_ofs - loop_index) / canvas->zoom + hcenter - point_ofs;    /* calculate pixel offset from center */
	      ypos = height_1 - (((int)i16buf[i]) + 32768) * sample_mul - point_ofs;    /* calculate amplitude ypos */
              cairo_rectangle (cr, xpos, ypos, point_width, point_width);
	    }

	  sample_count -= this_size;
	}
    }

  if (do_loopend_bool)
    {
      swamigui_util_set_cairo_rgba (cr, canvas->loop_start_color);
      cairo_fill (cr);

      loop_index = canvas->loop_end;
      do_loopend_bool = FALSE;

      goto do_loopend;
    }
  else
    {
      swamigui_util_set_cairo_rgba (cr, canvas->loop_end_color);
      cairo_fill (cr);
    }
}

/* "connect the dots" drawing for zooms <= 1.0 */
static void
swamigui_sample_canvas_draw_points (SwamiguiSampleCanvas *canvas, cairo_t *cr)
{
  int sample_start, sample_end, sample_count, point_index;
  guint sample_ofs, size_left, this_size;
  double xpos, ypos, sample_mul, point_width, point_ofs;
  cairo_path_t *path;
  gint16 *i16buf;
  int height_1;
  int i;

  sample_start = canvas->start;

  /* calculate end sample (+1 for tiling) */
  sample_end = canvas->start
    + gtk_widget_get_allocated_width ((GtkWidget *)canvas) * canvas->zoom + 1;

  /* no samples in area? */
  if (sample_start >= canvas->sample_size || sample_end < 0) return;

  sample_start = CLAMP (sample_start, 0, canvas->sample_size - 1);
  sample_end = CLAMP (sample_end, 0, canvas->sample_size - 1);
  sample_count = sample_end - sample_start + 1;

  height_1 = gtk_widget_get_allocated_height ((GtkWidget *)canvas) - 1;
  sample_mul = height_1 / (double)65535.0;      /* sample amplitude multiplier */

  /* Set size of sample points based on zoom level */
  if (canvas->zoom < 1.0 / 6.0) point_width = 5.0;
  else if (canvas->zoom < 1.0 / 4.0) point_width = 3.0;
  else point_width = 1.0;

  point_ofs = point_width / 2.0 - 0.5;

  sample_ofs = sample_start;
  this_size = canvas->max_frames;
  point_index = 0;

  // Create the sample lines between points
  for (size_left = sample_count; size_left > 0; size_left -= this_size)
  {
    if (size_left < this_size) this_size = size_left;

    if (!(i16buf = ipatch_sample_handle_read (&canvas->handle, sample_ofs, this_size, NULL, NULL)))
      return;		/* FIXME - Error reporting?? */

    /* loop over each sample */
    for (i = 0; i < this_size; i++, sample_ofs++, point_index++)
    {
      xpos = (sample_ofs - canvas->start) / canvas->zoom;
      ypos = height_1 - (((int)i16buf[i]) + 32768) * sample_mul;
      cairo_line_to (cr, xpos, ypos);
    }
  }

  path = cairo_copy_path (cr);  // ++ Copy current line path to use for points drawing

  // Draw the sample lines
  swamigui_util_set_cairo_rgba (cr, canvas->line_color);
  cairo_stroke (cr);

  swamigui_util_set_cairo_rgba (cr, canvas->point_color);

  // Draw the sample points
  for (i = 0; i < path->num_data; i += path->data[i].header.length)
    cairo_rectangle (cr, path->data[i + 1].point.x - point_ofs, path->data[i + 1].point.y - point_ofs,
                     point_width, point_width);

  cairo_fill (cr);

  cairo_path_destroy (path);    // -- Destroy the allocated path
}

/* peak line segment drawing for zooms > 1.0 */
static void
swamigui_sample_canvas_draw_segments (SwamiguiSampleCanvas *canvas, cairo_t *cr)
{
  int sample_start, sample_end, sample_count, sample_ofs;
  int segment_index, next_index;
  guint size_left, this_size;
  double sample_mul;
  gint16 *i16buf;
  gint16 min, max;
  int height_1;
  int i;

  sample_start = canvas->start * canvas->zoom + 0.5;
  sample_end = canvas->start
    + gtk_widget_get_allocated_width ((GtkWidget *)canvas) * canvas->zoom + 0.5;

  /* no samples in area? */
  if (sample_start >= canvas->sample_size || sample_end < 0) return;

  sample_start = CLAMP (sample_start, 0, canvas->sample_size - 1);
  sample_end = CLAMP (sample_end, 0, canvas->sample_size - 1);
  sample_count = sample_end - sample_start + 1;

  height_1 = gtk_widget_get_allocated_height ((GtkWidget *)canvas) - 1;
  sample_mul = height_1 / (double)65535.0; /* sample amplitude multiplier */

  sample_ofs = sample_start;
  this_size = canvas->max_frames;
  segment_index = 0;
  min = max = 0;
  next_index = canvas->start + canvas->zoom + 0.5;

  // Create the sample vertical line segments
  for (size_left = sample_count; size_left > 0; size_left -= this_size)
  {
    if (size_left < this_size) this_size = size_left;

    if (!(i16buf = ipatch_sample_handle_read (&canvas->handle, sample_ofs,
                                              this_size, NULL, NULL)))
      return;           /* FIXME - Error reporting?? */

    /* look for min/max sample values */
    for (i = 0; i < this_size; i++, sample_ofs++)
    {
      if (sample_ofs >= next_index)
      {
        cairo_move_to (cr, segment_index,
                       height_1 - (((int)max) + 32768) * sample_mul);
        cairo_line_to (cr, segment_index,
                       height_1 - (((int)min) + 32768) * sample_mul);

        min = max = 0;
        segment_index++;
        next_index = canvas->start + (segment_index + 1) * canvas->zoom + 0.5;
      }

      if (i16buf[i] < min) min = i16buf[i];
      if (i16buf[i] > max) max = i16buf[i];
    }
  }

  // Draw the line segments
  swamigui_util_set_cairo_rgba (cr, canvas->peak_line_color);
  cairo_stroke (cr);
}

/**
 * swamigui_sample_canvas_set_sample:
 * @canvas: Sample data canvas item
 * @sample: Sample data to assign to the canvas
 *
 * Set the sample data source of a sample canvas item.
 */
void
swamigui_sample_canvas_set_sample (SwamiguiSampleCanvas *canvas,
				   IpatchSampleData *sample)
{
  if (swamigui_sample_canvas_real_set_sample (canvas, sample))
    g_object_notify (G_OBJECT (canvas), "sample");
}

/* the real set sample function, returns TRUE if changed,
   FALSE otherwise */
static gboolean
swamigui_sample_canvas_real_set_sample (SwamiguiSampleCanvas *canvas,
					IpatchSampleData *sample)
{
  GError *err = NULL;
  int channel_map;

  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_CANVAS (canvas), FALSE);
  g_return_val_if_fail (!sample || IPATCH_IS_SAMPLE_DATA (sample), FALSE);

  if (sample == canvas->sample) return (FALSE);

  /* close previous source and unref sample */
  if (canvas->sample)
    {
      ipatch_sample_handle_close (&canvas->handle);
      g_object_unref (canvas->sample);
    }

  canvas->sample = NULL;

  if (sample)
    {
      g_object_get (sample, "sample-size", &canvas->sample_size, NULL);

      /* use right channel of stereo if right_chan is set (and stereo data) */
      if (canvas->right_chan && IPATCH_SAMPLE_FORMAT_GET_CHANNELS
	  (ipatch_sample_data_get_native_format (sample)) == IPATCH_SAMPLE_STEREO)
	channel_map = IPATCH_SAMPLE_MAP_CHANNEL (0, IPATCH_SAMPLE_RIGHT);
      else channel_map = IPATCH_SAMPLE_MAP_CHANNEL (0, IPATCH_SAMPLE_LEFT);

      if (!ipatch_sample_data_open_cache_sample (sample, &canvas->handle, SAMPLE_FORMAT,
                                                 channel_map, &err))
      {
        g_critical (_("Error opening cached sample data in sample canvas: %s"),
                    ipatch_gerror_message (err));
        g_error_free (err);
        return (FALSE);
      }

      canvas->sample = g_object_ref (sample);   /* ++ ref sample for canvas */
      canvas->max_frames = ipatch_sample_handle_get_max_frames (&canvas->handle);
    }
  else canvas->sample_size = 0;

  gtk_widget_queue_draw (GTK_WIDGET (canvas));

  return (TRUE);
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
swamigui_sample_canvas_xpos_to_sample (SwamiguiSampleCanvas *canvas, int xpos,
				       int *onsample)
{
  int index;

  if (onsample) *onsample = -1;

  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_CANVAS (canvas), 0);

  index = canvas->start + canvas->zoom * xpos;

  if (onsample && canvas->sample)
  {
    if (index < 0) *onsample = -1;
    else if (index > canvas->sample_size) *onsample = 1;
    else if (index == canvas->sample_size) *onsample = 2;
    else *onsample = 0;
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
swamigui_sample_canvas_sample_to_xpos (SwamiguiSampleCanvas *canvas, int index,
				       int *inview)
{
  int xpos, width;

  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_CANVAS (canvas), 0);

  xpos = (index - canvas->start) / canvas->zoom + 0.5;
  width = gtk_widget_get_allocated_width (GTK_WIDGET (canvas));

  if (inview)
  {
    /* set inview output parameter as appropriate */
    if (index < canvas->start) *inview = -1;
    else if (xpos >= width) *inview = 1;
    else *inview = 0;
  }

  return (xpos);
}
