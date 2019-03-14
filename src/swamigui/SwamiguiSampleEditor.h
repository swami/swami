/*
 * SwamiguiSampleEditor.h - Sample editor widget header file
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
#ifndef __SWAMIGUI_SAMPLE_EDITOR_H__
#define __SWAMIGUI_SAMPLE_EDITOR_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <swamigui/SwamiguiSampleCanvas.h>
#include <swamigui/SwamiguiLoopFinder.h>
#include <swamigui/SwamiguiCanvasMod.h>
#include <swamigui/SwamiguiPanel.h>

typedef struct _SwamiguiSampleEditor SwamiguiSampleEditor;
typedef struct _SwamiguiSampleEditorClass SwamiguiSampleEditorClass;

#define SWAMIGUI_TYPE_SAMPLE_EDITOR   (swamigui_sample_editor_get_type ())
#define SWAMIGUI_SAMPLE_EDITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_SAMPLE_EDITOR, \
   SwamiguiSampleEditor))
#define SWAMIGUI_SAMPLE_EDITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_SAMPLE_EDITOR, \
   SwamiguiSampleEditorClass))
#define SWAMIGUI_IS_SAMPLE_EDITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_SAMPLE_EDITOR))
#define SWAMIGUI_IS_SAMPLE_EDITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_SAMPLE_EDITOR))

typedef enum
{
  SWAMIGUI_SAMPLE_EDITOR_NORMAL,	/* no particular status */
  SWAMIGUI_SAMPLE_EDITOR_INIT,		/* check selection and initialize */
  SWAMIGUI_SAMPLE_EDITOR_UPDATE		/* selection changed */
} SwamiguiSampleEditorStatus;

/**
 * SwamiguiSampleEditorHandler:
 * @editor: Sample editor widget
 *
 * This function type is used to handle specific patch item types with
 * sample data and loop info. The @editor object
 * <structfield>status</structfield> field indicates the current
 * operation which is one of:
 *
 * %SWAMIGUI_SAMPLE_EDITOR_INIT - Check selection and initialize the
 * sample editor if the selection can be handled. Return %TRUE if
 * selection was handled, which will activate this handler, %FALSE
 * otherwise.
 *
 * %SWAMIGUI_SAMPLE_EDITOR_UPDATE - Item selection has changed, update
 * sample editor. Return %TRUE if selection change was handled, %FALSE
 * otherwise which will de-activate this handler.
 *
 * Other useful fields of a #SwamiguiSplits object include
 * <structfield>selection</structfield> which defines the current item
 * selection.
 *
 * Returns: Should return %TRUE if operation was handled, %FALSE otherwise.
 */
typedef gboolean (*SwamiguiSampleEditorHandler)(SwamiguiSampleEditor *editor);

/* Sample editor object */
struct _SwamiguiSampleEditor
{
  GtkHBox parent;		/* derived from GtkVBox */

  SwamiguiSampleEditorStatus status; /* current status */
  IpatchList *selection;	/* item selection */
  SwamiguiSampleEditorHandler handler; /* active handler or NULL */
  gpointer handler_data;	/* handler defined pointer */

  int marker_bar_height;	/* height of top marker/loop meter bar */

  GList *tracks; /* info for each sample (see SwamiguiSampleEditor.c) */
  GList *markers;   /* list of markers (see SwamiguiSampleEditor.c) */

  guint sample_size;	    /* cached sample data length, in frames */

  SwamiControl *loop_start_hub;	/* SwamiControl hub for loop_view start props */
  SwamiControl *loop_end_hub;	/* SwamiControl hub for loop_view end props */

  gboolean marker_cursor;	/* TRUE if mouse cursor set for marker move */
  int sel_marker;		/* index of select marker or -1 */
  int sel_marker_edge;		/* selected edge -1 = start, 0 = both, 1 = end */
  int move_range_ofs;		/* if sel_marker_edge == 0, sample ofs of move */

  int sel_state;	/* see SelState in .c */
  int sel_temp;		/* tmp pos in samples when sel_state = SEL_MAYBE */

  SwamiguiCanvasMod *sample_mod;	/* zoom/scroll sample canvas modulator */
  double scroll_acc;	/* scroll accumulator (since scrolling is integer only) */

  SwamiguiCanvasMod *loop_mod;	/* zoom/scroll loop view canvas modulator */
  double loop_zoom;		/* remember last loop zoom between sample loads */

  gboolean zoom_all;	/* TRUE if entire sample zoomed (remains on resize) */

  GtkWidget *mainvbox;		/* vbox for toolbar and sample canvases */
  GtkWidget *loop_finder_pane;	/* pane for loop finder and mainvbox */
  SwamiguiLoopFinder *loop_finder_gui;	/* loop finder GUI widget */
  gboolean loop_finder_active;	/* TRUE if loop finder is currently shown */

  GnomeCanvas *sample_canvas;	/* sample canvas */
  GnomeCanvas *loop_canvas;	/* sample loop canvas */
  GnomeCanvasItem *sample_border_line; /* sample marker bar horizontal line */
  GnomeCanvasItem *loop_border_line;	  /* loop marker bar horizontal line */
  GnomeCanvasItem *xsnap_line;	/* X zoom/scroll snap line (vertical) */
  GnomeCanvasItem *ysnap_line;	/* Y zoom/scroll snap line (horizontal) */
  GnomeCanvasItem *loop_line;	/* loop view center line */
  GnomeCanvasItem *loop_snap_line;	/* loop zoom snap line */

  GtkWidget *loopsel;		/* loop selector GtkComboBox */
  SwamiControl *loopsel_ctrl;	/* control for loop selector */
  GtkListStore *loopsel_store;	/* loop selector GtkListStore model */

  GtkWidget *spinbtn_start;	/* start loop spin button */
  GtkWidget *spinbtn_end;	/* end loop spin button */
  SwamiControl *spinbtn_start_ctrl;	/* control for loop start spin button */
  SwamiControl *spinbtn_end_ctrl;	/* control for loop end spin button */

  GtkWidget *hscrollbar;	/* horizontal scrollbar widget */
  GtkWidget *toolbar;		/* sample editor toolbar */
  GtkWidget *cut_button;	/* sample cut button */
  GtkWidget *crop_button;	/* sample crop button */
  GtkWidget *copy_new_button;	/* sample copy to new button */
  GtkWidget *finder_button;	/* loop finder activation button */
  GtkWidget *samplesel_button;	/* sample selector button */

  GtkWidget *dicer_button;	/* sample dicer toggle button */
  GtkWidget *new_button;	/* create new sample from selection button */
  GtkWidget *new_name;		/* new sample name entry */

  guint center_line_color;	/* horizontal center line color */
  guint marker_border_color;	/* color of marker bar horizontal border lines */
  guint snap_line_color;	/* zoom/scroll snap line color */
  guint loop_line_color;	/* loop view center line color */
};

/* Sample editor object class */
struct _SwamiguiSampleEditorClass
{
  GtkHBoxClass parent_class;
};

/* Flags for swamigui_sample_editor_add_marker() */
typedef enum
{
  SWAMIGUI_SAMPLE_EDITOR_MARKER_SINGLE	= 1 << 0,  /* Single value (not range) */
  SWAMIGUI_SAMPLE_EDITOR_MARKER_VIEW	= 1 << 1,  /* view only marker */
  SWAMIGUI_SAMPLE_EDITOR_MARKER_SIZE	= 1 << 2   /* a start/size marker */
} SwamiguiSampleEditorMarkerFlags;

/* Builtin markers (always present, although perhaps hidden) */
typedef enum
{
  SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_SELECTION,		/* selection marker */
  SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_START,	/* loop find start window */
  SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_END	/* loop find end window */
} SwamiguiSampleEditorMarkerId;

GType swamigui_sample_editor_get_type (void);
GtkWidget *swamigui_sample_editor_new (void);

void swamigui_sample_editor_zoom_ofs (SwamiguiSampleEditor *editor,
				      double zoom_amt, double zoom_xpos);
void swamigui_sample_editor_scroll_ofs (SwamiguiSampleEditor *editor,
				        int sample_ofs);
void swamigui_sample_editor_loop_zoom (SwamiguiSampleEditor *editor,
				       double zoom_amt);

void swamigui_sample_editor_set_selection (SwamiguiSampleEditor *editor,
				      IpatchList *items);
IpatchList *swamigui_sample_editor_get_selection (SwamiguiSampleEditor *editor);

void swamigui_sample_editor_register_handler
  (SwamiguiSampleEditorHandler handler, SwamiguiPanelCheckFunc check_func);
void swamigui_sample_editor_unregister_handler
  (SwamiguiSampleEditorHandler handler);

void swamigui_sample_editor_reset (SwamiguiSampleEditor *editor);
void swamigui_sample_editor_get_loop_controls (SwamiguiSampleEditor *editor,
					       SwamiControl **loop_start,
					       SwamiControl **loop_end);

int swamigui_sample_editor_add_track (SwamiguiSampleEditor *editor,
				      IpatchSampleData *sample,
				      gboolean right_chan);
gboolean
swamigui_sample_editor_get_track_info (SwamiguiSampleEditor *editor,
				       guint track,
				       IpatchSampleData **sample,
				       SwamiguiSampleCanvas **sample_view,
				       SwamiguiSampleCanvas **loop_view);
void swamigui_sample_editor_remove_track (SwamiguiSampleEditor *editor,
					  guint track);
void swamigui_sample_editor_remove_all_tracks (SwamiguiSampleEditor *editor);

guint swamigui_sample_editor_add_marker (SwamiguiSampleEditor *editor,
					 guint flags, SwamiControl **start,
					 SwamiControl **end);
gboolean
swamigui_sample_editor_get_marker_info (SwamiguiSampleEditor *editor,
					guint marker, guint *flags,
					GnomeCanvasItem **start_line,
					GnomeCanvasItem **end_line,
					SwamiControl **start_ctrl,
					SwamiControl **end_ctrl);
void swamigui_sample_editor_set_marker (SwamiguiSampleEditor *editor,
					guint marker, guint start, guint end);
void swamigui_sample_editor_remove_marker (SwamiguiSampleEditor *editor,
					   guint marker);
void swamigui_sample_editor_remove_all_markers (SwamiguiSampleEditor *editor);
void swamigui_sample_editor_show_marker (SwamiguiSampleEditor *editor,
					 guint marker, gboolean show_marker);
void swamigui_sample_editor_set_loop_types (SwamiguiSampleEditor *editor,
					    int *types, gboolean loop_play_btn);
void swamigui_sample_editor_set_active_loop_type (SwamiguiSampleEditor *editor,
						  int type);

#endif
