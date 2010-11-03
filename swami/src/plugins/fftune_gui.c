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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>
#include <swamigui/swamigui.h>
#include <libswami/swami_priv.h> /* log_if_fail() */

#include "fftune_gui.h"

/* the range of time for the mouse wheel scroll zoom speed function, times
   are in milliseconds and represent the time between events */
#define WHEEL_ZOOM_MIN_TIME 10	/* fastest event time interval */
#define WHEEL_ZOOM_MAX_TIME 500	/* slowest event time interval */

/* zoom in speed range defined for the time interval above */
#define WHEEL_ZOOM_MIN_SPEED 0.98 /* slowest zoom in speed */
#define WHEEL_ZOOM_MAX_SPEED 0.7 /* fastest zoom in speed */
#define WHEEL_ZOOM_RANGE (WHEEL_ZOOM_MIN_SPEED - WHEEL_ZOOM_MAX_SPEED)

/* min zoom value (indexes/pixel) */
#define SPECTRUM_CANVAS_MIN_ZOOM  0.02

#define SNAP_TIMEOUT_PRIORITY  (G_PRIORITY_HIGH_IDLE + 40)

#define SNAP_TIMEOUT_PIXEL_RANGE 60 /* range in pixels of timeout varience */
#define SNAP_TIMEOUT_MIN 40	/* minimum timeout in milliseconds */
#define SNAP_TIMEOUT_MAX 120	/* maximum timeout in milliseconds */

#define SNAP_SCROLL_MIN  1	/* minimum scroll amount in pixels */
#define SNAP_SCROLL_MULT 6.0	/* scroll multiplier in pixels */

/* range of zoom speeds over SNAP_TIMEOUT_PIXEL_RANGE */
#define SNAP_ZOOM_MIN 0.99 /* minimum (slowest) zoom in amount (< 1.0) */
#define SNAP_ZOOM_MAX 0.26   /* max (fastest) zoom in amount (< 1.0) */


enum
{
  PROP_0,
  PROP_ITEM_SELECTION
};

/* columns for fftunegui->freq_store */
enum
{
  COL_POWER,	/* power ratio column */
  COL_FREQ,	/* frequency column */
  COL_NOTE,	/* MIDI note column */
  COL_CENTS,	/* fractional tuning (cents) */
  COL_COUNT	/* count of columns */
};

static gboolean plugin_fftune_gui_init (SwamiPlugin *plugin, GError **err);
static void fftune_gui_panel_iface_init (SwamiguiPanelIface *panel_iface);
static gboolean fftune_gui_panel_iface_check_selection (IpatchList *selection,
							GType *selection_types);
static void fftune_gui_set_property (GObject *object,
				     guint property_id,
				     const GValue *value,
				     GParamSpec *pspec);
static void fftune_gui_get_property (GObject *object,
				     guint property_id,
				     GValue *value,
				     GParamSpec *pspec);
static void fftune_gui_finalize (GObject *object);
static void fftunegui_cb_revert_clicked (GtkButton *button, gpointer user_data);
static void fftunegui_cb_freq_list_sel_changed (GtkTreeSelection *selection,
                                                gpointer user_data);
static void fftune_gui_cb_spectrum_change (FFTuneSpectra *spectra, guint size,
					   double *spectrum, gpointer user_data);
static void fftune_gui_cb_tunings_change (FFTuneSpectra *spectra, guint count,
					  gpointer user_data);
static void fftune_gui_cb_mode_menu_changed (GtkComboBox *combo,
					     gpointer user_data);
static void fftune_gui_cb_canvas_size_allocate (GtkWidget *widget,
						GtkAllocation *allocation,
						gpointer user_data);
static void fftune_gui_cb_scroll (GtkAdjustment *adj, gpointer user_data);
static gboolean fftune_gui_cb_spectrum_canvas_event (GnomeCanvas *canvas,
						     GdkEvent *event,
						     gpointer data);
static gboolean fftune_gui_snap_timeout (gpointer data);
static void fftune_gui_cb_ampl_zoom_value_changed (GtkAdjustment *adj,
						   gpointer user_data);

static void fftune_gui_zoom_ofs (FFTuneGui *fftunegui, double zoom_amt,
				 int zoom_xpos);
static void fftune_gui_scroll_ofs (FFTuneGui *fftunegui, int index_ofs);


/* set plugin information */
SWAMI_PLUGIN_INFO (plugin_fftune_gui_init, NULL);

/* define FFTuneGui type */
G_DEFINE_TYPE_WITH_CODE (FFTuneGui, fftune_gui, GTK_TYPE_VBOX,
		    G_IMPLEMENT_INTERFACE (SWAMIGUI_TYPE_PANEL,
					   fftune_gui_panel_iface_init));


static gboolean
plugin_fftune_gui_init (SwamiPlugin *plugin, GError **err)
{
  /* bind the gettext domain */
#if defined(ENABLE_NLS)
  bindtextdomain ("SwamiPlugin-fftune_gui", LOCALEDIR);
#endif

  g_object_set (plugin,
		"name", "FFTuneGui",
		"version", "1.0",
		"author", "Josh Green",
		"copyright", "Copyright (C) 2005",
		"descr", N_("GUI for Fast Fourier Transform sample tuner"),
		"license", "GPL",
		NULL);

  /* register types and register it as a panel interface */
  swamigui_register_panel_selector_type (fftune_gui_get_type (), 200);

  return (TRUE);
}

static void
fftune_gui_class_init (FFTuneGuiClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->set_property = fftune_gui_set_property;
  obj_class->get_property = fftune_gui_get_property;
  obj_class->finalize = fftune_gui_finalize;

  g_object_class_override_property (obj_class, PROP_ITEM_SELECTION, "item-selection");
}

static void
fftune_gui_panel_iface_init (SwamiguiPanelIface *panel_iface)
{
  panel_iface->label = _("FFTune");
  panel_iface->blurb = _("Semi-automated tuning plugin");
  panel_iface->stockid = SWAMIGUI_STOCK_TUNING;
  panel_iface->check_selection = fftune_gui_panel_iface_check_selection;
}

static gboolean
fftune_gui_panel_iface_check_selection (IpatchList *selection,
					GType *selection_types)
{ /* a single item with sample interface is valid */
  return (!selection->items->next
	  && g_type_is_a (*selection_types, IPATCH_TYPE_SAMPLE));
}

static void
fftune_gui_set_property (GObject *object, guint property_id,
			 const GValue *value, GParamSpec *pspec)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (object);
  SwamiControl *samctrl;
  IpatchSample *sample = NULL;
  IpatchSampleData *sampledata = NULL;
  IpatchList *list;
  int root_note, fine_tune;

  switch (property_id)
    {
    case PROP_ITEM_SELECTION:
      list = g_value_get_object (value);

      /* only set sample if a single IpatchSample item or NULL list */
      if (list && list->items && !list->items->next
	  && IPATCH_IS_SAMPLE (list->items->data))
      {
	sample = IPATCH_SAMPLE (list->items->data);
        g_object_get (sample, "sample-data", &sampledata, NULL);
      }

      /* disconnect GUI controls (if connected) */
      swami_control_disconnect_all (fftunegui->root_note_ctrl);
      swami_control_disconnect_all (fftunegui->fine_tune_ctrl);

      /* connect controls to sample properties */

      if (sampledata)
	{
	  g_object_get (sample,
	                "root-note", &root_note,
	                "fine-tune", &fine_tune,
	                NULL);
	  fftunegui->orig_root_note = root_note;
	  fftunegui->orig_fine_tune = fine_tune;

	  samctrl = swami_get_control_prop_by_name (G_OBJECT (sample), "root-note");
	  swami_control_connect (samctrl, fftunegui->root_note_ctrl,
				 SWAMI_CONTROL_CONN_BIDIR
				 | SWAMI_CONTROL_CONN_INIT);
    
	  samctrl = swami_get_control_prop_by_name (G_OBJECT (sample), "fine-tune");
	  swami_control_connect (samctrl, fftunegui->fine_tune_ctrl,
				 SWAMI_CONTROL_CONN_BIDIR
				 | SWAMI_CONTROL_CONN_INIT);
	}

      /* recalculate full zoom */
      fftunegui->recalc_zoom = TRUE;

      /* de-activate spectra object before setting sample */
      g_object_set (fftunegui->spectra, "active", FALSE, NULL);

      /* reset amplitude zoom */
      gtk_range_set_value (GTK_RANGE (fftunegui->vscale), 1.0);

      g_object_set (fftunegui->spectra, "sample", sample, NULL);

      /* re-activate spectra if sample is set */
      if (sample) g_object_set (fftunegui->spectra, "active", TRUE, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
fftune_gui_get_property (GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (object);
  IpatchSampleData *sample = NULL;
  IpatchList *list;

  switch (property_id)
    {
    case PROP_ITEM_SELECTION:
      list = ipatch_list_new ();
      g_object_get (fftunegui->spectra, "sample", &sample, NULL);
      if (sample) list->items = g_list_append (list->items, sample);
      g_value_set_object (value, list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
fftune_gui_finalize (GObject *object)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (object);

  g_object_unref (fftunegui->spectra);

  swami_control_disconnect_unref (fftunegui->root_note_ctrl);
  swami_control_disconnect_unref (fftunegui->fine_tune_ctrl);

  if (G_OBJECT_CLASS (fftune_gui_parent_class)->finalize)
    G_OBJECT_CLASS (fftune_gui_parent_class)->finalize (object);
}

static void
fftune_gui_init (FFTuneGui *fftunegui)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *scrollwin;
  GtkWidget *box, *vbox, *hbox;
  GtkWidget *lbl;
  GtkWidget *frame;
  GtkStyle *style;
  GtkAdjustment *adj;
  GtkWidget *widg;

  fftunegui->snap_active = FALSE;
  fftunegui->snap_timeout_handler = 0;
  fftunegui->scroll_active = FALSE;
  fftunegui->zoom_active = FALSE;
  fftunegui->snap_line_color = 0xFF0000FF;

  /* create spectrum tuning object */
  fftunegui->spectra = g_object_new (g_type_from_name ("FFTuneSpectra"), NULL);

  /* connect to spectrum change signal */
  g_signal_connect (fftunegui->spectra, "spectrum-change",
		    G_CALLBACK (fftune_gui_cb_spectrum_change), fftunegui);

  /* connect to tunings change signal */
  g_signal_connect (fftunegui->spectra, "tunings-change",
		    G_CALLBACK (fftune_gui_cb_tunings_change), fftunegui);

  /* horizontal box to pack sample data selector, etc */
  box = gtk_hbox_new (FALSE, 4);
  gtk_widget_show (box);

  lbl = gtk_label_new (_("Sample data"));
  gtk_widget_show (lbl);
  gtk_box_pack_start (GTK_BOX (box), lbl, FALSE, FALSE, 0);

  fftunegui->mode_menu = gtk_combo_box_new_text ();
  gtk_widget_show (fftunegui->mode_menu);
  gtk_box_pack_start (GTK_BOX (box), fftunegui->mode_menu, FALSE, FALSE, 0);

  gtk_combo_box_append_text (GTK_COMBO_BOX (fftunegui->mode_menu), _("All"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (fftunegui->mode_menu), _("Loop"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (fftunegui->mode_menu), 0);

  g_signal_connect (fftunegui->mode_menu, "changed",
		    (GCallback)fftune_gui_cb_mode_menu_changed, fftunegui);

  lbl = gtk_label_new (_("Root note"));
  gtk_widget_show (lbl);
  gtk_box_pack_start (GTK_BOX (box), lbl, FALSE, FALSE, 0);

  fftunegui->root_notesel = swamigui_note_selector_new ();
  gtk_widget_show (fftunegui->root_notesel);
  gtk_box_pack_start (GTK_BOX (box), fftunegui->root_notesel, FALSE, FALSE, 0);

  /* create root note control and add to GUI queue */
  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (fftunegui->root_notesel));
  fftunegui->root_note_ctrl = SWAMI_CONTROL (swamigui_control_adj_new (adj));

  lbl = gtk_label_new (_("Fine tune"));
  gtk_widget_show (lbl);
  gtk_box_pack_start (GTK_BOX (box), lbl, FALSE, FALSE, 0);

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, -99.0, 99.0, 1.0, 5.0, 0.0));
  fftunegui->fine_tune = gtk_spin_button_new (adj, 1.0, 0);
  gtk_widget_show (fftunegui->fine_tune);
  gtk_box_pack_start (GTK_BOX (box), fftunegui->fine_tune, FALSE, FALSE, 0);

  /* create fine tune control */
  fftunegui->fine_tune_ctrl = SWAMI_CONTROL (swamigui_control_adj_new (adj));

  widg = gtk_vseparator_new ();
  gtk_widget_show (widg);
  gtk_box_pack_start (GTK_BOX (box), widg, FALSE, FALSE, 0);

  fftunegui->revert_button = gtk_button_new_with_mnemonic (_("_Revert"));
  gtk_widget_set_tooltip_text (fftunegui->revert_button, _("Revert to original tuning values"));
  gtk_widget_show (fftunegui->revert_button);
  gtk_box_pack_start (GTK_BOX (box), fftunegui->revert_button, FALSE, FALSE, 0);
  g_signal_connect (fftunegui->revert_button, "clicked",
                    G_CALLBACK (fftunegui_cb_revert_clicked), fftunegui);

  /* vbox to set vertical spacing of upper outtie frame */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 2);

  /* upper outtie frame, with spectrum data selector, etc */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (fftunegui), frame, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* lower inset frame for spectrum canvas */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (fftunegui), frame, TRUE, TRUE, 0);

  /* attach a horizontal scrollbar to the spectrum view */
  fftunegui->hscrollbar = gtk_hscrollbar_new (NULL);
  gtk_widget_show (fftunegui->hscrollbar);
  gtk_box_pack_start (GTK_BOX (fftunegui), fftunegui->hscrollbar, FALSE, FALSE, 0);

  g_signal_connect_after (gtk_range_get_adjustment
			  (GTK_RANGE (fftunegui->hscrollbar)), "value-changed",
			  G_CALLBACK (fftune_gui_cb_scroll),
			  fftunegui);

  /* hbox for frequency list and desired root note selector */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  /* create frequency suggestion store */
  fftunegui->freq_store
    = gtk_list_store_new (COL_COUNT, G_TYPE_STRING, G_TYPE_STRING,
			  G_TYPE_STRING, G_TYPE_STRING);

  /* scroll window for frequency suggestion list */
  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrollwin);
  gtk_box_pack_start (GTK_BOX (hbox), scrollwin, FALSE, FALSE, 0);

  /* create frequency suggestion list */
  fftunegui->freq_list
    = gtk_tree_view_new_with_model (GTK_TREE_MODEL (fftunegui->freq_store));
  /* Disable search, since it breaks piano key playback */
  g_object_set (fftunegui->freq_list, "enable-search", FALSE, NULL);
  gtk_widget_show (fftunegui->freq_list);
  gtk_container_add (GTK_CONTAINER (scrollwin), GTK_WIDGET (fftunegui->freq_list));

  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (fftunegui->freq_list)),
                    "changed", G_CALLBACK (fftunegui_cb_freq_list_sel_changed), fftunegui);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_("Power"), renderer,
     "text", COL_POWER,
     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (fftunegui->freq_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_("Frequency"), renderer,
     "text", COL_FREQ,
     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (fftunegui->freq_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_("Note"), renderer,
     "text", COL_NOTE,
     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (fftunegui->freq_list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_("Cents"), renderer,
     "text", COL_CENTS,
     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (fftunegui->freq_list), column);


  /* create canvas */
  fftunegui->canvas = GNOME_CANVAS (gnome_canvas_new ());
  gnome_canvas_set_center_scroll_region (GNOME_CANVAS (fftunegui->canvas), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (fftunegui->canvas),
		      TRUE, TRUE, 0);
  g_signal_connect (fftunegui->canvas, "event",
		    G_CALLBACK (fftune_gui_cb_spectrum_canvas_event),
		    fftunegui);
  g_signal_connect (fftunegui->canvas, "size-allocate",
		    G_CALLBACK (fftune_gui_cb_canvas_size_allocate),
		    fftunegui);

  /* change background color of canvas to black */
  style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (fftunegui->canvas)));
  style->bg[GTK_STATE_NORMAL] = style->black;
  gtk_widget_set_style (GTK_WIDGET (fftunegui->canvas), style);
  gtk_widget_show (GTK_WIDGET (fftunegui->canvas));

  /* create spectrum canvas item */
  fftunegui->spectrum
    = gnome_canvas_item_new (gnome_canvas_root (fftunegui->canvas),
			     SWAMIGUI_TYPE_SPECTRUM_CANVAS,
			     "adjustment", gtk_range_get_adjustment
			     (GTK_RANGE (fftunegui->hscrollbar)),
			     NULL);
  /* create snap line */
  fftunegui->snap_line
    = gnome_canvas_item_new (gnome_canvas_root (fftunegui->canvas),
			     GNOME_TYPE_CANVAS_LINE,
     			     "fill-color-rgba", fftunegui->snap_line_color,
			     "width-pixels", 2,
			     NULL);
  gnome_canvas_item_hide (fftunegui->snap_line);

  /* vertical scale for setting amplitude zoom */
  fftunegui->vscale = gtk_vscale_new_with_range (1.0, 100.0, 0.5);
  gtk_scale_set_draw_value (GTK_SCALE (fftunegui->vscale), FALSE);
  gtk_range_set_inverted (GTK_RANGE (fftunegui->vscale), TRUE);
  adj = gtk_range_get_adjustment (GTK_RANGE (fftunegui->vscale));
  g_signal_connect (adj, "value-changed",
	G_CALLBACK (fftune_gui_cb_ampl_zoom_value_changed), fftunegui);
  gtk_widget_show (fftunegui->vscale);
  gtk_box_pack_start (GTK_BOX (hbox), fftunegui->vscale, FALSE, FALSE, 0);
}

static void
fftunegui_cb_revert_clicked (GtkButton *button, gpointer user_data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (user_data);
  IpatchSample *sample;

  g_object_get (fftunegui->spectra, "sample", &sample, NULL);	/* ++ ref */
  if (!sample) return;

  g_object_set (sample,
                "root-note", fftunegui->orig_root_note,
                "fine-tune", fftunegui->orig_fine_tune,
                NULL);

  g_object_unref (sample);	/* -- unref */
}

static void
fftunegui_cb_freq_list_sel_changed (GtkTreeSelection *selection, gpointer user_data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (user_data);
  IpatchSample *sample;
  GtkTreeIter iter;
  char *notestr, *centstr;
  int note, finetune;

  if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) return;

  g_object_get (fftunegui->spectra, "sample", &sample, NULL);	/* ++ ref */
  if (!sample) return;

  gtk_tree_model_get (GTK_TREE_MODEL (fftunegui->freq_store), &iter,
                      COL_NOTE, &notestr,	/* ++ alloc */
                      COL_CENTS, &centstr,	/* ++ alloc */
                      -1);

  note = strtol (notestr, NULL, 10);
  finetune = -roundf (strtof (centstr, NULL));	/* Invert cents to get finetune adjustment */

  g_object_set (sample,
                "root-note", note,
                "fine-tune", finetune,
                NULL);

  g_object_unref (sample);	/* -- unref */
}

static void
fftune_gui_cb_spectrum_change (FFTuneSpectra *spectra, guint size,
			       double *spectrum, gpointer user_data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (user_data);
  int width, height;
  double zoom;

  swamigui_spectrum_canvas_set_data
    (SWAMIGUI_SPECTRUM_CANVAS (fftunegui->spectrum), spectrum, size, NULL);

  if (fftunegui->recalc_zoom)
    {
      g_object_get (fftunegui->spectrum,
		    "width", &width,
		    "height", &height,
		    NULL);

      if (width == 0.0) zoom = 0.0;
      else zoom = size / (double)width;

      g_object_set (fftunegui->spectrum, "zoom", zoom, NULL);

      fftunegui->recalc_zoom = FALSE;
    }
}

static void
fftune_gui_cb_tunings_change (FFTuneSpectra *spectra, guint count,
			      gpointer user_data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (user_data);
  GtkTreeIter iter;
  double power, max_power, freq, cents;
  char powerstr[6], freqstr[32], notestr[11], centsstr[16];
  int i, note;

  gtk_list_store_clear (fftunegui->freq_store);

  for (i = 0; i < count; i++)
    {
      /* select the current tuning index */
      g_object_set (spectra, "tune-select", i, NULL);

      /* get frequency and power of the current tuning suggestion */
      g_object_get (spectra,
		    "tune-freq", &freq,
		    "tune-power", &power,
		    NULL);

      if (i == 0) max_power = power;	/* first tuning is max power */

      cents = ipatch_unit_hertz_to_cents (freq);
      note = cents / 100.0 + 0.5;
      cents -= note * 100;

      sprintf (powerstr, "%0.2f", power / max_power);
      sprintf (freqstr, "%0.2f", freq);
      sprintf (centsstr, "%0.2f", cents);

      if (note < 0) strcpy (notestr, "<0");
      else if (note > 127) strcpy (notestr, ">127");
      else
      {
	sprintf (notestr, "%d | ", note);
	swami_util_midi_note_to_str (note, notestr + strlen (notestr));
      }

      gtk_list_store_append (fftunegui->freq_store, &iter);
      gtk_list_store_set (fftunegui->freq_store, &iter,
			  COL_POWER, powerstr,
			  COL_FREQ, freqstr,
			  COL_NOTE, notestr,
			  COL_CENTS, centsstr,
			  -1);
    }
}

/* callback when sample mode combo box changes */
static void
fftune_gui_cb_mode_menu_changed (GtkComboBox *combo, gpointer user_data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (user_data);
  int active;

  active = gtk_combo_box_get_active (combo);
  if (active != -1)
    g_object_set (fftunegui->spectra, "sample-mode", active, NULL);
}

static void
fftune_gui_cb_canvas_size_allocate (GtkWidget *widget,
				    GtkAllocation *allocation,
				    gpointer user_data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (user_data);
  GnomeCanvas *canvas = GNOME_CANVAS (widget);
  int width, height;

  width = GTK_WIDGET (canvas)->allocation.width;
  height = GTK_WIDGET (canvas)->allocation.height;

  /* update size of spectrum canvas item */
  g_object_set (fftunegui->spectrum,
		"width", width,
		"height", height,
		NULL);
}

/* horizontal scroll bar value changed callback */
static void
fftune_gui_cb_scroll (GtkAdjustment *adj, gpointer user_data)
{
//  FFTuneGui *fftunegui = FFTUNE_GUI (user_data);
//  marker_control_update_all (fftunegui); /* update all markers */
}

static gboolean
fftune_gui_cb_spectrum_canvas_event (GnomeCanvas *canvas,
				     GdkEvent *event, gpointer data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (data);
  GnomeCanvasPoints *points;
//  MarkerInfo *marker_info;
  GdkEventScroll *scroll_event;
  GdkEventButton *btn_event;
  GdkEventMotion *motion_event;
  double scale, zoom;
  int height;
  int ofs, val;
  guint32 timed = G_MAXUINT;

  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
      motion_event = (GdkEventMotion *)event;

#if 0
      if (fftunegui->sel_marker != -1)
	{
	  marker_info = g_list_nth_data (fftunegui->markers, fftunegui->sel_marker);
	  if (!marker_info) break;
	  val = swamigui_spectrum_canvas_pos_to_sample
	    (SWAMIGUI_SPECTRUM_CANVAS (track_info->sample_view), motion_event->x);

	  g_value_init (&value, G_TYPE_UINT);
	  g_value_set_uint (&value, val);
	  swami_control_transmit_value (marker_info->ctrl, &value);
	  g_value_unset (&value);

	  /* update the marker */
	  marker_info->sample_pos = val;
	  marker_control_update (marker_info);
	  break;
	}
      else  /* no active marker selection, see if mouse cursor over marker */
	{
	  /* see if mouse is over a marker */
	  marker_info = fftune_gui_xpos_is_marker (fftunegui,
							   motion_event->x);
	  /* see if cursor should be changed */
	  if ((marker_info != NULL) != fftunegui->marker_cursor)
	    {
	      if (marker_info)
		cursor = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW); /* ++ ref */
	      else cursor = gdk_cursor_new (GDK_LEFT_PTR);

	      gdk_window_set_cursor (GTK_WIDGET (fftunegui->canvas)->window,
				     cursor);
	      gdk_cursor_unref (cursor);
	      fftunegui->marker_cursor = marker_info != NULL;
	    }
	}
#endif

      if (!fftunegui->snap_active) break;

      ofs = motion_event->x - fftunegui->snap_pos;
      val = ABS (ofs);
      if (val > SNAP_TIMEOUT_PIXEL_RANGE) val = SNAP_TIMEOUT_PIXEL_RANGE;

      if (ofs != 0)
	fftunegui->snap_interval = (SNAP_TIMEOUT_PIXEL_RANGE - val)
	  * (SNAP_TIMEOUT_MAX - SNAP_TIMEOUT_MIN)
	  / (SNAP_TIMEOUT_PIXEL_RANGE - 1) + SNAP_TIMEOUT_MIN;
      else fftunegui->snap_interval = 0;

      /* add a timeout callback for zoom/scroll if not already added */
      if (!fftunegui->snap_timeout_handler && fftunegui->snap_interval > 0)
	{
	  fftunegui->snap_timeout_handler
	    = g_timeout_add_full (SNAP_TIMEOUT_PRIORITY,
				  fftunegui->snap_interval,
				  fftune_gui_snap_timeout,
				  fftunegui, NULL);
	}

      if (motion_event->state & GDK_SHIFT_MASK)
	{
	  fftunegui->scroll_active = TRUE;
	  g_object_get (fftunegui->spectrum, "zoom", &zoom, NULL);

	  if (ofs >= 0) val = SNAP_SCROLL_MIN;
	  else val = -SNAP_SCROLL_MIN;

	  fftunegui->scroll_amt = zoom * (ofs * SNAP_SCROLL_MULT + val);
	}
      else fftunegui->scroll_active = FALSE;

      if (motion_event->state & GDK_CONTROL_MASK)
	{
	  fftunegui->zoom_active = TRUE;

	  if (ofs != 0)
	    {
	      fftunegui->zoom_amt = val * (SNAP_ZOOM_MAX - SNAP_ZOOM_MIN)
		/ (SNAP_TIMEOUT_PIXEL_RANGE - 1) + SNAP_ZOOM_MIN;
	      if (ofs < 0) fftunegui->zoom_amt = 1.0 / fftunegui->zoom_amt;
	    }
	  else fftunegui->zoom_amt = 1.0;
	}
      else fftunegui->zoom_active = FALSE;
      break;
    case GDK_BUTTON_PRESS:
      btn_event = (GdkEventButton *)event;

      /* make sure its button 1 */
      if (btn_event->button != 1) break;

#if 0
      /* is it a marker click? */
      marker_info = fftune_gui_xpos_is_marker (fftunegui, btn_event->x);
      if (marker_info)
	{
	  fftunegui->sel_marker = g_list_index (fftunegui->markers, marker_info);
	  break;
	}
#endif

      if (!(btn_event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)))
	break;

      fftunegui->snap_active = TRUE;

      fftunegui->snap_pos = btn_event->x;
      height = GTK_WIDGET (fftunegui->canvas)->allocation.height;

      points = gnome_canvas_points_new (2);
      points->coords[0] = fftunegui->snap_pos;
      points->coords[1] = 0;
      points->coords[2] = fftunegui->snap_pos;
      points->coords[3] = height - 1;

      g_object_set (fftunegui->snap_line, "points", points, NULL);
      gnome_canvas_item_show (fftunegui->snap_line);

      gnome_canvas_points_free (points);
      break;
    case GDK_BUTTON_RELEASE:
      if (!fftunegui->snap_active) break;
//      if (!fftunegui->snap_active && fftunegui->sel_marker == -1) break;

      btn_event = (GdkEventButton *)event;
      if (btn_event->button != 1) break;

//      fftunegui->sel_marker = -1;
      fftunegui->snap_active = FALSE;

      if (fftunegui->snap_timeout_handler != 0)
	{
	  g_source_remove (fftunegui->snap_timeout_handler);
	  fftunegui->snap_timeout_handler = 0;
	}

      fftunegui->scroll_active = FALSE;
      fftunegui->zoom_active = FALSE;

      gnome_canvas_item_hide (fftunegui->snap_line);
      break;
    case GDK_SCROLL:		/* mouse wheel zooming */
      scroll_event = (GdkEventScroll *)event;
      if (scroll_event->direction != GDK_SCROLL_UP
	  && scroll_event->direction != GDK_SCROLL_DOWN)
	break;

      if (scroll_event->direction == fftunegui->last_wheel_dir)
	timed = scroll_event->time - fftunegui->last_wheel_time;

      timed = CLAMP (timed, WHEEL_ZOOM_MIN_TIME, WHEEL_ZOOM_MAX_TIME);
      timed -= WHEEL_ZOOM_MIN_TIME;

      scale = WHEEL_ZOOM_MAX_SPEED
	+ (double)timed / WHEEL_ZOOM_MAX_TIME * WHEEL_ZOOM_RANGE;

      if (scroll_event->direction == GDK_SCROLL_DOWN)
	scale = 1 / scale;

      fftune_gui_zoom_ofs (fftunegui, scale, scroll_event->x);

      fftunegui->last_wheel_dir = scroll_event->direction;
      fftunegui->last_wheel_time = scroll_event->time;
      break;
    default:
      break;
    }

  return (FALSE);
}

static gboolean
fftune_gui_snap_timeout (gpointer data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (data);

  if (fftunegui->scroll_active && fftunegui->scroll_amt != 0)
    fftune_gui_scroll_ofs (fftunegui, fftunegui->scroll_amt);

  if (fftunegui->zoom_active && fftunegui->zoom_amt != 1.0)
    fftune_gui_zoom_ofs (fftunegui, fftunegui->zoom_amt,
				    fftunegui->snap_pos);

  /* add timeout for next interval */
  if (fftunegui->snap_interval > 0)
    {
      fftunegui->snap_timeout_handler
	= g_timeout_add_full (SNAP_TIMEOUT_PRIORITY,
			      fftunegui->snap_interval,
			      fftune_gui_snap_timeout,
			      fftunegui, NULL);
    }
  else fftunegui->snap_timeout_handler = 0;

  return (FALSE);		/* remove this timeout */
}

static void
fftune_gui_cb_ampl_zoom_value_changed (GtkAdjustment *adj, gpointer user_data)
{
  FFTuneGui *fftunegui = FFTUNE_GUI (user_data);

  /* set vertical zoom */
  g_object_set (fftunegui->spectrum, "zoom-ampl", adj->value, NULL);
}

#if 0
/* check if a given xpos is on a marker */
static MarkerInfo *
fftune_gui_xpos_is_marker (FFTuneGui *fftunegui, int xpos)
{
  MarkerInfo *marker_info, *match = NULL;
  SwamiguiSampleCanvas *sample_view;
  int x, ofs = -1;
  GList *p;

  if (!fftunegui->markers || !fftunegui->tracks) return (NULL);

  sample_view = SWAMIGUI_SPECTRUM_CANVAS
    (((TrackInfo *)(fftunegui->tracks->data))->sample_view);

  for (p = fftunegui->markers; p; p = g_list_next (p))
    {
      marker_info = (MarkerInfo *)(p->data);

      x = swamigui_spectrum_canvas_sample_to_pos (sample_view,
						marker_info->sample_pos);
      if (x != -1)
	{
	  x = ABS (xpos - x);
	  if (x <= MARKER_CLICK_DISTANCE && (ofs == -1 || x < ofs))
	    {
	      match = marker_info;
	      ofs = x;
	    }
	}
    }

  return (match);
}
#endif

/* Zoom the spectrum canvas the specified offset amount and modify the
 * start index position to keep the given X coordinate stationary. */
static void
fftune_gui_zoom_ofs (FFTuneGui *fftunegui, double zoom_amt, int zoom_xpos)
{
  double zoom;
  guint start;
  guint spectrum_size;
  int width, index_ofs;

  g_return_if_fail (FFTUNE_IS_GUI (fftunegui));

  /* get current values from spectrum canvas view */
  g_object_get (fftunegui->spectrum,
		"zoom", &zoom,
		"start", &start,
		"width", &width,
		NULL);

  index_ofs = zoom_xpos * zoom; /* index to zoom xpos */
  zoom *= zoom_amt;

  spectrum_size = SWAMIGUI_SPECTRUM_CANVAS (fftunegui->spectrum)->spectrum_size;

  /* do some bounds checking on the zoom value */
  if (zoom < SPECTRUM_CANVAS_MIN_ZOOM)
    zoom = SPECTRUM_CANVAS_MIN_ZOOM;
  else if (width * zoom > spectrum_size)
    { /* view exceeds spectrum data */
      start = 0;
      zoom = spectrum_size / (double)width;
    }
  else
    {
      index_ofs -= zoom_xpos * zoom; /* subtract new zoom offset */
      if (index_ofs < 0 && -index_ofs > start) start = 0;
      else start += index_ofs;

      /* make sure spectrum doesn't end in the middle of the display */
      if (start + width * zoom > spectrum_size)
	start = spectrum_size - width * zoom;
    }

  g_object_set (fftunegui->spectrum,
		"zoom", zoom,
		"start", start,
		NULL);
}

/* Scroll the spectrum canvas by a given offset. */
static void
fftune_gui_scroll_ofs (FFTuneGui *fftunegui, int index_ofs)
{
  double zoom;
  guint start_index;
  int newstart, width;
  guint spectrum_size;
  int last_index;

  g_return_if_fail (FFTUNE_IS_GUI (fftunegui));
  if (index_ofs == 0) return;

  g_object_get (fftunegui->spectrum,
		"start", &start_index,
		"zoom", &zoom,
		"width", &width,
		NULL);

  spectrum_size = SWAMIGUI_SPECTRUM_CANVAS (fftunegui->spectrum)->spectrum_size;

  last_index = spectrum_size - zoom * width;
  if (last_index < 0) return;	/* spectrum too small for current zoom? */

  newstart = (int)start_index + index_ofs;
  newstart = CLAMP (newstart, 0, last_index);

  g_object_set (fftunegui->spectrum, "start", newstart, NULL);
}
