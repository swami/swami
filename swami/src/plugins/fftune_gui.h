/*
 * fftune_gui.h - FFTune GUI object (Fast Fourier sample tuning GUI)
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
#ifndef __FFTUNE_GUI_H__
#define __FFTUNE_GUI_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>
#include <libswami/SwamiControl.h>

#include "fftune.h"

typedef struct _FFTuneGui FFTuneGui;
typedef struct _FFTuneGuiClass FFTuneGuiClass;

#define FFTUNE_TYPE_GUI   (fftune_gui_get_type ())
#define FFTUNE_GUI(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FFTUNE_TYPE_GUI, FFTuneGui))
#define FFTUNE_GUI_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), FFTUNE_TYPE_GUI, FFTuneGuiClass))
#define FFTUNE_IS_GUI(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FFTUNE_TYPE_GUI))
#define FFTUNE_IS_GUI_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), FFTUNE_TYPE_GUI))

/* Sample view object */
struct _FFTuneGui
{
  GtkVBox parent;		/* derived from GtkVBox */

  FFTuneSpectra *spectra;	/* FFTuneSpectra object (fftune.[ch]) */

  gboolean snap_active;	/* set to TRUE when a snap zoom/scroll is active */
  int snap_pos;			/* xpos of zoom/scroll snap line */
  guint snap_timeout_handler;	/* snap timeout callback handler ID */
  guint snap_interval; /* interval in milliseconds for timeout handler */

  gboolean scroll_active;      /* TRUE if SHIFT scrolling is active */
  gboolean zoom_active;		/* TRUE if CTRL zooming is active */
  int scroll_amt;      /* scroll amount for each timeout in samples */
  double zoom_amt;		/* zoom multiplier for each timeout */

  /* for mouse wheel scrolling */
  GdkScrollDirection last_wheel_dir; /* last wheel direction */
  guint32 last_wheel_time;	/* last time stamp of mouse wheel */

  GnomeCanvas *canvas;		/* canvas */
  GnomeCanvasItem *spectrum;	/* SwamiguiSpectrumCanvas item */
  GnomeCanvasItem *snap_line;	/* zoom/scroll snap line */
  gboolean recalc_zoom;	/* TRUE to recalc full zoom (after sample change) */

  GtkWidget *mode_menu;		/* data selection menu */
  GtkWidget *hscrollbar;	/* horizontal scrollbar widget */

  GtkListStore *freq_store;	/* store for freq tuning list */
  GtkWidget *freq_list;		/* GtkTreeView list of freq tuning suggestions */
  GtkWidget *vscale;            /* Vertical scale widget */

  GtkWidget *root_notesel;	/* root note selector widget */
  GtkWidget *fine_tune;		/* fine tune spin button */
  GtkWidget *revert_button;	/* Revert button */
  SwamiControl *root_note_ctrl;	/* root note Swami control */
  SwamiControl *fine_tune_ctrl;	/* fine tune Swami control */

  guint8 orig_root_note;	/* Original root note value */
  guint8 orig_fine_tune;	/* Original fine tune value */

  guint snap_line_color;	/* zoom/scroll snap line color */
};

/* Sample view object class */
struct _FFTuneGuiClass
{
  GtkVBoxClass parent_class;
};

GType fftune_gui_get_type (void);
GtkWidget *fftune_gui_new (void);

#endif
