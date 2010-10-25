/*
 * SwamiguiSampleEditor.c - Sample editor widget
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
#include <gtk/gtk.h>

#include <libinstpatch/libinstpatch.h>

#include <libswami/swami_priv.h> /* log_if_fail() */

#include "SwamiguiControl.h"
#include "SwamiguiRoot.h"
#include "SwamiguiSampleEditor.h"
#include "SwamiguiSampleCanvas.h"
#include "SwamiguiLoopFinder.h"
#include "icons.h"
#include "util.h"
#include "i18n.h"

/* number of markers that are built in (don't get removed) */
#define BUILTIN_MARKER_COUNT	3

/* value stored in editor->sel_state for current active selection state */
typedef enum
{
  SEL_INACTIVE,		/* no selection is in progress */
  SEL_MAYBE,		/* a mouse click occurred by not moved enough yet */
  SEL_START,		/* actively changing selection from start edge */
  SEL_END		/* actively changing selection from end edge */
} SelState;

typedef struct
{
  IpatchSampleData *sample;
  gboolean right_chan;	/* set to TRUE for stereo to use right channel */
  GnomeCanvasItem *sample_view;
  GnomeCanvasItem *loop_view;
  GnomeCanvasItem *sample_center_line; /* sample view horizonal center line */
  GnomeCanvasItem *loop_center_line; /* loop view horizontal center line */
} TrackInfo;

typedef struct
{
  guint flags;			/* SwamiguiSampleEditorMarkerFlags */
  gboolean visible;		/* TRUE if visible, FALSE otherwise */
  SwamiControl *start_ctrl;	/* marker start control */
  SwamiControl *end_ctrl;	/* marker end control (range only) */
  GnomeCanvasItem *start_line;	/* marker start line canvas item */
  GnomeCanvasItem *end_line;	/* marker end line canvas item (range only) */
  GnomeCanvasItem *range_box;	/* marker range box (range only) */
  guint start_pos;		/* sample start position of marker */
  guint end_pos;		/* sample end position of marker (range only) */
  SwamiguiSampleEditor *editor;	/* so we can pass MarkerInfo to callbacks */
} MarkerInfo;

/* min zoom value for sample and loop canvas (samples/pixel) */
#define SAMPLE_CANVAS_MIN_ZOOM  0.02
#define LOOP_CANVAS_MIN_ZOOM	0.02

#define LOOP_CANVAS_DEF_ZOOM	0.2	/* default zoom for loop canvas */

/* default colors */
#define DEFAULT_CENTER_LINE_COLOR	0x7F00FFFF
#define DEFAULT_MARKER_BORDER_COLOR	0xBBBBBBFF
#define DEFAULT_SNAP_LINE_COLOR		0xFF0000FF
#define DEFAULT_LOOP_LINE_COLOR		0x777777FF

/* default marker colors */
guint default_marker_colors[] = {
  GNOME_CANVAS_COLOR (0xC2, 0xFF, 0xB6),	/* selection marker */
  GNOME_CANVAS_COLOR (0xB5, 0x10, 0xFF),	/* loop finder start marker */
  GNOME_CANVAS_COLOR (0xFF, 0x10, 0x70),	/* loop finder end marker */
  GNOME_CANVAS_COLOR (0x21, 0xED, 0x3D),	/* loop (handler defined) */
  GNOME_CANVAS_COLOR (0xFF, 0xEE, 0x10),
  GNOME_CANVAS_COLOR (0xFF, 0x30, 0x10),
  GNOME_CANVAS_COLOR (0x10, 0xC4, 0xFF),
  GNOME_CANVAS_COLOR (0x10, 0xFF, 0x7B)
};

/* default height of marker bar */
#define DEFAULT_MARKER_BAR_HEIGHT	24

/* colors are in RRGGBBAA */
#define MARKER_DEFAULT_COLOR  0x00FF00FF /* interactive marker color */
#define MARKER_NONIACTV_COLOR 0x666666FF /* non-interactive marker color */

#define MARKER_DEFAULT_WIDTH  1	/* width of markers (in pixels) */
#define MARKER_CLICK_DISTANCE 3	/* click area surrounding markers (in pixels) */

enum
{
  PROP_0,
  PROP_ITEM_SELECTION,
  PROP_BORDER_WIDTH,
  PROP_MARKER_BAR_HEIGHT
};

static void
swamigui_sample_editor_class_init (SwamiguiSampleEditorClass *klass);
static void swamigui_sample_editor_panel_iface_init (SwamiguiPanelIface *panel_iface);
static gboolean
swamigui_sample_editor_panel_iface_check_selection (IpatchList *selection,
						    GType *selection_types);
static void swamigui_sample_editor_set_property (GObject *object,
						 guint property_id,
						 const GValue *value,
						 GParamSpec *pspec);
static void swamigui_sample_editor_get_property (GObject *object,
						 guint property_id,
						 GValue *value,
						 GParamSpec *pspec);
static void swamigui_sample_editor_finalize (GObject *object);
static void swamigui_sample_editor_init (SwamiguiSampleEditor *editor);
#if 0
static void swamigui_sample_editor_cb_cut_sample (GtkToggleToolButton *button,
						  gpointer user_data);
static void swamigui_sample_editor_cb_crop_sample (GtkToggleToolButton *button,
						   gpointer user_data);
static void swamigui_sample_editor_cb_copy_new_sample (GtkToggleToolButton *button,
						       gpointer user_data);
static IpatchSampleData *get_selection_sample_data (SwamiguiSampleEditor *editor);
#endif
static void swamigui_sample_editor_cb_loop_finder (GtkToggleToolButton *button,
						   gpointer user_data);
static void editor_cb_pane_size_allocate (GtkWidget *pane,
					  GtkAllocation *allocation,
					  gpointer user_data);
static void
swamigui_sample_editor_cb_canvas_size_allocate (GtkWidget *widget,
						GtkAllocation *allocation,
						gpointer user_data);
static void swamigui_sample_editor_update_sizes (SwamiguiSampleEditor *editor);
static void
swamigui_sample_editor_update_canvas_size (SwamiguiSampleEditor *editor,
					   GnomeCanvas *canvas);
static void swamigui_sample_editor_cb_scroll (GtkAdjustment *adj,
					      gpointer user_data);
static gboolean
swamigui_sample_editor_cb_sample_canvas_event (GnomeCanvas *canvas,
					       GdkEvent *event, gpointer data);
static gboolean
swamigui_sample_editor_cb_loop_canvas_event (GnomeCanvas *canvas,
					     GdkEvent *event, gpointer data);
static MarkerInfo *
pos_is_marker (SwamiguiSampleEditor *editor, int xpos, int ypos,
	       int *marker_edge, gboolean *is_onbox);
static void
swamigui_sample_editor_sample_mod_update (SwamiguiCanvasMod *mod, double xzoom,
					  double yzoom, double xscroll,
					  double yscroll, double xpos,
					  double ypos, gpointer user_data);
static void
swamigui_sample_editor_sample_mod_snap (SwamiguiCanvasMod *mod, guint actions,
					double xsnap, double ysnap,
					gpointer user_data);
static void
swamigui_sample_editor_loop_mod_update (SwamiguiCanvasMod *mod, double xzoom,
					double yzoom, double xscroll,
					double yscroll, double xpos,
					double ypos, gpointer user_data);
static void
swamigui_sample_editor_loop_mod_snap (SwamiguiCanvasMod *mod, guint actions,
				      double xsnap, double ysnap,
				      gpointer user_data);
static gboolean
swamigui_sample_editor_real_set_selection (SwamiguiSampleEditor *editor,
				       IpatchList *items);
static void
swamigui_sample_editor_deactivate_handler (SwamiguiSampleEditor *editor);
static void
swamigui_sample_editor_remove_track_item (SwamiguiSampleEditor *editor,
					  GList *p, gboolean destroy);
static void
swamigui_sample_editor_set_marker_info (SwamiguiSampleEditor *editor,
					MarkerInfo *marker_info,
					guint start, guint end);
static void
swamigui_sample_editor_remove_marker_item (SwamiguiSampleEditor *editor,
					   GList *p, gboolean destroy);

static void start_marker_control_get_value_func (SwamiControl *control,
						 GValue *value);
static void start_marker_control_set_value_func (SwamiControl *control,
						 SwamiControlEvent *event,
						 const GValue *value);
static void end_marker_control_get_value_func (SwamiControl *control,
					       GValue *value);
static void end_marker_control_set_value_func (SwamiControl *control,
					       SwamiControlEvent *event,
					       const GValue *value);
static void marker_control_update (MarkerInfo *marker_info);
static void marker_control_update_all (SwamiguiSampleEditor *editor);
static gboolean
swamigui_sample_editor_default_handler (SwamiguiSampleEditor *editor);
static gboolean
swamigui_sample_editor_default_handler_check_func (IpatchList *selection,
						   GType *selection_types);
static void swamigui_sample_editor_loopsel_ctrl_get (SwamiControl *control,
						     GValue *value);
static void swamigui_sample_editor_loopsel_ctrl_set (SwamiControl *control,
						     SwamiControlEvent *event,
						     const GValue *value);
static void swamigui_sample_editor_loopsel_cb_changed (GtkComboBox *widget,
						       gpointer user_data);


static GObjectClass *parent_class = NULL;

/* list of SwamiguiSampleEditorHandlers */
static GList *sample_editor_handlers = NULL;

/* list of SwamiguiPanelCheckFunc corresponding to sample_editor_handlers */
static GList *sample_editor_check_funcs = NULL;

/* loop type info for IpatchSampleLoopType enum GtkComboBox */
struct
{
  int loop_type;
  char *icon;
  char *label;
  char *tooltip;
} loop_type_info[] =
{
  { IPATCH_SAMPLE_LOOP_NONE, SWAMIGUI_STOCK_LOOP_NONE,
    N_("None"), N_("No looping") },
  { IPATCH_SAMPLE_LOOP_STANDARD, SWAMIGUI_STOCK_LOOP_STANDARD,
    N_("Standard"), N_("Standard continuous loop") },
  { IPATCH_SAMPLE_LOOP_RELEASE, SWAMIGUI_STOCK_LOOP_RELEASE,
    N_("Release"), N_("Loop until key is released then play to end of sample") },
  { IPATCH_SAMPLE_LOOP_PINGPONG, SWAMIGUI_STOCK_LOOP_STANDARD,	/* FIXME */
    N_("Ping Pong"), N_("Alternate playing loop forwards and backwards") }
};

/* columns from GtkListStore model for loop selector GtkComboBox */
enum
{
  LOOPSEL_COL_LOOP_TYPE,
  LOOPSEL_COL_ICON,
  LOOPSEL_COL_LABEL,
  LOOPSEL_COL_TOOLTIP,
  LOOPSEL_COL_COUNT
};


GType
swamigui_sample_editor_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static const GTypeInfo obj_info =
	{
	  sizeof (SwamiguiSampleEditorClass), NULL, NULL,
	  (GClassInitFunc) swamigui_sample_editor_class_init, NULL, NULL,
	  sizeof (SwamiguiSampleEditor), 0,
	  (GInstanceInitFunc) swamigui_sample_editor_init,
	};

      static const GInterfaceInfo panel_info =
	{ (GInterfaceInitFunc)swamigui_sample_editor_panel_iface_init, NULL, NULL };

      obj_type = g_type_register_static (GTK_TYPE_HBOX,
					 "SwamiguiSampleEditor", &obj_info, 0);
      g_type_add_interface_static (obj_type, SWAMIGUI_TYPE_PANEL, &panel_info);

      /* register default sample editor handler */
      swamigui_sample_editor_register_handler (swamigui_sample_editor_default_handler,
					       swamigui_sample_editor_default_handler_check_func);
    }

  return (obj_type);
}

static void
swamigui_sample_editor_class_init (SwamiguiSampleEditorClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = swamigui_sample_editor_set_property;
  obj_class->get_property = swamigui_sample_editor_get_property;
  obj_class->finalize = swamigui_sample_editor_finalize;

  g_object_class_override_property (obj_class, PROP_ITEM_SELECTION, "item-selection");

  g_object_class_install_property (obj_class, PROP_MARKER_BAR_HEIGHT,
		g_param_spec_int ("marker-bar-height", _("Marker bar height"),
				  _("Height of marker and loop meter bar"),
				  0, 100, DEFAULT_MARKER_BAR_HEIGHT,
				  G_PARAM_READWRITE));
}

static void
swamigui_sample_editor_panel_iface_init (SwamiguiPanelIface *panel_iface)
{
  panel_iface->label = _("Sample Editor");
  panel_iface->blurb = _("Sample data and loop editor");
  panel_iface->stockid = SWAMIGUI_STOCK_SAMPLE_VIEWER;
  panel_iface->check_selection = swamigui_sample_editor_panel_iface_check_selection;
}

static gboolean
swamigui_sample_editor_panel_iface_check_selection (IpatchList *selection,
						    GType *selection_types)
{
  GList *p;

  /* loop over handler check functions */
  for (p = sample_editor_check_funcs; p; p = p->next)
  {
    if (((SwamiguiPanelCheckFunc)(p->data))(selection, selection_types))
      return (TRUE);
  }

  return (FALSE);
}

static void
swamigui_sample_editor_set_property (GObject *object, guint property_id,
				     const GValue *value, GParamSpec *pspec)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (object);
  IpatchList *list;
  int i;

  switch (property_id)
  {
  case PROP_ITEM_SELECTION:
    list = g_value_get_object (value);
    swamigui_sample_editor_real_set_selection (editor, list);
    break;
  case PROP_MARKER_BAR_HEIGHT:
    i = g_value_get_int (value);
    if (i != editor->marker_bar_height)
    {
      editor->marker_bar_height = i;
      swamigui_sample_editor_update_sizes (editor);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
swamigui_sample_editor_get_property (GObject *object, guint property_id,
				     GValue *value, GParamSpec *pspec)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (object);

  switch (property_id)
  {
  case PROP_ITEM_SELECTION:
    g_value_set_object (value, editor->selection);
    break;
  case PROP_MARKER_BAR_HEIGHT:
    g_value_set_int (value, editor->marker_bar_height);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
swamigui_sample_editor_finalize (GObject *object)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (object);
  GList *p, *temp;

  /* remove all markers (don't destroy GtkObjects though) */
  p = editor->markers;
  while (p)
    {
      temp = p;
      p = g_list_next (p);
      swamigui_sample_editor_remove_marker_item (editor, temp, FALSE);
    }

  /* remove all tracks (don't destroy GtkObjects though) */
  p = editor->tracks;
  while (p)
    {
      temp = p;
      p = g_list_next (p);
      swamigui_sample_editor_remove_track_item (editor, temp, FALSE);
    }

  if (editor->selection) g_object_unref (editor->selection);

  if (editor->loop_start_hub) g_object_unref (editor->loop_start_hub);
  if (editor->loop_end_hub) g_object_unref (editor->loop_end_hub);
  if (editor->loopsel_ctrl) g_object_unref (editor->loopsel_ctrl);
  if (editor->loopsel_store) g_object_unref (editor->loopsel_store);

  editor->selection = NULL;
  editor->loop_start_hub = NULL;
  editor->loop_end_hub = NULL;
  editor->loopsel_ctrl = NULL;
  editor->loopsel_store = NULL;

  /* remove ref, necessary since pane is added/removed from container */
  g_object_unref (editor->loop_finder_pane);

  if (parent_class->finalize)
    (*parent_class->finalize)(object);
}

static void
swamigui_sample_editor_init (SwamiguiSampleEditor *editor)
{
  GtkWidget *vbox, *hbox;
  GtkWidget *frame;
  GtkWidget *pane;
  GtkWidget *image;
  GtkToolItem *item;
  GtkStyle *style;
  GtkCellRenderer *renderer;
  GtkTooltips *tips;
  SwamiLoopFinder *loop_finder;
  SwamiControl *marker_start, *marker_end;
  int id;

  editor->selection = NULL;
  editor->marker_bar_height = DEFAULT_MARKER_BAR_HEIGHT;
  editor->sel_marker = -1;
  editor->zoom_all = TRUE;
  editor->sel_state = SEL_INACTIVE;
  editor->center_line_color = DEFAULT_CENTER_LINE_COLOR;
  editor->marker_border_color = DEFAULT_MARKER_BORDER_COLOR;
  editor->snap_line_color = DEFAULT_SNAP_LINE_COLOR;
  editor->loop_line_color = DEFAULT_LOOP_LINE_COLOR;
  editor->loop_zoom = LOOP_CANVAS_DEF_ZOOM;

  /* create zoom/scroll modulator for sample canvas */
  editor->sample_mod = swamigui_canvas_mod_new ();

  /* attach to "update" and "snap" signals */
  g_signal_connect (editor->sample_mod, "update",
		    G_CALLBACK (swamigui_sample_editor_sample_mod_update),
		    editor);
  g_signal_connect (editor->sample_mod, "snap",
		    G_CALLBACK (swamigui_sample_editor_sample_mod_snap),
		    editor);

  /* create zoom modulator for loop canvas */
  editor->loop_mod = swamigui_canvas_mod_new ();

  /* attach to "update" and "snap" signals */
  g_signal_connect (editor->loop_mod, "update",
		    G_CALLBACK (swamigui_sample_editor_loop_mod_update),
		    editor);
  g_signal_connect (editor->loop_mod, "snap",
		    G_CALLBACK (swamigui_sample_editor_loop_mod_snap),
		    editor);

  /* create control hubs to control sample loop views for all tracks */
  /* editor retains holds references on controls */
  editor->loop_start_hub = SWAMI_CONTROL (swami_control_hub_new ()); /* ++ ref */
  swamigui_control_set_queue (editor->loop_start_hub);

  editor->loop_end_hub = SWAMI_CONTROL (swami_control_hub_new ()); /* ++ ref */
  swamigui_control_set_queue (editor->loop_end_hub);

  tips = gtk_tooltips_new ();

  /* create loop finder widget and pane, only gets packed when active */
  editor->loop_finder_gui = SWAMIGUI_LOOP_FINDER (swamigui_loop_finder_new ());
  editor->loop_finder_pane = gtk_hpaned_new ();
  gtk_paned_pack1 (GTK_PANED (editor->loop_finder_pane),
		   GTK_WIDGET (editor->loop_finder_gui), FALSE, TRUE);
  gtk_widget_show_all (editor->loop_finder_pane);

  /* we add an extra ref to finder hpane since it will be added/removed as needed */
  g_object_ref (editor->loop_finder_pane);

  /* create main vbox for tool bar and sample canvases and pack in editor hbox */
  editor->mainvbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (editor), editor->mainvbox, TRUE, TRUE, 0);

  /* create toolbox area horizontal box */
  editor->toolbar = gtk_toolbar_new ();

  editor->loopsel_store	/* ++ ref - editor holds reference to loopsel_store */
    = gtk_list_store_new (LOOPSEL_COL_COUNT, G_TYPE_INT, G_TYPE_STRING,
			  G_TYPE_STRING, G_TYPE_STRING);

  editor->loopsel = gtk_combo_box_new_with_model
    (GTK_TREE_MODEL (editor->loopsel_store));
  gtk_widget_set_sensitive (editor->loopsel, FALSE);

  item = gtk_tool_item_new ();
  gtk_container_add (GTK_CONTAINER (item), editor->loopsel);
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar), item, -1);

  /* create the control for the loop type selector */
  /* ++ ref - editor holds reference to it */
  editor->loopsel_ctrl = SWAMI_CONTROL (swami_control_func_new ());

  /* attach to the changed signal of the combo box */
  g_signal_connect (editor->loopsel, "changed",
		    G_CALLBACK (swamigui_sample_editor_loopsel_cb_changed),
		    editor);

  swami_control_func_assign_funcs (SWAMI_CONTROL_FUNC (editor->loopsel_ctrl),
				   swamigui_sample_editor_loopsel_ctrl_get,
				   swamigui_sample_editor_loopsel_ctrl_set,
				   NULL, editor);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (editor->loopsel), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (editor->loopsel), renderer,
				  "stock-id", LOOPSEL_COL_ICON, NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (editor->loopsel), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (editor->loopsel), renderer,
				  "text", LOOPSEL_COL_LABEL, NULL);

  /* create spin button and spin control */
  editor->spinbtn_start = gtk_spin_button_new (NULL, 1.0, 0);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (editor->spinbtn_start),
			     0.0, (gdouble)G_MAXINT);
  gtk_tooltips_set_tip (tips, editor->spinbtn_start,
			_("Set loop start position"), NULL);
  editor->spinbtn_start_ctrl = swamigui_control_new_for_widget_full
    (G_OBJECT (editor->spinbtn_start), G_TYPE_UINT, NULL, 0);

  item = gtk_tool_item_new ();
  gtk_container_add (GTK_CONTAINER (item), editor->spinbtn_start);
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar), item, -1);

  /* spin button and spin control */
  editor->spinbtn_end = gtk_spin_button_new (NULL, 1.0, 0);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (editor->spinbtn_end),
			     0.0, (gdouble)G_MAXINT);
  gtk_tooltips_set_tip (tips, editor->spinbtn_end,
			_("Set loop end position"), NULL);
  editor->spinbtn_end_ctrl = swamigui_control_new_for_widget_full
    (G_OBJECT (editor->spinbtn_end), G_TYPE_UINT, NULL, 0);

  item = gtk_tool_item_new ();
  gtk_container_add (GTK_CONTAINER (item), editor->spinbtn_end);
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar), item, -1);


/* Disabled until sample editing operations are implemented */

#if 0
  /* add separator */
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar),
		      gtk_separator_tool_item_new (), -1);

  /* create cut button */
  image = gtk_image_new_from_stock (GTK_STOCK_CUT, GTK_ICON_SIZE_SMALL_TOOLBAR);
  editor->cut_button = GTK_WIDGET (gtk_tool_button_new (image, _("Cut")));
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (editor->cut_button), tips,
			     _("Cut selected sample data"), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar),
		      GTK_TOOL_ITEM (editor->cut_button), -1);
  g_signal_connect (editor->cut_button, "clicked",
		    G_CALLBACK (swamigui_sample_editor_cb_cut_sample), editor);

  /* create crop button */
  image = gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  editor->crop_button = GTK_WIDGET (gtk_tool_button_new (image, _("Crop")));
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (editor->crop_button), tips,
			     _("Crop to current selection"), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar),
		      GTK_TOOL_ITEM (editor->crop_button), -1);
  g_signal_connect (editor->crop_button, "clicked",
		    G_CALLBACK (swamigui_sample_editor_cb_crop_sample), editor);

  /* create copy to new button */
  image = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_SMALL_TOOLBAR);
  editor->copy_new_button = GTK_WIDGET (gtk_tool_button_new (image,
							     _("Copy to new")));
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (editor->copy_new_button), tips,
			     _("Copy selection to new sample"), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar),
		      GTK_TOOL_ITEM (editor->copy_new_button), -1);
  g_signal_connect (editor->copy_new_button, "clicked",
		    G_CALLBACK (swamigui_sample_editor_cb_copy_new_sample),
		    editor);

  /* create sample selector button */
  editor->samplesel_button = GTK_WIDGET (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (editor->samplesel_button),
			     _("Sample selector"));
  image = gtk_image_new_from_stock (GTK_STOCK_INDEX, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (editor->samplesel_button),
				   image);
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (editor->samplesel_button), tips,
			     _("Make selection the entire sample"), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar),
		      GTK_TOOL_ITEM (editor->samplesel_button), -1);
#endif


  /* add separator */
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar),
		      gtk_separator_tool_item_new (), -1);

  /* create loop finder button */
  editor->finder_button = GTK_WIDGET (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (editor->finder_button),
			     _("Find loops"));
  image = gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (editor->finder_button),
				   image);
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (editor->finder_button), tips,
			     _("Loop point finder"), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (editor->toolbar),
		      GTK_TOOL_ITEM (editor->finder_button), -1);
  g_signal_connect (editor->finder_button, "toggled",
		    G_CALLBACK (swamigui_sample_editor_cb_loop_finder), editor);

  gtk_box_pack_start (GTK_BOX (editor->mainvbox), editor->toolbar,
		      FALSE, FALSE, 0);

  /* lower inset frame for sample viewer */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
  gtk_box_pack_start (GTK_BOX (editor->mainvbox), frame, TRUE, TRUE, 0);

  /* vbox within frame to put sample canvas */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  pane = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (hbox), pane, TRUE, TRUE, 0);

  /* connect to the size-allocate signal so we can initialize the gutter pos */
  g_signal_connect_after (pane, "size-allocate",
			  G_CALLBACK (editor_cb_pane_size_allocate), NULL);


  /* create sample editor canvas */

  editor->sample_canvas = GNOME_CANVAS (gnome_canvas_new ());
  gnome_canvas_set_center_scroll_region (GNOME_CANVAS (editor->sample_canvas),
					 FALSE);
  gtk_paned_pack1 (GTK_PANED (pane), GTK_WIDGET (editor->sample_canvas),
		   TRUE, TRUE);
  g_signal_connect (editor->sample_canvas, "event",
		    G_CALLBACK (swamigui_sample_editor_cb_sample_canvas_event),
		    editor);
  g_signal_connect (editor->sample_canvas, "size-allocate",
		    G_CALLBACK (swamigui_sample_editor_cb_canvas_size_allocate),
		    editor);

  /* change background color of canvas to black */
  style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET
						(editor->sample_canvas)));
  style->bg[GTK_STATE_NORMAL] = style->black;
  gtk_widget_set_style (GTK_WIDGET (editor->sample_canvas), style);

  /* create sample view marker bar border horizontal line */
  editor->sample_border_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->sample_canvas),
			     GNOME_TYPE_CANVAS_LINE,
			     "fill-color-rgba", editor->marker_border_color,
			     "width-pixels", 1,
			     NULL);

  /* create snap lines */
  editor->xsnap_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->sample_canvas),
			     GNOME_TYPE_CANVAS_LINE,
     			     "fill-color-rgba", editor->snap_line_color,
			     "width-pixels", 2,
			     NULL);
  gnome_canvas_item_hide (editor->xsnap_line);

  editor->ysnap_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->sample_canvas),
			     GNOME_TYPE_CANVAS_LINE,
     			     "fill-color-rgba", editor->snap_line_color,
			     "width-pixels", 2,
			     NULL);
  gnome_canvas_item_hide (editor->ysnap_line);

  /* create sample loop canvas */

  editor->loop_canvas = GNOME_CANVAS (gnome_canvas_new ());
  gnome_canvas_set_center_scroll_region (GNOME_CANVAS (editor->loop_canvas),
					 FALSE);
  gtk_paned_pack2 (GTK_PANED (pane), GTK_WIDGET (editor->loop_canvas),
		   FALSE, TRUE);

  g_signal_connect (editor->loop_canvas, "event",
		    G_CALLBACK (swamigui_sample_editor_cb_loop_canvas_event),
		    editor);
  g_signal_connect (editor->loop_canvas, "size-allocate",
  		    G_CALLBACK (swamigui_sample_editor_cb_canvas_size_allocate),
  		    editor);

  /* change background color of canvas to black */
  style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET
						(editor->loop_canvas)));
  style->bg[GTK_STATE_NORMAL] = style->black;
  gtk_widget_set_style (GTK_WIDGET (editor->loop_canvas), style);

  /* create vertical center line for loop view */
  editor->loop_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->loop_canvas),
			     GNOME_TYPE_CANVAS_LINE,
     			     "fill-color-rgba", editor->loop_line_color,
			     "width-pixels", 1,
			     NULL);

  /* create loop view marker bar border horizontal line */
  editor->loop_border_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->loop_canvas),
			     GNOME_TYPE_CANVAS_LINE,
			     "fill-color-rgba", editor->marker_border_color,
			     "width-pixels", 1,
			     NULL);

  /* create zoom snap line for loop view */
  editor->loop_snap_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->loop_canvas),
			     GNOME_TYPE_CANVAS_LINE,
     			     "fill-color-rgba", editor->snap_line_color,
			     "width-pixels", 2,
			     NULL);
  gnome_canvas_item_hide (editor->loop_snap_line);

  /* attach a horizontal scrollbar to the sample viewer */
  editor->hscrollbar = gtk_hscrollbar_new (NULL);
  gtk_box_pack_start (GTK_BOX (editor->mainvbox), editor->hscrollbar,
		      FALSE, FALSE, 0);

  g_signal_connect_after (gtk_range_get_adjustment
			  (GTK_RANGE (editor->hscrollbar)), "value-changed",
			  G_CALLBACK (swamigui_sample_editor_cb_scroll),
			  editor);

  gtk_widget_show_all (editor->mainvbox);

  /* create marker 0 for selection and hide it */
  id = swamigui_sample_editor_add_marker (editor, 0, NULL, NULL);
  swamigui_sample_editor_show_marker (editor, id, FALSE);

  loop_finder = editor->loop_finder_gui->loop_finder;

  /* create marker for loop finder start search window and hide it */
  id = swamigui_sample_editor_add_marker (editor, 0, &marker_start, &marker_end);
  swamigui_sample_editor_show_marker (editor, id, FALSE);

  /* connect the marker controls to the loop finder properties */
  swami_control_prop_connect_to_control (G_OBJECT (loop_finder),
					 "window1-start", marker_start,
					 SWAMI_CONTROL_CONN_BIDIR_INIT);
  swami_control_prop_connect_to_control (G_OBJECT (loop_finder),
					 "window1-end", marker_end,
					 SWAMI_CONTROL_CONN_BIDIR_INIT);

  /* create marker for loop finder end search window and hide it */
  id = swamigui_sample_editor_add_marker (editor, 0, &marker_start, &marker_end);
  swamigui_sample_editor_show_marker (editor, id, FALSE);

  /* connect the marker controls to the loop finder properties */
  swami_control_prop_connect_to_control (G_OBJECT (loop_finder),
					 "window2-start", marker_start,
					 SWAMI_CONTROL_CONN_BIDIR_INIT);
  swami_control_prop_connect_to_control (G_OBJECT (loop_finder),
					 "window2-end", marker_end,
					 SWAMI_CONTROL_CONN_BIDIR_INIT);
}

#if 0
static void
swamigui_sample_editor_cb_cut_sample (GtkToggleToolButton *button,
				      gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  MarkerInfo *marker;
  IpatchSampleData *data;
  IpatchSampleStore *store;

  /* shouldn't happen, but.. */
  if (!editor->markers || !editor->markers->data) return;

  /* first marker is selection */
  marker = (MarkerInfo *)(editor->markers->data);
  if (!marker->visible) return;

  data = get_selection_sample_data (editor);	/* ++ ref */
  if (!data) return;

  store = ipatch_sample_data_get_default_store (data);	/* ++ ref */

  list = ipatch_sample_list_new ();
  ipatch_sample_list_append (list, store, 0, 0, 0);
  ipatch_sample_list_cut (list, marker->start_pos,
			  marker->end_pos - marker->start_pos + 1);

  g_object_unref (data);	/* -- unref */
}

static void
swamigui_sample_editor_cb_crop_sample (GtkToggleToolButton *button,
				       gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  IpatchSampleData *data;

  data = get_selection_sample_data (editor);
  if (!data) return;
}

static void
swamigui_sample_editor_cb_copy_new_sample (GtkToggleToolButton *button,
					   gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  IpatchSampleData *data;

  data = get_selection_sample_data (editor);
  if (!data) return;
}

/* returns a IpatchSampleData object for the current selection or NULL if
 * not a single item which has an IpatchSample interface which allows
 * get/set of sample data.  Caller owns ref to sample data
 */
static IpatchSampleData *
get_selection_sample_data (SwamiguiSampleEditor *editor)
{
  IpatchSample *sample;

  if (!editor->selection || !editor->selection->items
      || editor->selection->items->next
      || !IPATCH_IS_SAMPLE (editor->selection->items->data))
    return (NULL);

  sample = IPATCH_SAMPLE (editor->selection->items->data);
  if (!ipatch_sample_has_data (sample)) return (NULL);

  return (ipatch_sample_get_data (sample));	/* !! caller takes reference */
}
#endif

static void
swamigui_sample_editor_cb_loop_finder (GtkToggleToolButton *button,
				       gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);

  editor->loop_finder_active = gtk_toggle_tool_button_get_active (button);

  if (editor->loop_finder_active)
  { /* repack the main vbox into the loop finder pane */
    g_object_ref (editor->mainvbox);
    gtk_container_remove (GTK_CONTAINER (editor), editor->mainvbox);
    gtk_paned_pack2 (GTK_PANED (editor->loop_finder_pane), editor->mainvbox,
		     TRUE, TRUE);
    g_object_unref (editor->mainvbox);

    /* pack the loop finder pane into the editor widget */
    gtk_box_pack_start (GTK_BOX (editor), editor->loop_finder_pane, TRUE, TRUE, 0);
  }
  else
  {
    /* remove the loop finder pane from the editor widget */
    gtk_container_remove (GTK_CONTAINER (editor), editor->loop_finder_pane);

    /* repack the main vbox into the editor */
    g_object_ref (editor->mainvbox);
    gtk_container_remove (GTK_CONTAINER (editor->loop_finder_pane),
			  editor->mainvbox);
    gtk_box_pack_start (GTK_BOX (editor), editor->mainvbox, TRUE, TRUE, 0);
    g_object_unref (editor->mainvbox);
  }

  /* show/hide finder markers */
  swamigui_sample_editor_show_marker (editor,
    SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_START, editor->loop_finder_active);
  swamigui_sample_editor_show_marker (editor, 
    SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_END, editor->loop_finder_active);
}

/* size-allocate callback to initialize hpane gutter position */
static void
editor_cb_pane_size_allocate (GtkWidget *pane, GtkAllocation *allocation,
			      gpointer user_data)
{
  int width;

  /* set loop view size to 1/5 total size or 160 pixels whichever is smallest */
  width = allocation->width / 5;
  if (width > 160) width = 160;

  /* disconnect the from the signal (one time only init) */
  g_signal_handlers_disconnect_by_func (pane, editor_cb_pane_size_allocate,
					user_data);

  gtk_paned_set_position (GTK_PANED (pane), allocation->width - width);
}

static void
swamigui_sample_editor_cb_canvas_size_allocate (GtkWidget *widget,
						GtkAllocation *allocation,
						gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  GnomeCanvas *canvas = GNOME_CANVAS (widget);

  swamigui_sample_editor_update_canvas_size (editor, canvas);
}

static void
swamigui_sample_editor_update_sizes (SwamiguiSampleEditor *editor)
{
  swamigui_sample_editor_update_canvas_size (editor, editor->sample_canvas);
  swamigui_sample_editor_update_canvas_size (editor, editor->loop_canvas);
}

static void
swamigui_sample_editor_update_canvas_size (SwamiguiSampleEditor *editor,
					   GnomeCanvas *canvas)
{
  TrackInfo *track_info;
  GnomeCanvasItem *item;
  int width, height, sample_height, count, y, y2, i;
  double zoom, fullzoom;
  GList *p;

  width = GTK_WIDGET (canvas)->allocation.width;
  height = GTK_WIDGET (canvas)->allocation.height;
  sample_height = height - editor->marker_bar_height;

  if (width <= 0) width = 1;
  if (height <= 0) height = 1;
  if (sample_height <= 0) sample_height = 1;


  /* update marker bar border line */
  item = canvas == editor->sample_canvas ? editor->sample_border_line
    : editor->loop_border_line;

  swamigui_util_canvas_line_set (item, 0, editor->marker_bar_height - 1,
				 width, editor->marker_bar_height - 1);

  p = editor->tracks;
  if (!p) return;

  /* get count and last track in list */
  count = 1;
  while (p->next)
    {
      p = g_list_next (p);
      count++;
    }

  /* if sample canvas, calculate full sample zoom and use it if current zoom
   * would cause complete sample view to be less than the canvas width */
  if (canvas == editor->sample_canvas)
    {
      fullzoom = (double)(editor->sample_size) / width;
      track_info = (TrackInfo *)(p->data);
      g_object_get (track_info->sample_view, "zoom", &zoom, NULL);
    }

  /* loop in reverse over list (top sample down) */
  for (i = 1, y = editor->marker_bar_height; p; p = g_list_previous (p), i++)
    {
      y2 = i * sample_height / count + editor->marker_bar_height;

      track_info = (TrackInfo *)(p->data);

      /* update data view */
      item = (canvas == editor->sample_canvas) ? track_info->sample_view
	: track_info->loop_view;
      g_object_set (item,
		    "y", y,
		    "width", width,
		    "height", y2 - y,
		    NULL);

      /* set zoom value if current sample zoom would be less than canvas width,
       * or zoom_all is currently set */
      if (canvas == editor->sample_canvas
	  && (editor->zoom_all || fullzoom < zoom))
	g_object_set (item, "zoom", fullzoom, NULL);

      /* update horizontal center line */
      item = (canvas == editor->sample_canvas)
	? track_info->sample_center_line : track_info->loop_center_line;

      swamigui_util_canvas_line_set (item, 0, y + (y2 - y) / 2, width - 1,
				     y + (y2 - y) / 2);
      y = y2 + 1;
    }

  if (canvas == editor->sample_canvas)
    marker_control_update_all (editor); /* update all markers */
  else if (canvas == editor->loop_canvas)
    swamigui_util_canvas_line_set (editor->loop_line, width / 2, 0,
				   width / 2, height);
}

/* horizontal scroll bar value changed callback */
static void
swamigui_sample_editor_cb_scroll (GtkAdjustment *adj, gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  marker_control_update_all (editor); /* update all markers */
}

static gboolean
swamigui_sample_editor_cb_sample_canvas_event (GnomeCanvas *canvas,
					       GdkEvent *event, gpointer data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (data);
  SwamiguiSampleCanvas *sample_view;
  TrackInfo *track_info;
  MarkerInfo *marker_info;
  GdkEventButton *btn_event;
  GdkEventMotion *motion_event;
  GdkCursor *cursor;
  int val;
  guint newstart, newend;
  gboolean onbox;

  if (!editor->tracks) return (FALSE);
  track_info = (TrackInfo *)(editor->tracks->data);
  sample_view = SWAMIGUI_SAMPLE_CANVAS (track_info->sample_view);

  if (event->type == GDK_BUTTON_PRESS)
    btn_event = (GdkEventButton *)event;

  /* don't process zoom/scroll mod events if its a middle marker click */
  if (event->type != GDK_BUTTON_PRESS
      || btn_event->button != 2
      || btn_event->y > editor->marker_bar_height)
  {
    /* handle zoom/scroll related events */
    if (swamigui_canvas_mod_handle_event (editor->sample_mod, event))
      return (FALSE);	/* was zoom/scroll event - return */
  }

  switch (event->type)
  {
  case GDK_MOTION_NOTIFY:
    motion_event = (GdkEventMotion *)event;

    if (editor->sel_marker != -1)	/* active marker? */
    {
      marker_info = g_list_nth_data (editor->markers, editor->sel_marker);
      if (!marker_info) break;

      val = swamigui_sample_canvas_xpos_to_sample (sample_view, motion_event->x,
						   NULL);
      newstart = marker_info->start_pos;
      newend = marker_info->end_pos;

      if (editor->sel_marker_edge == -1)	/* start edge selected? */
	newstart = CLAMP (val, 0, (int)editor->sample_size);
      else if (editor->sel_marker_edge == 1)	/* end edge selected? */
	newend = CLAMP (val, 0, (int)editor->sample_size);
      else		/* range move (both edges selected) */
      {
	val -= editor->move_range_ofs;
	newstart = CLAMP (val, 0, (int)(editor->sample_size - (newend - newstart)));
	newend += newstart - marker_info->start_pos;
      }

      /* swap selection if endpoints in reverse order */
      if (newstart > newend)
	editor->sel_marker_edge = -editor->sel_marker_edge;

      /* set the marker */
      swamigui_sample_editor_set_marker_info (editor, marker_info,
					      newstart, newend);
      break;
    }
    else if (editor->sel_state != SEL_INACTIVE)	/* if drag sel not inactive.. */
    {
      val = swamigui_sample_canvas_xpos_to_sample (sample_view, motion_event->x,
						   NULL);
      val = CLAMP (val, 0, editor->sample_size);

      /* get selection marker info */
      marker_info = g_list_nth_data (editor->markers, 0);

      if (editor->sel_state == SEL_MAYBE)	/* drag just begun? */
      {
	if (val == editor->sel_temp) break;	/* drag position not changed? */

	/* just assume end drag, swap below if needed */
	editor->sel_state = SEL_END;
	newstart = editor->sel_temp;
	newend = val;
      }
      else if (editor->sel_state == SEL_START)
      {
	newstart = val;
	newend = marker_info->end_pos;
      }
      else
      {
	newstart = marker_info->start_pos;
	newend = val;
      }

      if (newstart > newend)	/* swap start/end if needed */
	editor->sel_state = editor->sel_state == SEL_START ? SEL_END : SEL_START;

      /* set selection marker - start/end swapped in function if needed */

      /* Disabled until sample editing operations implemented */
#if 0
      swamigui_sample_editor_show_marker (editor, 0, TRUE);
#endif
      swamigui_sample_editor_set_marker (editor, 0, newstart, newend);
    }
    else  /* no marker sel or range sel active - mouse cursor over marker? */
    {
      /* see if mouse is over a marker */
      marker_info = pos_is_marker (editor, motion_event->x, motion_event->y,
				   NULL, &onbox);

      /* see if cursor should be changed */
      if (!onbox && (marker_info != NULL) != editor->marker_cursor)
      {
	if (marker_info)
	  cursor = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW); /* ++ ref */
	else cursor = gdk_cursor_new (GDK_LEFT_PTR);

	gdk_window_set_cursor (GTK_WIDGET (editor->sample_canvas)->window,
			       cursor);
	gdk_cursor_unref (cursor);
	editor->marker_cursor = marker_info != NULL;
      }
    }
    break;
  case GDK_BUTTON_PRESS:
    btn_event = (GdkEventButton *)event;

    if (btn_event->button == 2)		/* middle click moves marker ranges */
    {
      marker_info = pos_is_marker (editor, btn_event->x, btn_event->y,
				   NULL, &onbox);
      if (marker_info && onbox)	/* if click on a range box.. */
      {
	editor->sel_marker = g_list_index (editor->markers, marker_info);
	editor->sel_marker_edge = 0;

	/* calculate offset in samples from range start to click pos */
	val = swamigui_sample_canvas_xpos_to_sample (sample_view, btn_event->x,
						     NULL);
	editor->move_range_ofs = val - marker_info->start_pos;
      }

      break;
    }

    /* make sure its button 1 */
    if (btn_event->button != 1) break;

    /* is it a marker click? */
    marker_info = pos_is_marker (editor, btn_event->x, btn_event->y,
				 &editor->sel_marker_edge, NULL);
    if (marker_info)
    {
      editor->sel_marker = g_list_index (editor->markers, marker_info);
      break;
    }

    /* get sample position of click and assign to sel_temp */
    editor->sel_temp
      = swamigui_sample_canvas_xpos_to_sample (sample_view, btn_event->x, NULL);

    /* if click is within the sample, we have a potential selection */
    if (editor->sel_temp >= 0 && editor->sel_temp < editor->sample_size)
      editor->sel_state = SEL_MAYBE;

    break;
  case GDK_BUTTON_RELEASE:
    btn_event = (GdkEventButton *)event;

    if (btn_event->button == 1)
    {
      editor->sel_marker = -1;
      editor->sel_state = SEL_INACTIVE;
    }
    else if (btn_event->button == 2)
      editor->sel_marker = -1;
    break;
  default:
    break;
  }

  return (FALSE);
}

static gboolean
swamigui_sample_editor_cb_loop_canvas_event (GnomeCanvas *canvas,
					     GdkEvent *event, gpointer data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (data);

  /* handle zoom/scroll related events */
  swamigui_canvas_mod_handle_event (editor->loop_mod, event);

  return (FALSE);	/* was zoom/scroll event - return */
}

/* check if a given xpos is on a marker */
static MarkerInfo *
pos_is_marker (SwamiguiSampleEditor *editor, int xpos, int ypos,
	       int *marker_edge, gboolean *is_onbox)
{
  MarkerInfo *marker_info, *match = NULL;
  gboolean match_marker_edge = -1;
  SwamiguiSampleCanvas *sample_view;
  GnomeCanvasItem *item;
  int x, ofs = -1;
  int inview, onsample, pos;
  int startdiff, enddiff;
  GList *p;

  if (is_onbox) *is_onbox = FALSE;
  if (marker_edge) *marker_edge = -1;

  if (!editor->markers || !editor->tracks) return (NULL);

  sample_view = SWAMIGUI_SAMPLE_CANVAS
    (((TrackInfo *)(editor->tracks->data))->sample_view);

  /* click in marker bar area? */
  if (ypos <= editor->marker_bar_height)
  {	/* retrieve canvas item at the given coordinates */
    item = gnome_canvas_get_item_at (editor->sample_canvas, xpos, ypos);
    if (!item) return (NULL);

    for (p = editor->markers; p; p = p->next)
    {
      marker_info = (MarkerInfo *)(p->data);

      if (marker_info->range_box == item)	/* range box matches? */
      {
	if (marker_edge)
	{
	  pos = swamigui_sample_canvas_xpos_to_sample (sample_view, xpos,
						       &onsample);
	  startdiff = marker_info->start_pos - pos;
	  enddiff = marker_info->end_pos - pos;

	  if (ABS (startdiff) <= ABS (enddiff))
	    *marker_edge = -1;
	  else *marker_edge = 1;
	}

	if (is_onbox) *is_onbox = TRUE;

	return (marker_info);
      }
    }

    return (NULL);
  }

  /* click not in marker bar area */

  for (p = editor->markers; p; p = p->next)
  {
    marker_info = (MarkerInfo *)(p->data);

    x = swamigui_sample_canvas_sample_to_xpos (sample_view, 
					       marker_info->start_pos, &inview);
    if (inview == 0)
    {
      x = ABS (xpos - x);
      if (x <= MARKER_CLICK_DISTANCE && (ofs == -1 || x < ofs))
	{
	  match = marker_info;
	  match_marker_edge = -1;
	  ofs = x;
	}
    }

    x = swamigui_sample_canvas_sample_to_xpos (sample_view,
					       marker_info->end_pos, &inview);
    if (inview == 0)
    {
      x = ABS (xpos - x);
      if (x <= MARKER_CLICK_DISTANCE && (ofs == -1 || x < ofs))
	{
	  match = marker_info;
	  match_marker_edge = 1;
	  ofs = x;
	}
    }
  }

  if (marker_edge) *marker_edge = match_marker_edge;
  if (is_onbox) *is_onbox = FALSE;

  return (match);
}

/* canvas zoom/scroll modulator update signal handler */
static void
swamigui_sample_editor_sample_mod_update (SwamiguiCanvasMod *mod, double xzoom,
					  double yzoom, double xscroll,
					  double yscroll, double xpos,
					  double ypos, gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  GnomeCanvasItem *sample_view;
  double zoom;
  int val;

  if (!editor->tracks) return;

  if (xzoom != 1.0)
    swamigui_sample_editor_zoom_ofs (editor, xzoom, xpos);

  if (xscroll != 0.0)
  {
    /* get first sample view item */
    sample_view = ((TrackInfo *)(editor->tracks->data))->sample_view;

    /* get zoom from first sample canvas (samples/pixel) */
    g_object_get (sample_view, "zoom", &zoom, NULL);

    xscroll *= zoom;	/* convert from scroll pixels/sec to samples/sec */

    /* do accumulation of scroll value, since we can only scroll by samples */
    editor->scroll_acc += xscroll;

    if (editor->scroll_acc >= 1.0) val = editor->scroll_acc;
    else if (editor->scroll_acc <= -1.0) val = -(int)(-editor->scroll_acc);
    else val = 0;

    if (val != 0)
    {
      editor->scroll_acc -= val;
      swamigui_sample_editor_scroll_ofs (editor, val);
    }
  }

  /* no support for Y zoom/scroll yet */
}

/* canvas zoom/scroll modulator snap signal handler */
static void
swamigui_sample_editor_sample_mod_snap (SwamiguiCanvasMod *mod, guint actions,
					double xsnap, double ysnap,
					gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  int height, width;

  width = GTK_WIDGET (editor->sample_canvas)->allocation.width;
  height = GTK_WIDGET (editor->sample_canvas)->allocation.height;

  if (actions & (SWAMIGUI_CANVAS_MOD_ZOOM_X | SWAMIGUI_CANVAS_MOD_SCROLL_X))
  {
    swamigui_util_canvas_line_set (editor->xsnap_line, xsnap,
				   editor->marker_bar_height, xsnap, height);
    gnome_canvas_item_show (editor->xsnap_line);
    editor->scroll_acc = 0.0;
  }
  else gnome_canvas_item_hide (editor->xsnap_line);

  if (actions & (SWAMIGUI_CANVAS_MOD_ZOOM_Y | SWAMIGUI_CANVAS_MOD_SCROLL_Y))
  {
    swamigui_util_canvas_line_set (editor->ysnap_line, 0, ysnap, width, ysnap);
    gnome_canvas_item_show (editor->ysnap_line);
  }
  else gnome_canvas_item_hide (editor->ysnap_line);
}

/* loop canvas zoom modulator update signal handler (scrolling does not apply) */
static void
swamigui_sample_editor_loop_mod_update (SwamiguiCanvasMod *mod, double xzoom,
					double yzoom, double xscroll,
					double yscroll, double xpos,
					double ypos, gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);

  if (xzoom != 1.0)
    swamigui_sample_editor_loop_zoom (editor, xzoom);

  /* no support for Y zoom yet */
}

/* loop canvas zoom modulator snap signal handler */
static void
swamigui_sample_editor_loop_mod_snap (SwamiguiCanvasMod *mod, guint actions,
				      double xsnap, double ysnap,
				      gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  int height;

  height = GTK_WIDGET (editor->sample_canvas)->allocation.height;

  if (actions & SWAMIGUI_CANVAS_MOD_ZOOM_X)
  {
    swamigui_util_canvas_line_set (editor->loop_snap_line, xsnap,
				   editor->marker_bar_height, xsnap, height);
    gnome_canvas_item_show (editor->loop_snap_line);
  }
  else gnome_canvas_item_hide (editor->loop_snap_line);
}

/**
 * swamigui_sample_editor_new:
 *
 * Create a new sample view object
 *
 * Returns: new widget of type SwamiguiSampleEditor
 */
GtkWidget *
swamigui_sample_editor_new (void)
{
  return (GTK_WIDGET (gtk_type_new (swamigui_sample_editor_get_type ())));
}

/**
 * swamigui_sample_editor_zoom_ofs:
 * @editor: Sample editor object
 * @zoom_amt: Zoom multiplier (> 1 = zoom in, < 1 = zoom out)
 * @zoom_xpos: X coordinate position to keep stationary
 *
 * Zoom the sample canvas the specified scale amount and modify the
 * start sample position to keep the given X coordinate stationary.
 */
void
swamigui_sample_editor_zoom_ofs (SwamiguiSampleEditor *editor, double zoom_amt,
				 double zoom_xpos)
{
  GnomeCanvasItem *sample_view;
  TrackInfo *track_info;
  double zoom, newzoom, sample_ofs;
  guint start, newstart;
  int width;
  GList *p;

  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  if (!editor->tracks) return;

  /* get first sample view item */
  sample_view = ((TrackInfo *)(editor->tracks->data))->sample_view;

  /* get values from first sample in editor */
  g_object_get (sample_view,
		"zoom", &zoom,
		"start", &start,
		"width", &width,
		NULL);

  sample_ofs = zoom_xpos * zoom; /* offset, in samples, to zoom xpos */
  newzoom = zoom * (1.0 / zoom_amt);
  newstart = start;

  editor->zoom_all = FALSE;

  /* do some bounds checking on the zoom value */
  if (newzoom < SAMPLE_CANVAS_MIN_ZOOM)
    newzoom = SAMPLE_CANVAS_MIN_ZOOM;
  else if (width * newzoom > editor->sample_size)
    { /* view exceeds sample data */
      newstart = 0;
      newzoom = editor->sample_size / (double)width;
      editor->zoom_all = TRUE;
    }
  else
    {
      sample_ofs -= zoom_xpos * newzoom; /* subtract new zoom offset */
      if (sample_ofs < 0.0 && -sample_ofs > newstart) newstart = 0;
      else newstart += (sample_ofs + 0.5);

      /* make sure sample doesn't end in the middle of the display */
      if (newstart + width * newzoom > editor->sample_size)
	newstart = editor->sample_size - width * newzoom;
    }

  if (newzoom == zoom && newstart == start) return;

  p = editor->tracks;
  while (p)
    {
      track_info = (TrackInfo *)(p->data);
      g_object_set (track_info->sample_view,
		    "zoom", newzoom,
		    "start", newstart,
		    NULL);
      p = g_list_next (p);
    }
}

/**
 * swamigui_sample_editor_scroll_ofs:
 * @editor: Sample editor object
 * @sample_ofs: Offset amount in samples
 * 
 * Scroll the sample canvas by a given offset.
 */
void
swamigui_sample_editor_scroll_ofs (SwamiguiSampleEditor *editor,
				   int sample_ofs)
{
  GnomeCanvasItem *sample_view;
  double zoom;
  guint start_sample;
  int newstart, width;
  int last_sample;

  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));
  if (sample_ofs == 0) return;

  /* get first sample view item */
  sample_view = ((TrackInfo *)(editor->tracks->data))->sample_view;

  g_object_get (sample_view,
		"start", &start_sample,
		"zoom", &zoom,
		"width", &width,
		NULL);

  last_sample = editor->sample_size - zoom * width;
  if (last_sample < 0) return;	/* sample too small for current zoom? */

  newstart = (int)start_sample + sample_ofs;
  newstart = CLAMP (newstart, 0, last_sample);

  if (newstart == start_sample) return;

  g_object_set (sample_view, "start", newstart, NULL);
}

/**
 * swamigui_sample_editor_loop_zoom:
 * @editor: Sample editor object
 * @zoom_amt: Zoom multiplier (> 1 = zoom in, < 1 = zoom out)
 *
 * Zoom the loop viewer canvas the specified scale amount.  Zoom always occurs
 * around center of loop cross overlap.
 */
void
swamigui_sample_editor_loop_zoom (SwamiguiSampleEditor *editor, double zoom_amt)
{
  GnomeCanvasItem *loop_view;
  TrackInfo *track_info;
  double zoom, newzoom;
  GList *p;

  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  if (!editor->tracks) return;

  /* get first loop view item */
  loop_view = ((TrackInfo *)(editor->tracks->data))->loop_view;

  /* get values from first sample in editor */
  g_object_get (loop_view,
		"zoom", &zoom,
		NULL);

  newzoom = zoom * (1.0 / zoom_amt);

  /* do some bounds checking on the zoom value */
  if (newzoom < LOOP_CANVAS_MIN_ZOOM) newzoom = LOOP_CANVAS_MIN_ZOOM;
  else if (newzoom > 1.0) newzoom = 1.0;

  if (newzoom == zoom) return;

  editor->loop_zoom = newzoom;

  p = editor->tracks;
  while (p)
    {
      track_info = (TrackInfo *)(p->data);
      g_object_set (track_info->loop_view,
		    "zoom", newzoom,
		    NULL);
      p = g_list_next (p);
    }
}

/**
 * swamigui_sample_editor_set_selection:
 * @editor: Sample editor object
 * @items: List of selected items or %NULL to unset selection
 *
 * Set the items of a sample editor widget. The @items list will usually
 * contain a single patch item that has sample data associated with it,
 * although sometimes multiple items will be handled as in the case of
 * stereo pairs.
 */
void
swamigui_sample_editor_set_selection (SwamiguiSampleEditor *editor,
				 IpatchList *items)
{
  if (swamigui_sample_editor_real_set_selection (editor, items))
    g_object_notify (G_OBJECT (editor), "item-selection");
}

static gboolean
swamigui_sample_editor_real_set_selection (SwamiguiSampleEditor *editor,
				       IpatchList *items)
{
  SwamiguiSampleEditorHandler hfunc;
  GList *p;

  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor), FALSE);
  g_return_val_if_fail (!items || IPATCH_IS_LIST (items), FALSE);

  if (editor->selection) g_object_unref (editor->selection); /* -- unref old */

  /* if items list then duplicate it and take over the reference */
  if (items) editor->selection = ipatch_list_duplicate (items);	/* !! */
  else editor->selection = NULL;

  if (editor->handler)		/* active handler? */
    {
      editor->status = SWAMIGUI_SAMPLE_EDITOR_UPDATE;
      if (!items || !(*editor->handler)(editor))
	swamigui_sample_editor_deactivate_handler (editor);
    }

  if (items && !editor->handler) /* re-test in case it was de-activated */
    {
      editor->status = SWAMIGUI_SAMPLE_EDITOR_INIT;
      p = sample_editor_handlers;
      while (p)			/* try handlers */
	{
	  hfunc = (SwamiguiSampleEditorHandler)(p->data);
	  if ((*hfunc)(editor)) /* selection handled? */
	    {
	      editor->handler = hfunc;
	      break;
	    }
	  p = g_list_next (p);
	}
    }

  editor->status = SWAMIGUI_SAMPLE_EDITOR_NORMAL;

  return (TRUE);
}

static void
swamigui_sample_editor_deactivate_handler (SwamiguiSampleEditor *editor)
{
  swamigui_sample_editor_reset (editor);
  editor->handler = NULL;
  editor->handler_data = NULL;
}

/**
 * swamigui_sample_editor_get_selection:
 * @editor: Sample editor widget
 *
 * Get the list of active items in a sample editor widget.
 *
 * Returns: New list containing selected items which has a ref count of one
 *   which the caller owns or %NULL if no items selected. Remove the
 *   reference when finished with it.
 */
IpatchList *
swamigui_sample_editor_get_selection (SwamiguiSampleEditor *editor)
{
  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor), NULL);

  if (editor->selection && editor->selection->items)
    return (ipatch_list_duplicate (editor->selection));
  else return (NULL);
}

/**
 * swamigui_sample_editor_register_handler:
 * @handler: Sample editor handler function to register
 * @check_func: Function used to check if an item selection is valid for this handler
 *
 * Registers a new handler for sample editor widgets. Sample editor handlers
 * interface patch item's of particular types with sample data and related
 * parameters (such as looping params).
 *
 * MT: This function should only be called within GUI thread.
 */
void
swamigui_sample_editor_register_handler (SwamiguiSampleEditorHandler handler,
					 SwamiguiPanelCheckFunc check_func)
{
  g_return_if_fail (handler != NULL);
  g_return_if_fail (check_func != NULL);

  sample_editor_handlers = g_list_prepend (sample_editor_handlers, handler);
  sample_editor_check_funcs = g_list_prepend (sample_editor_check_funcs, check_func);
}

/**
 * swamigui_sample_editor_unregister_handler:
 * @handler: Handler function to unregister
 *
 * Unregisters a handler previously registered with
 * swamigui_sample_editor_register_handler().
 *
 * MT: This function should only be called in GUI thread.
 */
void
swamigui_sample_editor_unregister_handler (SwamiguiSampleEditorHandler handler)
{
  int handler_index;
  GList *p;

  g_return_if_fail (handler != NULL);

  handler_index = g_list_index (sample_editor_handlers, handler);
  g_return_if_fail (handler_index != -1);

  sample_editor_handlers = g_list_remove (sample_editor_handlers, handler);

  /* remove the corresponding check function */
  p = g_list_nth (sample_editor_check_funcs, handler_index);
  sample_editor_check_funcs = g_list_delete_link (sample_editor_check_funcs, p);
}

/**
 * swamigui_sample_editor_reset:
 * @editor: Sample editor widget
 * 
 * Resets a sample editor by removing all tracks and markers and disconnecting
 * loop view controls. Usually only used by sample editor handlers.
 */
void
swamigui_sample_editor_reset (SwamiguiSampleEditor *editor)
{
  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  g_object_set (editor->loop_finder_gui->loop_finder, "sample", NULL, NULL);

  swamigui_sample_editor_remove_all_tracks (editor);
  swamigui_sample_editor_remove_all_markers (editor);

  /* disable loop type selector */
  swamigui_sample_editor_set_loop_types (editor, NULL, FALSE);

  /* disconnect any controls connected to loop view hub controls */
  swami_control_disconnect_all (editor->loop_start_hub);
  swami_control_disconnect_all (editor->loop_end_hub);
  swami_control_disconnect_all (editor->loopsel_ctrl);

  /* disconnect any controls to spin buttons */
  swami_control_disconnect_all (editor->spinbtn_start_ctrl);
  swami_control_disconnect_all (editor->spinbtn_end_ctrl);
}

/**
 * swamigui_sample_editor_get_loop_controls:
 * @editor: Sample editor object
 * @loop_start: Output - Loop view start point control (%NULL to ignore)
 * @loop_end: Output - Loop view end point control (%NULL to ignore)
 * 
 * Get the loop start and end controls that are connected to all loop view
 * start and end properties. Essentially controls for the loop view start
 * and end points.
 */
void
swamigui_sample_editor_get_loop_controls (SwamiguiSampleEditor *editor,
					  SwamiControl **loop_start,
					  SwamiControl **loop_end)
{
  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  if (loop_start) *loop_start = editor->loop_start_hub;
  if (loop_end) *loop_end = editor->loop_end_hub;
}

/**
 * swamigui_sample_editor_add_track:
 * @editor: Sample editor object
 * @sample: Object with an #IpatchSample interface to add
 * @right_chan: Only used if @sample is stereo. If %TRUE the right channel
 *   will be used, %FALSE will use the left channel.
 *
 * Add a sample track to a sample editor. Usually only done by
 * sample editor handlers. This function can be used to add multiple
 * samples for stereo or multi-track audio.
 *
 * Returns: The index of the new track in the sample editor object (starting
 * from 0).
 */
int
swamigui_sample_editor_add_track (SwamiguiSampleEditor *editor,
				  IpatchSampleData *sample,
				  gboolean right_chan)
{
  TrackInfo *track_info;
  SwamiControl *ctrl;
  double zoom;
  guint sample_size;
  int width;

  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor), 0);
  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sample), 0);

  /* calculate zoom to view entire sample */
  g_object_get (sample, "sample-size", &sample_size, NULL);
  width = GTK_WIDGET (editor->sample_canvas)->allocation.width;
  zoom = (double)sample_size / width;

  if (!editor->tracks)    /* if no samples have been added yet */
    editor->sample_size = sample_size; /* cache sample size */

  track_info = g_new (TrackInfo, 1);
  track_info->sample = g_object_ref (sample); /* ++ ref sample */
  track_info->right_chan = right_chan;

  /* create sample view horizontal center line */
  track_info->sample_center_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->sample_canvas),
			     GNOME_TYPE_CANVAS_LINE,
			     "fill-color-rgba", editor->center_line_color,
			     "width-pixels", 1,
			     NULL);
  track_info->sample_view =  /* create the sample view canvas item */
    gnome_canvas_item_new (gnome_canvas_root (editor->sample_canvas),
			   SWAMIGUI_TYPE_SAMPLE_CANVAS,
			   "sample", sample,
			   "right-chan", right_chan,
			   "zoom", zoom,

			   /* only the first sample item will update adj. */
			   "update-adj", editor->tracks ? FALSE : TRUE,
			   "adjustment", gtk_range_get_adjustment
			   (GTK_RANGE (editor->hscrollbar)),
			   NULL);

  /* create loop view horizontal center line */
  track_info->loop_center_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->loop_canvas),
			     GNOME_TYPE_CANVAS_LINE,
     			     "fill-color-rgba", editor->center_line_color,
			     "width-pixels", 1,
			     NULL);
  track_info->loop_view =	/* create the loop view canvas item */
    gnome_canvas_item_new (gnome_canvas_root (editor->loop_canvas),
			   SWAMIGUI_TYPE_SAMPLE_CANVAS,
			   "sample", sample,
			   "right-chan", right_chan,
			   "loop-mode", TRUE,
			   "zoom", editor->loop_zoom,
			   NULL);

  gnome_canvas_item_lower_to_bottom (track_info->sample_center_line);
  gnome_canvas_item_lower_to_bottom (track_info->loop_center_line);

  gnome_canvas_item_lower_to_bottom (track_info->sample_view);
  gnome_canvas_item_lower_to_bottom (track_info->loop_view);

  gnome_canvas_item_lower_to_bottom (editor->loop_line);

  /* add track to track list */
  editor->tracks = g_list_append (editor->tracks, track_info);


  /* create loop view "loop-start" property control and connect to hub */
  ctrl = SWAMI_CONTROL (swami_get_control_prop_by_name /* ++ ref */
			(G_OBJECT (track_info->loop_view), "loop-start"));
  swami_control_connect (editor->loop_start_hub, ctrl, 0); /* hub is queued */
  g_object_unref (ctrl);	/* -- unref */

  /* create loop view "loop-end" property control and connect to hub */
  ctrl = SWAMI_CONTROL (swami_get_control_prop_by_name /* ++ ref */
			(G_OBJECT (track_info->loop_view), "loop-end"));
  swami_control_connect (editor->loop_end_hub, ctrl, 0); /* hub is queued */
  g_object_unref (ctrl);	/* -- unref */

  swamigui_sample_editor_update_sizes (editor);

  return (g_list_length (editor->tracks) - 1);
}

/**
 * swamigui_sample_editor_get_track_info:
 * @editor: Sample editor object
 * @track: Track index to get info from (starting from 0)
 * @sample: Output - Sample object (%NULL to ignore)
 * @sample_view: Output - The sample view canvas (%NULL to ignore)
 * @loop_view: Output - The loop view canvas (%NULL to ignore)
 *
 * Get info for a sample track. No reference counting is done, since
 * track will not get removed unless the sample editor handler does
 * so. Returned object pointers should only be used within editor
 * callback or references should be added.
 *
 * Returns: %TRUE if @index is a valid sample index, %FALSE otherwise in
 * which case the returned pointers are undefined.
 */
gboolean
swamigui_sample_editor_get_track_info (SwamiguiSampleEditor *editor,
				       guint track,
				       IpatchSampleData **sample,
				       SwamiguiSampleCanvas **sample_view,
				       SwamiguiSampleCanvas **loop_view)
{
  TrackInfo *track_info;

  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor), FALSE);

  track_info = (TrackInfo *)g_list_nth_data (editor->tracks, track);
  if (!track_info) return (FALSE);

  if (sample) *sample = track_info->sample;
  if (sample_view)
    *sample_view = SWAMIGUI_SAMPLE_CANVAS (track_info->sample_view);
  if (loop_view) *loop_view = SWAMIGUI_SAMPLE_CANVAS (track_info->loop_view);

  return (TRUE);
}

/**
 * swamigui_sample_editor_remove_track:
 * @editor: Sample editor object
 * @track: Index of track to remove
 *
 * Remove a track from a sample editor by its index (starts from 0).
 */
void
swamigui_sample_editor_remove_track (SwamiguiSampleEditor *editor, guint track)
{
  GList *found_track;

  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  found_track = g_list_nth (editor->tracks, track);
  g_return_if_fail (found_track != NULL);

  swamigui_sample_editor_remove_track_item (editor, found_track, TRUE);

  swamigui_sample_editor_update_sizes (editor);
}

/* internal function for removing by track info list node, destroy flag is
   used by finalize (set to FALSE since no need to destroy GtkObjects) */
static void
swamigui_sample_editor_remove_track_item (SwamiguiSampleEditor *editor,
					  GList *p, gboolean destroy)
{
  TrackInfo *track_info;

  track_info = (TrackInfo *)(p->data);
  g_object_unref (track_info->sample); /* -- unref sample */

  if (destroy)
    {
      /* destroy sample canvas items */
      gtk_object_destroy (GTK_OBJECT (track_info->sample_view));
      gtk_object_destroy (GTK_OBJECT (track_info->loop_view));
    
      /* destroy horizontal center lines */
      gtk_object_destroy (GTK_OBJECT (track_info->sample_center_line));
      gtk_object_destroy (GTK_OBJECT (track_info->loop_center_line));
    }

  g_free (track_info);

  /* if removing the first track and there are others.. */
  if (p == editor->tracks && editor->tracks->next)
    {
      track_info = (TrackInfo *)(editor->tracks->next->data);

      /* the new first sample canvas item now is adjustment master */
      g_object_set (track_info->sample_view, "update-adj", TRUE, NULL);
    }

  /* remove from list */
  editor->tracks = g_list_delete_link (editor->tracks, p);
}

/**
 * swamigui_sample_editor_remove_all_tracks:
 * @editor: Sample editor object
 *
 * Remove all tracks from a sample editor.
 */
void
swamigui_sample_editor_remove_all_tracks (SwamiguiSampleEditor *editor)
{
  GList *p, *temp;

  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  p = editor->tracks;
  while (p)
    {
      temp = p;
      p = g_list_next (p);
      swamigui_sample_editor_remove_track_item (editor, temp, TRUE);
    }
}

/**
 * swamigui_sample_editor_add_marker:
 * @editor: Sample editor object
 * @flags: #SwamiguiSampleEditorMarkerFlags
 *   - #SWAMIGUI_SAMPLE_EDITOR_MARKER_SINGLE is used for markers which define
 *   a single value.
 *   - #SWAMIGUI_SAMPLE_EDITOR_MARKER_VIEW is used for markers which are view
 *   only and not controllable by user.
 *   - #SWAMIGUI_SAMPLE_EDITOR_MARKER_SIZE is used to define a start/size
 *   marker instead of a start/end marker.  This causes the second control to
 *   be the current size of the marker instead of the end position.
 *   Note that passing 0 defines a interactive range marker.
 * @start: Output - New control for start of marker or %NULL to ignore
 * @end: Output - New control for end/size of marker (ranges only) or %NULL to
 *   ignore
 * 
 * Add a new marker to a sample editor. Markers can be used for loop points,
 * sample start/end points, selections and other position indicators/controls.
 * Marker 0 is the sample selection marker and it is always present although it
 * may be hidden.
 * 
 * Returns: New marker index
 */
guint
swamigui_sample_editor_add_marker (SwamiguiSampleEditor *editor,
				   guint flags, SwamiControl **start,
				   SwamiControl **end)
{
  MarkerInfo *mark_info;
  guint color;
  int id;

  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor), 0);

  mark_info = g_new0 (MarkerInfo, 1);

  mark_info->flags = flags;
  mark_info->editor = editor;
  mark_info->visible = TRUE;

  id = g_list_length (editor->markers);
  color = default_marker_colors [id % G_N_ELEMENTS (default_marker_colors)];

  mark_info->start_line
    = gnome_canvas_item_new (gnome_canvas_root (editor->sample_canvas),
			     GNOME_TYPE_CANVAS_LINE,
			     "fill-color-rgba", color,
			     "width-pixels", MARKER_DEFAULT_WIDTH,
			     NULL);
  gnome_canvas_item_hide (mark_info->start_line);

  /* create start marker control (!! editor takes over ref) */
  mark_info->start_ctrl = swamigui_control_new (SWAMI_TYPE_CONTROL_FUNC);
  swami_control_set_value_type (mark_info->start_ctrl, G_TYPE_UINT);
  swami_control_func_assign_funcs (SWAMI_CONTROL_FUNC (mark_info->start_ctrl),
				   start_marker_control_get_value_func,
				   start_marker_control_set_value_func,
				   NULL, mark_info);

  /* create end line and range box if a range marker */
  if (!(flags & SWAMIGUI_SAMPLE_EDITOR_MARKER_SINGLE))
    {
      mark_info->end_line
	= gnome_canvas_item_new (gnome_canvas_root (editor->sample_canvas),
				 GNOME_TYPE_CANVAS_LINE,
				 "fill-color-rgba", color,
				 "width-pixels", MARKER_DEFAULT_WIDTH,
				 NULL);
      gnome_canvas_item_hide (mark_info->end_line);

      mark_info->range_box
	= gnome_canvas_item_new (gnome_canvas_root (editor->sample_canvas),
				 GNOME_TYPE_CANVAS_RECT,
				 "fill-color-rgba", color,
				 NULL);
      gnome_canvas_item_hide (mark_info->range_box);

      /* create end/size marker control (!! editor takes over ref) */
      mark_info->end_ctrl = swamigui_control_new (SWAMI_TYPE_CONTROL_FUNC);
      swami_control_set_value_type (mark_info->end_ctrl, G_TYPE_UINT);
      swami_control_func_assign_funcs (SWAMI_CONTROL_FUNC (mark_info->end_ctrl),
				       end_marker_control_get_value_func,
				       end_marker_control_set_value_func,
				       NULL, mark_info);
    }

  editor->markers = g_list_append (editor->markers, mark_info);

  marker_control_update_all (editor);

  if (start) *start = mark_info->start_ctrl;
  if (end) *end = mark_info->end_ctrl;

  return (id);
}

/**
 * swamigui_sample_editor_get_marker_info:
 * @editor: Sample editor object
 * @marker: Marker index to get info on (starts at 0 - the selection marker)
 * @flags: #SwamiguiSampleEditorMarkerFlags
 * @start_line: Output - Gnome canvas line for the start marker (%NULL to ignore)
 * @end_line: Output - Gnome canvas line for the end marker (%NULL to ignore),
 *   will be %NULL for single markers (non ranges)
 * @start_ctrl: Output - Control for the start marker (%NULL to ignore)
 * @end_ctrl: Output - Control for the end/size marker (%NULL to ignore), will be
 *   %NULL for single markers (non ranges)
 * 
 * Get info for the given @marker index. No reference counting is
 * done, since marker will not get removed unless the sample editor
 * handler does so. Returned object pointers should only be used
 * within editor callback or references should be added.
 * 
 * Returns: %TRUE if a marker with the given index exists, %FALSE otherwise.
 */
gboolean
swamigui_sample_editor_get_marker_info (SwamiguiSampleEditor *editor,
					guint marker, guint *flags,
					GnomeCanvasItem **start_line,
					GnomeCanvasItem **end_line,
					SwamiControl **start_ctrl,
					SwamiControl **end_ctrl)
{
  MarkerInfo *marker_info;

  g_return_val_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor), FALSE);

  marker_info = (MarkerInfo *)g_list_nth_data (editor->markers, marker);
  if (!marker_info) return (FALSE);

  if (flags) *flags = marker_info->flags;
  if (start_line) *start_line = marker_info->start_line;
  if (end_line) *end_line = marker_info->end_line;
  if (start_ctrl) *start_ctrl = marker_info->start_ctrl;
  if (end_ctrl) *end_ctrl = marker_info->end_ctrl;

  return (TRUE);
}

/**
 * swamigui_sample_editor_set_marker:
 * @editor: Sample editor widget
 * @marker: Marker number
 * @start: Marker start position
 * @end: Marker end position
 *
 * Set the marker start and end positions.
 */
void
swamigui_sample_editor_set_marker (SwamiguiSampleEditor *editor, guint marker,
				   guint start, guint end)
{
  MarkerInfo *marker_info;

  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  marker_info = (MarkerInfo *)g_list_nth_data (editor->markers, marker);
  if (!marker_info) return;

  swamigui_sample_editor_set_marker_info (editor, marker_info, start, end);
}

static void
swamigui_sample_editor_set_marker_info (SwamiguiSampleEditor *editor,
					MarkerInfo *marker_info,
					guint start, guint end)
{
  GValue value = { 0 };
  gboolean size_marker;
  gboolean start_changed = FALSE;
  guint temp;

  if (start > end)	/* swap if backwards */
  {
    temp = start;
    start = end;
    end = temp;
  }

  size_marker = (marker_info->flags & SWAMIGUI_SAMPLE_EDITOR_MARKER_SIZE) != 0;

  g_value_init (&value, G_TYPE_UINT);

  if (start != marker_info->start_pos)
  {
    marker_info->start_pos = start;
    g_value_set_uint (&value, start);
    swami_control_transmit_value (marker_info->start_ctrl, &value);
    start_changed = TRUE;
  }

  if (end != marker_info->end_pos || (size_marker && start_changed))
  {
    marker_info->end_pos = end;

    if (size_marker)
      g_value_set_uint (&value, end - start + 1);
    else g_value_set_uint (&value, end);

    swami_control_transmit_value (marker_info->end_ctrl, &value);
  }

  g_value_unset (&value);
  marker_control_update (marker_info);	/* update the marker GUI */
}

/**
 * swamigui_sample_editor_remove_marker:
 * @editor: Sample editor object
 * @marker: Index of marker to remove (builtin markers are hidden rather than
 *   removed)
 * 
 * Remove a marker.
 */
void
swamigui_sample_editor_remove_marker (SwamiguiSampleEditor *editor,
				      guint marker)
{
  GList *p;

  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  if (marker < BUILTIN_MARKER_COUNT)
  {
    swamigui_sample_editor_show_marker (editor, marker, FALSE);
    return;
  }

  p = g_list_nth (editor->markers, marker);
  if (!p) return;

  swamigui_sample_editor_remove_marker_item (editor, p, TRUE);
}

/**
 * swamigui_sample_editor_remove_all_markers:
 * @editor: Sample editor object
 * 
 * Remove all markers (except selection marker 0, which is always present).
 */
void
swamigui_sample_editor_remove_all_markers (SwamiguiSampleEditor *editor)
{
  GList *p, *temp;
  int i;

  p = g_list_nth (editor->markers, BUILTIN_MARKER_COUNT);
  while (p)
    {
      temp = p;
      p = g_list_next (p);
      swamigui_sample_editor_remove_marker_item (editor, temp, TRUE);
    }

  /* hide builtin markers */
  for (i = 0; i < BUILTIN_MARKER_COUNT; i++)
    swamigui_sample_editor_show_marker (editor, i, FALSE);
}

/* remove a marker, destroy flag is used by finalize function since GtkObjects
   dont need to be destroyed on finalize */
static void
swamigui_sample_editor_remove_marker_item (SwamiguiSampleEditor *editor,
					   GList *p, gboolean destroy)
{
  MarkerInfo *marker_info;
  SwamiControl *ctrl;

  marker_info = (MarkerInfo *)(p->data);

  /* remove from marker list */
  editor->markers = g_list_delete_link (editor->markers, p);

  /* destroy line canvas item */
  if (destroy)
  {
    gtk_object_destroy (GTK_OBJECT (marker_info->start_line));

    if (marker_info->end_line)
      gtk_object_destroy (GTK_OBJECT (marker_info->end_line));

    if (marker_info->range_box)
      gtk_object_destroy (GTK_OBJECT (marker_info->range_box));
  }

  /* we don't want to depend on the existence of mark_info anymore */
  ctrl = marker_info->end_ctrl;		/* FIXME - What about start_ctrl !!! */

  /* clear data in marker info */
  memset (marker_info, 0, sizeof (MarkerInfo));

  /* add a week references to the marker controls to free the marker_info
     structures (since we still might receive queued events) */
  g_object_weak_ref (G_OBJECT (ctrl), (GWeakNotify)g_free, marker_info);
  g_object_unref (ctrl);	/* -- remove the editor's reference */
}

/* start marker control get value method */
static void
start_marker_control_get_value_func (SwamiControl *control, GValue *value)
{
  MarkerInfo *marker_info = SWAMI_CONTROL_FUNC_DATA (control);
  g_value_set_uint (value, marker_info->start_pos);
}

/* start marker control set value method */
static void
start_marker_control_set_value_func (SwamiControl *control,
				     SwamiControlEvent *event,
				     const GValue *value)
{
  MarkerInfo *marker_info = SWAMI_CONTROL_FUNC_DATA (control);
  guint u;

  if (!marker_info->start_line) return;	/* in process of being destroyed? */

  u = g_value_get_uint (value);
  if (u != marker_info->start_pos)
    {
      marker_info->start_pos = u;
      marker_control_update (marker_info);
    }
}

/* end marker control get value method */
static void
end_marker_control_get_value_func (SwamiControl *control, GValue *value)
{
  MarkerInfo *marker_info = SWAMI_CONTROL_FUNC_DATA (control);

  if (marker_info->flags & SWAMIGUI_SAMPLE_EDITOR_MARKER_SIZE)
    g_value_set_uint (value, marker_info->end_pos - marker_info->start_pos + 1);
  else g_value_set_uint (value, marker_info->end_pos);
}

/* end marker control set value method */
static void
end_marker_control_set_value_func (SwamiControl *control,
				   SwamiControlEvent *event,
				   const GValue *value)
{
  MarkerInfo *marker_info = SWAMI_CONTROL_FUNC_DATA (control);
  guint u;

  if (!marker_info->end_line) return;	/* in process of being destroyed? */

  u = g_value_get_uint (value);

  if (marker_info->flags & SWAMIGUI_SAMPLE_EDITOR_MARKER_SIZE)
    u += marker_info->start_pos - 1;

  if (u != marker_info->end_pos && u >= marker_info->start_pos)
    {
      marker_info->end_pos = u;
      marker_control_update (marker_info);
    }
}

/* update the display of a single marker in the sample view */
static void
marker_control_update (MarkerInfo *marker_info)
{
  SwamiguiSampleEditor *editor = marker_info->editor;
  SwamiguiSampleCanvas *sample_view = NULL;
  TrackInfo *track_info;
  GnomeCanvasPoints *points;
  int startx, endx, sview = -1, eview = -1;
  int width, height;
  int visible_count, pos;
  int y1 = 0, y2 = 0;
  GList *p;

  /* can't position marker lines if no sample canvases, exception is if marker
     is hidden, then we don't need to update the position */
  if (!editor->tracks && marker_info->visible) return;

  /* count number of visible markers and locate visible position of this marker */
  for (p = editor->markers, visible_count = 0; p; p = p->next)
  {
    if (p->data == marker_info) pos = visible_count;
    if (((MarkerInfo *)(p->data))->visible) visible_count++;
  }

  pos = visible_count - pos - 1;	/* adjust for Y-coord order */

  if (editor->tracks)
  {
    track_info = (TrackInfo *)(editor->tracks->data);
    sample_view = SWAMIGUI_SAMPLE_CANVAS (track_info->sample_view);
  }

  width = GTK_WIDGET (editor->sample_canvas)->allocation.width;
  height = GTK_WIDGET (editor->sample_canvas)->allocation.height;

  /* calculate Y positions of range bar */
  if (visible_count > 0)
  {
    y1 = (pos * (editor->marker_bar_height - 1)) / visible_count;
    y2 = ((pos+1) * (editor->marker_bar_height - 1)) / visible_count - 1;
  }

  points = gnome_canvas_points_new (2);
  points->coords[1] = y2;
  points->coords[3] = height;

  /* update start marker line */
  if (sample_view)
    startx = swamigui_sample_canvas_sample_to_xpos (sample_view,
					      marker_info->start_pos, &sview);
  if (sample_view && sview == 0 && marker_info->visible)
  {
    points->coords[0] = startx;
    points->coords[2] = startx;

    g_object_set (marker_info->start_line, "points", points, NULL);
    gnome_canvas_item_show (marker_info->start_line);
  }
  else
  {
    gnome_canvas_item_hide (marker_info->start_line);
    startx = 0;
  }

  /* update end marker and range box (if any) */
  if (marker_info->end_line)
  {
    if (sample_view)
      endx = swamigui_sample_canvas_sample_to_xpos (sample_view,
						  marker_info->end_pos, &eview);
    if (sample_view && eview == 0 && marker_info->visible)
    {
      points->coords[0] = endx;
      points->coords[2] = endx;
      endx++;	/* for marker range_box below */

      g_object_set (marker_info->end_line, "points", points, NULL);
      gnome_canvas_item_show (marker_info->end_line);
    }
    else
    {
      gnome_canvas_item_hide (marker_info->end_line);
      endx = width;
    }

    /* is marker range box in view? */
    if (sample_view && (sview == 0 || eview == 0 || (sview == -1 && eview == 1))
	&& marker_info->visible)
    {
      g_object_set (marker_info->range_box,
		    "x1", (double)startx,
		    "x2", (double)endx,
		    "y1", (double)y1,
		    "y2", (double)y2,
		    NULL);

      gnome_canvas_item_show (marker_info->range_box);
    }
    else gnome_canvas_item_hide (marker_info->range_box);
  }

  gnome_canvas_points_free (points);
}

static void
marker_control_update_all (SwamiguiSampleEditor *editor)
{
  MarkerInfo *marker_info;
  GList *p;

  for (p = editor->markers; p; p = g_list_next (p))
    {
      marker_info = (MarkerInfo *)(p->data);
      marker_control_update (marker_info);
    }
}

/**
 * swamigui_sample_editor_show_marker:
 * @editor: Sample editor widget
 * @marker: Marker number to set visibility of
 * @show_marker: %TRUE to show marker, %FALSE to hide
 *
 * Set the visibility of a marker.
 */
void
swamigui_sample_editor_show_marker (SwamiguiSampleEditor *editor,
				    guint marker, gboolean show_marker)
{
  MarkerInfo *marker_info;
  GList *p;

  g_return_if_fail (SWAMIGUI_IS_SAMPLE_EDITOR (editor));

  p = g_list_nth (editor->markers, marker);
  if (!p) return;

  marker_info = (MarkerInfo *)(p->data);
  if (marker_info->visible == show_marker) return;

  marker_info->visible = show_marker;
  marker_control_update_all (editor);
}

/**
 * swamigui_sample_editor_set_loop_types:
 * @editor: Sample editor object
 * @types: -1 terminated array of #IpatchSampleLoopType values to show in
 *   loop selector (%NULL to hide loop selector)
 * @loop_play_btn: If @types is %NULL, setting this to %TRUE will cause a
 *   play loop toggle button to be shown (for items that dont have a loop type
 *   property, but it is desirable for the user to be able to listen to the loop).
 *
 * Usually only used by sample editor handlers.  Sets the available loop types
 * in the loop selector menu.
 */
void
swamigui_sample_editor_set_loop_types (SwamiguiSampleEditor *editor, int *types,
				       gboolean loop_play_btn)
{
  GtkTreeIter iter;
  int i, i2;

  gtk_list_store_clear (editor->loopsel_store);

  if (types)
    {
      for (i = 0; i < G_N_ELEMENTS (loop_type_info); i++)
	{
	  for (i2 = 0; types[i2] != -1; i2++)
	    {
	      if (loop_type_info[i].loop_type == types[i2])
		{
		  gtk_list_store_append (editor->loopsel_store, &iter);
		  gtk_list_store_set (editor->loopsel_store, &iter,
				      LOOPSEL_COL_LOOP_TYPE, types[i2],
				      LOOPSEL_COL_ICON, loop_type_info[i].icon,
				      LOOPSEL_COL_LABEL, loop_type_info[i].label,
				      LOOPSEL_COL_TOOLTIP, loop_type_info[i].tooltip,
				      -1);
		}
	    }
	}

      gtk_widget_set_sensitive (editor->loopsel, TRUE);
    }
  else gtk_widget_set_sensitive (editor->loopsel, FALSE);
}

/**
 * swamigui_sample_editor_set_active_loop_type:
 * @editor: Sample editor widget
 * @type: #IpatchSampleLoopType or TRUE/FALSE if using loop play button.
 *
 * Set the active loop type in the loop type selector or loop play button.
 * Usually only used by sample editor handlers.
 */
void
swamigui_sample_editor_set_active_loop_type (SwamiguiSampleEditor *editor,
					     int type)
{
  GtkTreeModel *model = GTK_TREE_MODEL (editor->loopsel_store);
  GtkTreeIter iter;
  int cmptype;

  if (!gtk_tree_model_get_iter_first (model, &iter))
      return;

  do
    {
      gtk_tree_model_get (model, &iter, LOOPSEL_COL_LOOP_TYPE, &cmptype, -1);
      if (cmptype == type)
	{
	  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (editor->loopsel), &iter);
	  break;
	}
    }
  while (gtk_tree_model_iter_next (model, &iter));
}

/* Default IpatchSample interface sample editor handler */
static gboolean
swamigui_sample_editor_default_handler (SwamiguiSampleEditor *editor)
{
  IpatchSample *sample;
  IpatchSampleData *sampledata;
  SwamiControl *loop_view_start, *loop_view_end;
  SwamiControl *mark_loop_start, *mark_loop_end;
  SwamiControl *loop_start_prop, *loop_end_prop, *loop_type_prop;
  int *loop_types;
  GObject *obj;

  /* check if selection is handled (single item with IpatchSample interface) */
  if (editor->status == SWAMIGUI_SAMPLE_EDITOR_INIT
      || editor->status == SWAMIGUI_SAMPLE_EDITOR_UPDATE)
  {
    /* only handle single item with IpatchSample interface */
    if (!editor->selection || !editor->selection->items
	|| editor->selection->items->next)
      return (FALSE);

    obj = editor->selection->items->data;
    if (!IPATCH_IS_SAMPLE (obj)) return (FALSE);

    sample = IPATCH_SAMPLE (obj);

    /* same item already selected? */
    if ((gpointer)sample == (gpointer)(editor->handler_data))
      return (TRUE);

    sampledata = ipatch_sample_get_sample_data (sample);        /* ++ ref sample data */

    /* clear all tracks and markers if updating and a new item is selected */
    if (editor->status == SWAMIGUI_SAMPLE_EDITOR_UPDATE)
      swamigui_sample_editor_reset (editor);

    editor->handler_data = sample; /* set handler data to selected item */

    /* clear loop finder results */
    swamigui_loop_finder_clear_results (editor->loop_finder_gui);

    /* stereo sample data?? */
    if (IPATCH_SAMPLE_FORMAT_GET_CHANNELS (ipatch_sample_get_format (sample))
        == IPATCH_SAMPLE_STEREO)
    {
      swamigui_sample_editor_add_track (editor, sampledata, FALSE);
      swamigui_sample_editor_add_track (editor, sampledata, TRUE);
    }
    else			/* mono data */
      swamigui_sample_editor_add_track (editor, sampledata, FALSE);

    /* set sample of loop finder */
    g_object_set (editor->loop_finder_gui->loop_finder, "sample", sample, NULL);

    /* create loop range marker */
    swamigui_sample_editor_add_marker (editor, 0, &mark_loop_start,
				       &mark_loop_end);

    /* ++ ref sample loop property controls */
    loop_start_prop = swami_get_control_prop_by_name (G_OBJECT (sample), "loop-start");
    loop_end_prop = swami_get_control_prop_by_name (G_OBJECT (sample), "loop-end");

    /* connect sample properties to loop markers */
    swami_control_connect (loop_start_prop, mark_loop_start,
			   SWAMI_CONTROL_CONN_BIDIR | SWAMI_CONTROL_CONN_INIT);
    swami_control_connect (loop_end_prop, mark_loop_end,
			   SWAMI_CONTROL_CONN_BIDIR | SWAMI_CONTROL_CONN_INIT);

    /* connect sample properties to spin buttons */
    swami_control_connect (loop_start_prop, editor->spinbtn_start_ctrl,
			   SWAMI_CONTROL_CONN_BIDIR | SWAMI_CONTROL_CONN_INIT);
    swami_control_connect (loop_end_prop, editor->spinbtn_end_ctrl,
			   SWAMI_CONTROL_CONN_BIDIR | SWAMI_CONTROL_CONN_INIT);

    /* get loop view controls */
    swamigui_sample_editor_get_loop_controls (editor, &loop_view_start,
					      &loop_view_end);

    /* connect sample properties to loop view controls */
    swami_control_connect (loop_start_prop, loop_view_start,
			   SWAMI_CONTROL_CONN_INIT);
    swami_control_connect (loop_end_prop, loop_view_end,
			   SWAMI_CONTROL_CONN_INIT);

    /* unref sample property controls */
    g_object_unref (loop_start_prop);
    g_object_unref (loop_end_prop);

    /* set the available loop types of the loop selector */
    loop_types = ipatch_sample_get_loop_types (sample);
    swamigui_sample_editor_set_loop_types (editor, loop_types, FALSE);

    if (loop_types)
    { /* ++ ref new prop control */
      loop_type_prop = swami_get_control_prop_by_name (G_OBJECT (sample), "loop-type");
      swami_control_connect (loop_type_prop, editor->loopsel_ctrl,
			     SWAMI_CONTROL_CONN_BIDIR
			     | SWAMI_CONTROL_CONN_INIT);
      g_object_unref (loop_type_prop);	/* -- unref prop control */
    }

    /* show finder markers (if finder enabled) */
    if (editor->loop_finder_active)
    {
      swamigui_sample_editor_show_marker (editor,
		    SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_START, TRUE);
      swamigui_sample_editor_show_marker (editor,
		    SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_END, TRUE);
    }

    g_object_unref (sampledata);        /* -- unref sample data */
  } /* if INIT or UPDATE */

  return (TRUE);
}

static gboolean
swamigui_sample_editor_default_handler_check_func (IpatchList *selection,
						   GType *selection_types)
{
  IpatchSampleData *sampledata;

  /* only handle single item with IpatchSample interface which returns sample data */
  if (selection->items->next
      || !g_type_is_a (*selection_types, IPATCH_TYPE_SAMPLE)
      || !(sampledata = ipatch_sample_get_sample_data   /* ++ ref sample data */
           ((IpatchSample *)(selection->items->data))))
    return (FALSE);

  g_object_unref (sampledata);          /* -- unref sample data */

  return (TRUE);
}

/* get value function for loopsel_ctrl */
static void
swamigui_sample_editor_loopsel_ctrl_get (SwamiControl *control, GValue *value)
{
  SwamiguiSampleEditor *editor =
    SWAMIGUI_SAMPLE_EDITOR (SWAMI_CONTROL_FUNC_DATA (control));
  GtkTreeIter iter;
  int loop_type = 0;

  /* get active loop type or failing that get first loop type */
  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (editor->loopsel), &iter)
      || gtk_tree_model_get_iter_first (GTK_TREE_MODEL (editor->loopsel_store),
					&iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (editor->loopsel_store), &iter,
			  LOOPSEL_COL_LOOP_TYPE, &loop_type, -1);
    }

  g_value_set_enum (value, loop_type);
}

/* set value function for loopsel_ctrl */
static void
swamigui_sample_editor_loopsel_ctrl_set (SwamiControl *control,
					 SwamiControlEvent *event,
					 const GValue *value)
{
  SwamiguiSampleEditor *editor =
    SWAMIGUI_SAMPLE_EDITOR (SWAMI_CONTROL_FUNC_DATA (control));
  int loop_type;

  loop_type = g_value_get_enum (value);

  /* block changed signal while setting the combo box selection */
  g_signal_handlers_block_by_func (editor->loopsel,
			    swamigui_sample_editor_loopsel_cb_changed, editor);

  swamigui_sample_editor_set_active_loop_type (editor, loop_type);

  g_signal_handlers_unblock_by_func (editor->loopsel,
			    swamigui_sample_editor_loopsel_cb_changed, editor);
}

/* callback for when the loop selector combo box is changed */
static void
swamigui_sample_editor_loopsel_cb_changed (GtkComboBox *widget,
					   gpointer user_data)
{
  SwamiguiSampleEditor *editor = SWAMIGUI_SAMPLE_EDITOR (user_data);
  GtkTreeIter iter;
  int loop_type;
  GValue value = { 0 };

  if (gtk_combo_box_get_active_iter (widget, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (editor->loopsel_store), &iter,
			  LOOPSEL_COL_LOOP_TYPE, &loop_type, -1);

      /* transmit the loop type change */
      g_value_init (&value, G_TYPE_INT);
      g_value_set_int (&value, loop_type);
      swami_control_transmit_value ((SwamiControl *)editor->loopsel_ctrl,
				    &value);
      g_value_unset (&value);
    }
}
