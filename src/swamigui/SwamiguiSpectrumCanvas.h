/*
 * SwamiguiSpectrumCanvas.h - Spectrum frequency canvas item
 * A canvas item for displaying frequency spectrum data
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
#ifndef __SWAMIGUI_SPECTRUM_CANVAS_H__
#define __SWAMIGUI_SPECTRUM_CANVAS_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiSpectrumCanvas SwamiguiSpectrumCanvas;
typedef struct _SwamiguiSpectrumCanvasClass SwamiguiSpectrumCanvasClass;

#define SWAMIGUI_TYPE_SPECTRUM_CANVAS   (swamigui_spectrum_canvas_get_type ())
#define SWAMIGUI_SPECTRUM_CANVAS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_SPECTRUM_CANVAS, \
			       SwamiguiSpectrumCanvas))
#define SWAMIGUI_SPECTRUM_CANVAS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_SPECTRUM_CANVAS, \
   SwamiguiSpectrumCanvasClass))
#define SWAMIGUI_IS_SPECTRUM_CANVAS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_SPECTRUM_CANVAS))
#define SWAMIGUI_IS_SPECTRUM_CANVAS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_SPECTRUM_CANVAS))

/**
 * SwamiguiSpectrumDestroyNotify:
 * @spectrum: The spectrum data pointer as passed to
 *   swamigui_spectrum_canvas_set_data().
 * @size: The number of values in the @spectrum array
 *
 * This is a function type that gets called when a spectrum canvas item
 * is destroyed. This function is responsible for freeing @spectrum.
 */
typedef void (*SwamiguiSpectrumDestroyNotify)(double *spectrum, guint size);

/* Spectrum canvas item */
struct _SwamiguiSpectrumCanvas
{
    GnomeCanvasItem parent_instance;

    /*< private >*/

    double *spectrum;		/* spectrum data */
    guint spectrum_size;		/* number of values in spectrum data */
    SwamiguiSpectrumDestroyNotify notify;	/* notify function for spectrum data */

    double max_value;		/* maximum value in spectrum data */

    GtkAdjustment *adj;		/* adjustment for view */
    gboolean update_adj;	/* TRUE if adj should be updated (to stop loop) */

    guint start;			/* start spectrum index */
    double zoom;			/* zoom factor indexes/pixel */
    double zoom_ampl;		/* amplitude zoom factor */
    int x, y;			/* x, y coordinates of spectrum item */
    int width, height;		/* width and height in pixels */
    GdkGC *min_gc;		/* GC for drawing minimum lines */
    GdkGC *max_gc;		/* GC for drawing maximum lines */
    GdkGC *bar_gc;		/* GC for spectrum bars (zoom < 1.0) */
    guint need_bbox_update : 1;	/* set if bbox needs to be updated */
};

struct _SwamiguiSpectrumCanvasClass
{
    GnomeCanvasItemClass parent_class;
};


GType swamigui_spectrum_canvas_get_type(void);
void swamigui_spectrum_canvas_set_data(SwamiguiSpectrumCanvas *canvas,
                                       double *spectrum, guint size,
                                       SwamiguiSpectrumDestroyNotify notify);
int swamigui_spectrum_canvas_pos_to_spectrum(SwamiguiSpectrumCanvas *canvas,
        int xpos);
int swamigui_spectrum_canvas_spectrum_to_pos(SwamiguiSpectrumCanvas *canvas,
        int index);

#endif
