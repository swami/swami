/*
 * SwamiguiSplits.c - Key/velocity splits widget
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

#include <libinstpatch/libinstpatch.h>

#include "SwamiguiSplits.h"
#include "SwamiguiControl.h"
#include "SwamiguiPiano.h"
#include "SwamiguiRoot.h"
#include "SwamiguiStatusbar.h"
#include "icons.h"
#include "i18n.h"
#include "util.h"

enum
{
  PROP_0,
  PROP_ITEM_SELECTION,
  PROP_SPLITS_ITEM,
  PROP_PIANO
};

/* Value used for active_drag field in SwamiguiSplits which indicates the
 * current drag mode */
typedef enum
{
  ACTIVE_NONE,		/* Inactive drag */
  ACTIVE_LOW,		/* Dragging low handle of span */
  ACTIVE_HIGH,		/* Dragging upper handle of span */
  ACTIVE_UNDECIDED,	/* Not yet decided which handle of span to drag */
  ACTIVE_MOVE_ROOTNOTES, /* Moving root note(s) */
  ACTIVE_MOVE_RANGES,	/* Dragging note/velocity range(s) */
  ACTIVE_MOVE_BOTH      /* Moving both root notes and range(s) */
} ActiveDrag;

/* min/max width of piano/splits in pixels */
#define MIN_SPLITS_WIDTH  SWAMIGUI_PIANO_DEFAULT_WIDTH
#define MAX_SPLITS_WIDTH  2400

#define SPAN_DEFAULT_HEIGHT 12	/* span handle height in pixels */
#define SPAN_DEFAULT_SPACING 3	/* vertical spacing between spans in pixels */
#define MOVEMENT_THRESHOLD 3 /* pixels of mouse movement till threshold */

#define SPLIT_IS_SELECTED(entry)  (((entry)->flags & SPLIT_SELECTED) != 0)

/* default colors */
#define DEFAULT_BG_COLOR		GNOME_CANVAS_COLOR (255, 255, 178)
#define DEFAULT_SPAN_COLOR		GNOME_CANVAS_COLOR (0, 252, 113)
#define DEFAULT_SPAN_OUTLINE_COLOR	GNOME_CANVAS_COLOR (0, 0, 0)
#define DEFAULT_SPAN_SEL_COLOR		GNOME_CANVAS_COLOR (255, 13, 53)
#define DEFAULT_SPAN_SEL_OUTLINE_COLOR	DEFAULT_SPAN_SEL_COLOR
#define DEFAULT_LINE_COLOR		DEFAULT_SPAN_OUTLINE_COLOR
#define DEFAULT_LINE_SEL_COLOR		DEFAULT_SPAN_SEL_OUTLINE_COLOR
#define DEFAULT_ROOT_NOTE_COLOR		GNOME_CANVAS_COLOR (80, 80, 255)


/* some flags for each split */
enum
{
  SPLIT_SELECTED       = 1 << 0, /* span is selected? */
};

/* structure for a single split */
struct _SwamiguiSplitsEntry
{
  SwamiguiSplits *splits;		/* parent split object */
  int index;				/* index of this entry in splits->entry_list */
  GObject *item;			/* item of this split */
  IpatchRange range;	 		/* current span range (MIDI note/velocity) */
  guint rootnote_val;			/* current root note number (if active) */
  SwamiControl *span_control;		/* span range control for this split or NULL */
  SwamiControl *rootnote_control;	/* root note control for this split or NULL */
  gboolean destroyed; 		/* set to TRUE when a split has been destroyed */
  int refcount;	/* refcount of structure (held by span_control and rootnote_control) */

  GnomeCanvasItem *span;	/* span canvas item (GnomeCanvasRect) */
  GnomeCanvasItem *lowline;	/* low range endpoint vertical line */
  GnomeCanvasItem *highline;	/* high range endpoint vertical line */
  GnomeCanvasItem *rootnote;	/* root note circle indicator (GnomeCanvasEllipse) */
  int flags;			/* flags */
};

static void swamigui_splits_class_init (SwamiguiSplitsClass *klass);
static void splits_cb_mode_btn_clicked (GtkButton *button, gpointer user_data);
static void swamigui_splits_cb_canvas_size_allocate (GtkWidget *widget,
						     GtkAllocation *allocation,
						     gpointer user_data);
static void swamigui_splits_set_property (GObject *object, guint property_id,
					  const GValue *value,
					  GParamSpec *pspec);
static void swamigui_splits_get_property (GObject *object, guint property_id,
					  GValue *value, GParamSpec *pspec);
static void swamigui_splits_destroy (GtkObject *object);
static void swamigui_splits_init (SwamiguiSplits *splits);
static gboolean swamigui_splits_cb_low_canvas_event (GnomeCanvasItem *item,
						     GdkEvent *event,
						     gpointer data);
static GList *swamigui_splits_get_split_at_pos (SwamiguiSplits *splits,
						int x, int y, int *index);
static void swamigui_splits_update_status_bar (SwamiguiSplits *splits,
					       int low, int high);
static void swamigui_splits_span_control_get_func (SwamiControl *control,
						   GValue *value);
static void swamigui_splits_span_control_set_func (SwamiControl *control,
						   SwamiControlEvent *event,
						   const GValue *value);
static void swamigui_splits_span_control_destroy_func (SwamiControlFunc *control);
static void swamigui_splits_root_note_control_get_func (SwamiControl *control,
							GValue *value);
static void swamigui_splits_root_note_control_set_func (SwamiControl *control,
							SwamiControlEvent *event,
							const GValue *value);
static void swamigui_splits_root_note_control_destroy_func (SwamiControlFunc *control);

static void swamigui_splits_deactivate_handler (SwamiguiSplits *splits);
static SwamiguiSplitsEntry *swamigui_splits_create_entry (SwamiguiSplits *splits,
							  GObject *item);
static void swamigui_splits_destroy_entry (SwamiguiSplitsEntry *entry);
static GList *swamigui_splits_lookup_item (SwamiguiSplits *splits,
					   GObject *item);
static void swamigui_splits_update_item_sel (SwamiguiSplitsEntry *entry);
static void swamigui_splits_update_selection (SwamiguiSplits *splits);
static gboolean swamigui_splits_real_set_selection (SwamiguiSplits *splits,
						IpatchList *items);
static void swamigui_splits_update_entries (SwamiguiSplits *splits, GList *startp,
					    gboolean width_change,
					    gboolean height_change);
static void swamigui_splits_entry_set_span_control (SwamiguiSplitsEntry *entry,
						    int low, int high);
static void swamigui_splits_entry_set_span (SwamiguiSplitsEntry *entry,
					    int low, int high);
static void swamigui_splits_entry_set_root_note_control (SwamiguiSplitsEntry *entry,
							 int val);
static void swamigui_splits_entry_set_root_note (SwamiguiSplitsEntry *entry,
						 int val);
static gboolean swamigui_splits_default_handler (SwamiguiSplits *splits);
static GdkPixbuf *swamigui_splits_create_velocity_gradient (void);
static void gradient_data_free (guchar *pixels, gpointer data);


/* data */

static GObjectClass *parent_class = NULL;

/* we lock handlers list since they can be registered outside GUI thread */
G_LOCK_DEFINE_STATIC (handlers);
static GList *split_handlers = NULL;	/* list of SwamiguiSplitsHandlers */

/* start and end velocity gradient colors */
static guint8 velbar_scolor[3] = { 0, 0, 0 };
static guint8 velbar_ecolor[3] = { 0, 0, 255 };


GType
swamigui_splits_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static const GTypeInfo obj_info =
	{
	  sizeof (SwamiguiSplitsClass), NULL, NULL,
	  (GClassInitFunc) swamigui_splits_class_init, NULL, NULL,
	  sizeof (SwamiguiSplits), 0,
	  (GInstanceInitFunc) swamigui_splits_init,
	};

      obj_type = g_type_register_static (GTK_TYPE_VBOX, "SwamiguiSplits",
					 &obj_info, 0);
    }

  return (obj_type);
}

static void
swamigui_splits_class_init (SwamiguiSplitsClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtkobj_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = swamigui_splits_set_property;
  obj_class->get_property = swamigui_splits_get_property;

  gtkobj_class->destroy = swamigui_splits_destroy;

  g_object_class_install_property (obj_class, PROP_ITEM_SELECTION,
		g_param_spec_object ("item-selection", "Item selection",
				     "Item selection",
				     IPATCH_TYPE_LIST, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SPLITS_ITEM,
		g_param_spec_object ("splits-item", "Splits item",
				     "Splits item", IPATCH_TYPE_ITEM,
                                     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PIANO,
		g_param_spec_object ("piano", "Piano", "Piano",
				     SWAMIGUI_TYPE_PIANO, G_PARAM_READABLE));
}

static void
swamigui_splits_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
  SwamiguiSplits *splits = SWAMIGUI_SPLITS (object);
  IpatchList *items;
  GObject *obj;

  switch (property_id)
    {
    case PROP_ITEM_SELECTION:
      items = g_value_get_object (value);
      swamigui_splits_real_set_selection (splits, items);
      break;
    case PROP_SPLITS_ITEM:
      if (splits->splits_item) g_object_unref (splits->splits_item);
      obj = g_value_dup_object (value);
      splits->splits_item = obj ? IPATCH_ITEM (obj) : NULL;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_splits_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
  SwamiguiSplits *splits = SWAMIGUI_SPLITS (object);

  switch (property_id)
    {
    case PROP_ITEM_SELECTION:
      g_value_set_object (value, splits->selection);
      break;
    case PROP_SPLITS_ITEM:
      g_value_set_object (value, splits->splits_item);
      break;
    case PROP_PIANO:
      g_value_set_object (value, splits->piano);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_splits_destroy (GtkObject *object)
{
  SwamiguiSplits *splits = SWAMIGUI_SPLITS (object);
  SwamiguiSplitsEntry *entry;
  GList *p;

  /* unref objects in entries (entries are freed in control destroy
     callback, since control events might still occur and they depend on
     entry and splits widget) */
  p = splits->entry_list;
  while (p)
    {
      entry = (SwamiguiSplitsEntry *)(p->data);
      entry->destroyed = TRUE;
      if (entry->item) g_object_unref (entry->item);
      if (entry->span_control) swami_control_disconnect_unref (entry->span_control);
      if (entry->rootnote_control) swami_control_disconnect_unref (entry->rootnote_control);
      p = g_list_delete_link (p, p);
    }

  splits->entry_list = NULL;
  splits->entry_count = 0;

  /* free the selection */
  if (splits->selection)
    {
      g_object_unref (splits->selection);
      splits->selection = NULL;
    }

  /* deactivate the handler */
  splits->handler = NULL;
  splits->handler_data = NULL;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy)(object);
}

static void
swamigui_splits_init (SwamiguiSplits *splits)
{
  GtkWidget *gladewidg;
  GtkWidget *scrollwin;
  GtkWidget *widg;
  GtkAdjustment *hadj, *vadj;
  GtkStyle *style;
  GdkPixbuf *pixbuf;

  splits->anchor = -1;		/* no split selection anchor set */
  splits->active_drag = ACTIVE_NONE; /* no active click drag */

  /* set default size values */
  splits->height = SPAN_DEFAULT_HEIGHT;
  splits->width = -1;			/* Invalid width, to force update */
  splits->vert_lines_width = 1;
  splits->span_height = SPAN_DEFAULT_HEIGHT;
  splits->move_threshold = MOVEMENT_THRESHOLD;
  splits->span_spacing = SPAN_DEFAULT_SPACING;

  splits->bg_color = DEFAULT_BG_COLOR;
  splits->span_color = DEFAULT_SPAN_COLOR;
  splits->span_sel_color = DEFAULT_SPAN_SEL_COLOR;
  splits->span_outline_color = DEFAULT_SPAN_OUTLINE_COLOR;
  splits->span_sel_outline_color = DEFAULT_SPAN_SEL_OUTLINE_COLOR;
  splits->line_color = DEFAULT_LINE_COLOR;
  splits->line_sel_color = DEFAULT_LINE_SEL_COLOR;
  splits->root_note_color = DEFAULT_ROOT_NOTE_COLOR;

  splits->selection = ipatch_list_new (); /* ++ ref new list */

  gladewidg = swamigui_util_glade_create ("SwamiguiSplits");
  gtk_box_pack_start (GTK_BOX (splits), gladewidg, TRUE, TRUE, 0);

  splits->gladewidg = gladewidg;

  splits->notes_btn = swamigui_util_glade_lookup (gladewidg, "BtnNotes");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (splits->notes_btn), TRUE);
  g_signal_connect (splits->notes_btn, "clicked",
		    G_CALLBACK (splits_cb_mode_btn_clicked), splits);

  widg = swamigui_util_glade_lookup (gladewidg, "BtnNotesImage");
  gtk_image_set_from_stock (GTK_IMAGE (widg), SWAMIGUI_STOCK_PIANO,
			    GTK_ICON_SIZE_SMALL_TOOLBAR);

  splits->velocity_btn = swamigui_util_glade_lookup (gladewidg, "BtnVelocity");
  g_signal_connect (splits->velocity_btn, "clicked",
		    G_CALLBACK (splits_cb_mode_btn_clicked), splits);

  widg = swamigui_util_glade_lookup (gladewidg, "BtnVelocityImage");
  gtk_image_set_from_stock (GTK_IMAGE (widg), SWAMIGUI_STOCK_VELOCITY,
			    GTK_ICON_SIZE_SMALL_TOOLBAR);

  splits->vertical_scrollbar
    = swamigui_util_glade_lookup (gladewidg, "SplitsVScrollBar");
  vadj = gtk_range_get_adjustment (GTK_RANGE (splits->vertical_scrollbar));

  widg = swamigui_util_glade_lookup (gladewidg, "SplitsHScrollBar");
  hadj = gtk_range_get_adjustment (GTK_RANGE (widg));

  /* Set horizontal adjustment of upper scroll window to the horizontal scrollbar's */
  scrollwin = swamigui_util_glade_lookup (gladewidg, "SplitsScrollWinUpper");
  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (scrollwin), hadj);

  /* setup upper canvas */
  splits->top_canvas = gnome_canvas_new ();
  gtk_widget_show (splits->top_canvas);
  gtk_container_add (GTK_CONTAINER (scrollwin), splits->top_canvas);
  gnome_canvas_set_center_scroll_region (GNOME_CANVAS (splits->top_canvas), FALSE);
  gtk_widget_set_size_request (splits->top_canvas, -1,
			       SWAMIGUI_PIANO_DEFAULT_HEIGHT);
  g_signal_connect (splits->top_canvas, "size-allocate",
		    G_CALLBACK (swamigui_splits_cb_canvas_size_allocate),
		    splits);

  /* create piano canvas item */
  splits->piano =
    SWAMIGUI_PIANO (gnome_canvas_item_new (gnome_canvas_root
					   (GNOME_CANVAS (splits->top_canvas)),
					   SWAMIGUI_TYPE_PIANO, NULL));

  /* create velocity gradient canvas item */
  pixbuf = swamigui_splits_create_velocity_gradient ();
  splits->velgrad = gnome_canvas_item_new (gnome_canvas_root
					   (GNOME_CANVAS (splits->top_canvas)),
					   GNOME_TYPE_CANVAS_PIXBUF,
					   "pixbuf", pixbuf,
					   "x", (double)0.0,
					   "y", (double)0.0,
					   "height", (double)SWAMIGUI_PIANO_DEFAULT_HEIGHT,
					   "height-set", TRUE,
					   "width-set", TRUE,
					   NULL);
  gnome_canvas_item_hide (splits->velgrad);

  /* assign adjustments of lower scrolled window */
  scrollwin = swamigui_util_glade_lookup (gladewidg, "SplitsScrollWinLower");
  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (scrollwin), hadj);
  gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (scrollwin), vadj);

  /* setup lower canvas */
  splits->low_canvas = gnome_canvas_new ();
  gtk_widget_show (splits->low_canvas);
  gtk_container_add (GTK_CONTAINER (scrollwin), splits->low_canvas);
  gnome_canvas_set_center_scroll_region (GNOME_CANVAS (splits->low_canvas), FALSE);

  /* set background color of canvas to white */
  style = gtk_style_copy (gtk_widget_get_style (splits->low_canvas));
  style->bg[GTK_STATE_NORMAL] = style->white;
  gtk_widget_set_style (splits->low_canvas, style);

  /* create lower background rectangle (to catch events) */
  splits->bgrect =
    gnome_canvas_item_new (gnome_canvas_root
			   (GNOME_CANVAS (splits->low_canvas)),
			   GNOME_TYPE_CANVAS_RECT,
			   "fill-color-rgba", splits->bg_color,
			   "x1", (double)0.0,
			   "x2", (double)SWAMIGUI_PIANO_DEFAULT_WIDTH,
			   "y1", (double)0.0,
			   "y2", (double)splits->span_height,
			   NULL);

  /* create vertical line group */
  splits->vline_group = GNOME_CANVAS_GROUP
    (gnome_canvas_item_new (gnome_canvas_root
			    (GNOME_CANVAS (splits->low_canvas)),
			    GNOME_TYPE_CANVAS_GROUP,
			    NULL));

  //  g_signal_connect (gnome_canvas_root (GNOME_CANVAS (splits->low_canvas)),
  g_signal_connect (splits->low_canvas, "event",
		    G_CALLBACK (swamigui_splits_cb_low_canvas_event), splits);
}

/* callback when Notes or Velocity mode toggle button is clicked */
static void
splits_cb_mode_btn_clicked (GtkButton *button, gpointer user_data)
{
  SwamiguiSplits *splits = SWAMIGUI_SPLITS (user_data);

  g_signal_handlers_block_by_func (splits->notes_btn, splits_cb_mode_btn_clicked, splits);
  g_signal_handlers_block_by_func (splits->velocity_btn, splits_cb_mode_btn_clicked, splits);

  if ((GtkWidget *)button == splits->notes_btn)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (splits->notes_btn), TRUE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (splits->velocity_btn), FALSE);
    swamigui_splits_set_mode (splits, SWAMIGUI_SPLITS_NOTE);
  }
  else
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (splits->notes_btn), FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (splits->velocity_btn), TRUE);
    swamigui_splits_set_mode (splits, SWAMIGUI_SPLITS_VELOCITY);
  }

  g_signal_handlers_unblock_by_func (splits->notes_btn, splits_cb_mode_btn_clicked, splits);
  g_signal_handlers_unblock_by_func (splits->velocity_btn, splits_cb_mode_btn_clicked, splits);
}

static void
swamigui_splits_cb_canvas_size_allocate (GtkWidget *widget,
					 GtkAllocation *allocation,
					 gpointer user_data)
{
  SwamiguiSplits *splits = SWAMIGUI_SPLITS (user_data);

  if (!splits->width_set)
    {
      if (allocation->width < SWAMIGUI_PIANO_DEFAULT_WIDTH)
	allocation->width = SWAMIGUI_PIANO_DEFAULT_WIDTH;

      swamigui_splits_set_width (splits, allocation->width);
    }
}

static gboolean
swamigui_splits_cb_low_canvas_event (GnomeCanvasItem *item, GdkEvent *event,
				     gpointer data)
{
  SwamiguiSplits *splits = SWAMIGUI_SPLITS (data);
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  SwamiguiSplitsEntry *entry, *selsplit;
  GList *p, *selsplitp;
  double dlow, dhigh;
  int index, i, low, high, note, noteofs;
  gboolean updatesel = FALSE;

  switch (event->type)
  {
    case GDK_SCROLL:
      /* forward the event to the vertical scroll bar */
      gtk_widget_event (splits->vertical_scrollbar, event);
      return (TRUE);

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *)event;
      if (!(bevent->button >= 1 && bevent->button <= 3) || bevent->y < 0.0)
	break;

      p = swamigui_splits_get_split_at_pos (splits, bevent->x, bevent->y, &index);
      if (!p) return (FALSE);	/* no split found? */

      selsplit = (SwamiguiSplitsEntry *)(p->data);
      selsplitp = p;

      if (bevent->button != 1 && bevent->button != 2) break;

      /* deselect all spans if CTRL and SHIFT not pressed and Left click or Middle on unselected item */
      if (!(bevent->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
	  && !(swamigui_root_is_middle_click (NULL, bevent) && SPLIT_IS_SELECTED (selsplit)))
	{
	  for (p = splits->entry_list; p; p = g_list_next (p))
	    {
	      entry = (SwamiguiSplitsEntry *)(p->data);
	      if (SPLIT_IS_SELECTED (entry))
		{
		  entry->flags &= ~SPLIT_SELECTED;
		  swamigui_splits_update_item_sel (entry);
		  updatesel = TRUE;
		}
	    }
	}

      /* no CTRL or SHIFT, single select */
      if (!(bevent->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)))
      {
        if (!SPLIT_IS_SELECTED (selsplit))
        {
          selsplit->flags |= SPLIT_SELECTED;
          swamigui_splits_update_item_sel (selsplit);
          updatesel = TRUE;
        }

        splits->anchor = index;

        /* not yet decided if to edit handle */
        if (bevent->button == 1)
        {
          splits->active_drag = ACTIVE_UNDECIDED;
          splits->active_xpos = bevent->x;
          splits->active_drag_btn = 1;
          splits->threshold_value = 0.0;
          splits->active_split = selsplitp;
        }
      }

      if (swamigui_root_is_middle_click (NULL, bevent)) /* middle click? */
	{
	  if (!SPLIT_IS_SELECTED (selsplit))
	    {
	      selsplit->flags |= SPLIT_SELECTED;
	      swamigui_splits_update_item_sel (selsplit);
	      updatesel = TRUE;
	    }

	  splits->anchor = index;

          if (updatesel) swamigui_splits_update_selection (splits);

	  note = swamigui_piano_pos_to_note (splits->piano, bevent->x, 0.0,
					     NULL, NULL);
	  if (note == -1) return (FALSE);

          // CTRL & SHIFT move both root note and ranges, CTRL moves ranges, move root notes otherwise
          if ((bevent->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
              == (GDK_CONTROL_MASK | GDK_SHIFT_MASK) && selsplit->rootnote && selsplit->span)
            splits->active_drag = ACTIVE_MOVE_BOTH;
          else if ((bevent->state & GDK_CONTROL_MASK) || !selsplit->rootnote)
            splits->active_drag = ACTIVE_MOVE_RANGES;
          else
          {
            gboolean selected = FALSE;

            splits->active_drag = ACTIVE_MOVE_ROOTNOTES;

            // Check for multiple selected items
            for (p = splits->entry_list; p; p = g_list_next (p))
            {
              if (SPLIT_IS_SELECTED ((SwamiguiSplitsEntry *)(p->data)))
              {
                if (selected) break;
                selected = TRUE;
              }
	    }

            // Set relative note offset if multiple selected items, absolute offset otherwise
            if (p) splits->move_note_ofs = note - selsplit->rootnote_val;
            else splits->move_note_ofs = 0;
          }

	  splits->active_drag_btn = bevent->button;
	  splits->active_split = selsplitp;

          if (splits->active_drag != ACTIVE_MOVE_ROOTNOTES)
            splits->move_note_ofs = note - selsplit->range.low;

	  /* update statusbar */
	  swamigui_splits_update_status_bar (splits, selsplit->range.low,
					     selsplit->range.high);
	  return (FALSE);
	}

      /* SHIFT key and an anchor? - select range */
      if ((bevent->state & GDK_SHIFT_MASK) && splits->anchor != -1)
	{
	  int select;

	  if (splits->anchor < index)
	    {
	      low = splits->anchor;
	      high = index;
	    }
	  else
	    {
	      low = index;
	      high = splits->anchor;
	    }

	  i = -1;
	  p = splits->entry_list;
	  while (p)
	    {
	      entry = (SwamiguiSplitsEntry *)(p->data);
	      p = g_list_next (p);
	      i++;

	      if (i >= low && i <= high) select = TRUE;
	      else if (!(bevent->state & GDK_CONTROL_MASK)) select = FALSE;
	      else continue;

	      if (SPLIT_IS_SELECTED (entry) != select)
		{
		  entry->flags ^= SPLIT_SELECTED;
		  swamigui_splits_update_item_sel (entry);
		  updatesel = TRUE;
		}
	    }
	}
      else if (bevent->state & GDK_CONTROL_MASK) /* CTRL key? */
	{
	  selsplit->flags ^= SPLIT_SELECTED; /* toggle sel state */
	  swamigui_splits_update_item_sel (selsplit);
	  updatesel = TRUE;
	  splits->anchor = index;
	}

      if (updatesel) swamigui_splits_update_selection (splits);
      break;
    case GDK_BUTTON_RELEASE:
      if (splits->active_drag == ACTIVE_NONE) return (FALSE);

      /* Same button released as caused the drag? */
      if (splits->active_drag_btn == event->button.button)
      {
	splits->active_drag = ACTIVE_NONE;

	/* clear status bar */
	swamigui_statusbar_msg_set_label (swamigui_root->statusbar, 0, "Global", NULL);
      }
      break;
    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *)event;
      if (splits->active_drag == ACTIVE_NONE)
	{
	  p = swamigui_splits_get_split_at_pos (splits, mevent->x, mevent->y, NULL);
	  if (p)
	    {
	      entry = (SwamiguiSplitsEntry *)(p->data);
	      swamigui_splits_update_status_bar (splits, entry->range.low,
						 entry->range.high);
	    }

	  return (FALSE);
	}

      entry = (SwamiguiSplitsEntry *)(splits->active_split->data);

      /* still haven't decided which handle? */
      if (splits->active_drag == ACTIVE_UNDECIDED)
	{	/* has cursor moved beyond threshold? */
	  splits->threshold_value += ABS (mevent->x - splits->active_xpos);
	  if (splits->threshold_value < splits->move_threshold) return (FALSE);

	  /* find the edge closest to the original click */
	  dlow = swamigui_piano_note_to_pos (splits->piano, entry->range.low,
	  				     -1, FALSE, NULL);
	  dhigh = swamigui_piano_note_to_pos (splits->piano, entry->range.high,
	  				      1, FALSE, NULL);
	  if (ABS (splits->active_xpos - dlow)
	      <= ABS (splits->active_xpos - dhigh))
	    splits->active_drag = ACTIVE_LOW; /* select lower handle1 */
	  else splits->active_drag = ACTIVE_HIGH; /* select upper handle2 */
	}

      if (mevent->x < 0.0) note = 0;
      else if (mevent->x > splits->piano->width) note = 127;
      else note = swamigui_piano_pos_to_note (splits->piano, mevent->x, 0.0,
					      NULL, NULL);
      if (note == -1) return (FALSE);

      /* Handle move separately (could be multiple items) */
      if (splits->active_drag >= ACTIVE_MOVE_ROOTNOTES && splits->active_drag <= ACTIVE_MOVE_BOTH)
      {
	note -= splits->move_note_ofs;

	if (note < 0) note = 0;
	else if (note > 127) note = 127;

	/* If drag has not changed the current note offset, short cut */
	if ((splits->active_drag != ACTIVE_MOVE_ROOTNOTES && entry->range.low == note)
            || (splits->active_drag == ACTIVE_MOVE_ROOTNOTES && entry->rootnote_val == note))
	  break;

	if (splits->active_drag == ACTIVE_MOVE_ROOTNOTES)
	  noteofs = note - entry->rootnote_val;
	else
	  noteofs = note - entry->range.low;	/* note offset to low note range */

	/* Check if any spans/root notes would go out of range and clamp accordingly */
	for (p = splits->entry_list; p && noteofs != 0; p = p->next)
	{
	  entry = (SwamiguiSplitsEntry *)(p->data);
	  if (!SPLIT_IS_SELECTED (entry)) continue;

	  if (splits->active_drag != ACTIVE_MOVE_ROOTNOTES && entry->span)
	  {
	    if ((int)(entry->range.low) + noteofs < 0) noteofs = -entry->range.low;
	    if ((int)(entry->range.high) + noteofs > 127) noteofs = 127 - entry->range.high;
	  }

	  if (splits->active_drag != ACTIVE_MOVE_RANGES && entry->rootnote)
	  {
	    if ((int)(entry->rootnote_val) + noteofs < 0) noteofs = -entry->rootnote_val;
	    if ((int)(entry->rootnote_val) + noteofs > 127) noteofs = 127 - entry->rootnote_val;
	  }
	}

	if (noteofs == 0) break;

	/* Move the selected spans and/or root notes */
	for (p = splits->entry_list; p; p = p->next)
	{
	  entry = (SwamiguiSplitsEntry *)(p->data);
	  if (!SPLIT_IS_SELECTED (entry)) continue;

	  if (splits->active_drag != ACTIVE_MOVE_ROOTNOTES && entry->span)
	  {
	    swamigui_splits_entry_set_span_control (entry, entry->range.low + noteofs,
						    entry->range.high + noteofs);

	    if (entry == splits->active_split->data)
	      swamigui_splits_update_status_bar (splits, entry->range.low, entry->range.high);
	  }
	  if (splits->active_drag != ACTIVE_MOVE_RANGES && entry->rootnote)
	    swamigui_splits_entry_set_root_note_control (entry, entry->rootnote_val
							 + noteofs);
	}
	break;
      }

      low = entry->range.low;
      high = entry->range.high;

      switch (splits->active_drag)
      {
	case ACTIVE_LOW:	/* lower handle? */
	  /* need to switch controlled handles? */
	  if (note > entry->range.high)
	    {
	      splits->active_drag = ACTIVE_HIGH;
	      low = entry->range.high;
	      high = note;
	    }
	  else low = note;
	  break;
	case ACTIVE_HIGH:		/* upper handle */
	  /* need to switch controlled handles? */
	  if (note < entry->range.low)
	    {
	      splits->active_drag = ACTIVE_LOW;
	      high = entry->range.low;
	      low = note;
	    }
	  else high = note;
	  break;
      }

      if (low != entry->range.low || high != entry->range.high)
      {
	swamigui_splits_update_status_bar (splits, low, high);
	swamigui_splits_entry_set_span_control (entry, low, high);
      }
      break;
    default:
      break;
  }

  return (FALSE);
}

/* find a split at a given position */
static GList *
swamigui_splits_get_split_at_pos (SwamiguiSplits *splits, int x, int y,
				  int *index)
{
  GList *p;
  int d, idx;

  if (index) *index = 0;

  /* click is at least greater than upper blank area? */
  if (y <= splits->span_height) return (NULL);

  /* subtract blank area and half of spacing */
  d = y - (splits->span_height - splits->span_spacing / 2);

  /* calculate span index */
  idx = d / (splits->span_height + splits->span_spacing);

  /* calculate pixel offset in span */
  d -= idx * (splits->span_height + splits->span_spacing);

  if (index) *index = idx;

  if (d < splits->span_height) /* click within span height? */
  {
    p = g_list_nth (splits->entry_list, idx);

    if (p && ((SwamiguiSplitsEntry *)(p->data))->span_control)
      return (p);
    else return (NULL);
  }
  else return (NULL);
}

/* Update status bar message.  Use high = -1 for root notes or other non-range
 * parameters */
static void
swamigui_splits_update_status_bar (SwamiguiSplits *splits, int low, int high)
{
  char lstr[5], hstr[5];
  char *msg;

  if (splits->mode == SWAMIGUI_SPLITS_NOTE)
    {
      swami_util_midi_note_to_str (low, lstr);
      if (high != -1)
      {
	swami_util_midi_note_to_str (high, hstr);
	msg = g_strdup_printf (_("Range: %s:%s (%d-%d)"), lstr, hstr, low, high);
      }
      else msg = g_strdup_printf (_("Note: %s (%d)"), lstr, low);
    }
  else msg = g_strdup_printf (_("Range: %d-%d"), low, high);

  swamigui_statusbar_msg_set_label (swamigui_root->statusbar, 0, "Global", msg);
  g_free (msg);
}

/* SwamiControlFunc callback for getting a splits current range value */
static void
swamigui_splits_span_control_get_func (SwamiControl *control, GValue *value)
{
  SwamiguiSplitsEntry *entry = SWAMI_CONTROL_FUNC_DATA (control);

  if (entry->destroyed) return;
  ipatch_value_set_range (value, &entry->range);
}

/* SwamiControlFunc callback for setting a splits current range value */
static void
swamigui_splits_span_control_set_func (SwamiControl *control,
				       SwamiControlEvent *event,
				       const GValue *value)
{
  SwamiguiSplitsEntry *entry = SWAMI_CONTROL_FUNC_DATA (control);
  IpatchRange *range;

  if (entry->destroyed) return;

  range = ipatch_value_get_range (value);
  swamigui_splits_entry_set_span (entry, range->low, range->high);
}

/* SwamiControlFunc destroy callback. Things are tricky here because
   control events might still occur after a split has been destroyed.
   Therefore each control holds a reference to the splits widget and
   frees its entry as well (if no more entry->refcounts). */
static void
swamigui_splits_span_control_destroy_func (SwamiControlFunc *control)
{
  SwamiguiSplitsEntry *entry = SWAMI_CONTROL_FUNC_DATA (control);

  /* -- unref the control's held reference to the splits widget */
  g_object_unref (entry->splits);

  /* free the entry if no more controls referencing it */
  if (g_atomic_int_dec_and_test (&entry->refcount)) g_free (entry);
}

/* SwamiControlFunc callback for getting a root note current value */
static void
swamigui_splits_root_note_control_get_func (SwamiControl *control, GValue *value)
{
  SwamiguiSplitsEntry *entry = SWAMI_CONTROL_FUNC_DATA (control);

  if (entry->destroyed) return;
  g_value_set_int (value, entry->rootnote_val);
}

/* SwamiControlFunc callback for setting a root note current value */
static void
swamigui_splits_root_note_control_set_func (SwamiControl *control,
					    SwamiControlEvent *event,
					    const GValue *value)
{
  SwamiguiSplitsEntry *entry = SWAMI_CONTROL_FUNC_DATA (control);

  if (entry->destroyed) return;

  swamigui_splits_entry_set_root_note (entry, g_value_get_int (value));
}

/* SwamiControlFunc destroy callback. Things are tricky here because
   control events might still occur after a split has been destroyed.
   Therefore each control holds a reference to the splits widget and
   frees its entry as well (if no more entry->refcounts). */
static void
swamigui_splits_root_note_control_destroy_func (SwamiControlFunc *control)
{
  SwamiguiSplitsEntry *entry = SWAMI_CONTROL_FUNC_DATA (control);

  /* -- unref the control's held reference to the splits widget */
  g_object_unref (entry->splits);

  /* free the entry if no more controls referencing it */
  if (g_atomic_int_dec_and_test (&entry->refcount)) g_free (entry);
}

/* internal functions */

/* unset any active split handler */
static void
swamigui_splits_deactivate_handler (SwamiguiSplits *splits)
{
  swamigui_splits_remove_all (splits);
  splits->handler = NULL;
  splits->handler_data = NULL;
  g_object_set (splits, "splits-item", NULL, NULL);
}

static SwamiguiSplitsEntry *
swamigui_splits_create_entry (SwamiguiSplits *splits, GObject *item)
{
  SwamiguiSplitsEntry *entry;

  entry = g_new0 (SwamiguiSplitsEntry, 1);
  entry->splits = splits;
  entry->index = 0;
  entry->item = g_object_ref (G_OBJECT (item));
  entry->range.low = 0;
  entry->range.high = 127;
  entry->destroyed = FALSE;
  entry->refcount = 0;
  entry->flags = 0;		/* not selected */

  return (entry);
}

/* calls destroy on split widgets and unrefs objects */
static void
swamigui_splits_destroy_entry (SwamiguiSplitsEntry *entry)
{
  entry->destroyed = TRUE;

  if (entry->span) gtk_object_destroy (GTK_OBJECT (entry->span));
  if (entry->lowline) gtk_object_destroy (GTK_OBJECT (entry->lowline));
  if (entry->highline) gtk_object_destroy (GTK_OBJECT (entry->highline));
  if (entry->rootnote) gtk_object_destroy (GTK_OBJECT (entry->rootnote));
  if (entry->item) g_object_unref (entry->item);
  if (entry->span_control) swami_control_disconnect_unref (entry->span_control);
  if (entry->rootnote_control) swami_control_disconnect_unref (entry->rootnote_control);
}

/* lookup a entry GList pointer by item */
static GList *
swamigui_splits_lookup_item (SwamiguiSplits *splits, GObject *item)
{
  GList *p;

  p = splits->entry_list;
  while (p)
    {
      if (((SwamiguiSplitsEntry *)(p->data))->item == item) break;
      p = g_list_next (p);
    }

  return (p);
}

/* visually update a split's selected state */
static void
swamigui_splits_update_item_sel (SwamiguiSplitsEntry *entry)
{
  gboolean sel = SPLIT_IS_SELECTED (entry);
  SwamiguiSplits *splits = entry->splits;
  guint color;

  g_object_set (entry->span,
		"fill-color-rgba",
		sel ? splits->span_sel_color : splits->span_color,
		"outline-color-rgba", sel ? splits->span_sel_outline_color
		: splits->span_outline_color,
		NULL);

  if (sel)
    {
      gnome_canvas_item_raise_to_top (entry->lowline);
      gnome_canvas_item_raise_to_top (entry->highline);
      color = splits->line_sel_color;
    }
  else
    {
      gnome_canvas_item_lower_to_bottom (entry->lowline);
      gnome_canvas_item_lower_to_bottom (entry->highline);
      color = splits->line_color;
    }

  g_object_set (entry->lowline, "fill-color-rgba", color, NULL);
  g_object_set (entry->highline, "fill-color-rgba", color, NULL);
}

/* updates splits->selection based on currently selected splits and issues
   a property change notify on "item-selection" */
static void
swamigui_splits_update_selection (SwamiguiSplits *splits)
{
  SwamiguiSplitsEntry *entry;
  IpatchList *listobj;
  GList *list = NULL, *p;

  for (p = splits->entry_list; p; p = g_list_next (p))
    {
      entry = (SwamiguiSplitsEntry *)(p->data);
      if (SPLIT_IS_SELECTED (entry))
	{
	  list = g_list_prepend (list, entry->item);
	  g_object_ref (entry->item); /* ++ ref item for list obj */
	}
    }

  list = g_list_reverse (list);

  listobj = ipatch_list_new ();	/* ++ ref new list object */
  listobj->items = list;

  if (splits->selection) g_object_unref (splits->selection);
  splits->selection = listobj;	/* !! takes over list object reference */

  g_object_notify (G_OBJECT (splits), "item-selection");
}

/**
 * swamigui_splits_new:
 *
 * Create new note/velocity splits widget.
 *
 * Returns: New splits widget.
 */
GtkWidget *
swamigui_splits_new (void)
{
  return (GTK_WIDGET (gtk_type_new (swamigui_splits_get_type ())));
}

/**
 * swamigui_splits_set_mode:
 * @splits: Splits object
 * @mode: Velocity or key mode enum
 *
 * Set the mode of a splits object.
 */
void
swamigui_splits_set_mode (SwamiguiSplits *splits, SwamiguiSplitsMode mode)
{
  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));

  if (mode == splits->mode) return;

  splits->mode = mode;

  if (splits->mode == SWAMIGUI_SPLITS_VELOCITY)	/* velocity mode? */
    {
      gnome_canvas_item_hide (GNOME_CANVAS_ITEM (splits->piano));
      gnome_canvas_item_show (splits->velgrad);
    }
  else	/* note mode */
    {
      gnome_canvas_item_hide (splits->velgrad);
      gnome_canvas_item_show (GNOME_CANVAS_ITEM (splits->piano));
    }

  G_LOCK (handlers);
  if (splits->handler)
    {
      splits->status = SWAMIGUI_SPLITS_MODE;
      if (!(*splits->handler)(splits))
	swamigui_splits_deactivate_handler (splits);
      splits->status = SWAMIGUI_SPLITS_NORMAL;
    }
  G_UNLOCK (handlers);
}

/**
 * swamigui_splits_set_width:
 * @splits: Splits object
 * @width: Width in pixels
 *
 * Set the width of the splits widget in pixels.
 */
void
swamigui_splits_set_width (SwamiguiSplits *splits, int width)
{
  if (width == splits->width) return;

  splits->width = width;

  /* update piano width */
  g_object_set (splits->piano, "width-pixels", width, NULL);

  /* update velocity width */
  g_object_set (splits->velgrad, "width", (double)width, NULL);

  swamigui_splits_update_entries (splits, splits->entry_list, TRUE, FALSE);
}

/**
 * swamigui_splits_set_selection:
 * @splits: Splits object
 * @items: List of selected items (selected splits and/or the parent of
 *   split items) or %NULL to unset selection.
 *
 * Set the items of a splits widget. The @items list can contain
 * an item that is a parent of items with split parameters (a
 * SoundFont #IpatchSF2Preset or IpatchSF2Inst for example) and/or a list
 * of children split item's with the same parent (for example
 * SoundFont #IpatchSF2PZone items), any other selection list will de-activate
 * the splits widget.
 */
void
swamigui_splits_set_selection (SwamiguiSplits *splits, IpatchList *items)
{
  if (swamigui_splits_real_set_selection (splits, items))
    g_object_notify (G_OBJECT (splits), "item-selection");
}

static gboolean
swamigui_splits_real_set_selection (SwamiguiSplits *splits, IpatchList *items)
{
  SwamiguiSplitsHandler hfunc;
  GList *p;

  g_return_val_if_fail (SWAMIGUI_IS_SPLITS (splits), FALSE);
  g_return_val_if_fail (!items || IPATCH_IS_LIST (items), FALSE);

  if (splits->selection) g_object_unref (splits->selection); /* -- unref old */
  if (items) splits->selection = ipatch_list_duplicate (items);	/* ++ ref */
  else splits->selection = NULL;

  G_LOCK (handlers);
  if (splits->handler)		/* active handler? */
    {
      splits->status = SWAMIGUI_SPLITS_UPDATE;
      if (!items || !(*splits->handler)(splits))
	swamigui_splits_deactivate_handler (splits);
    }

  if (items && !splits->handler) /* re-test in case it was de-activated */
    {
      splits->status = SWAMIGUI_SPLITS_INIT;
      p = split_handlers;
      while (p)			/* try handlers */
	{
	  hfunc = (SwamiguiSplitsHandler)(p->data);
	  if ((*hfunc)(splits)) /* selection handled? */
	    {
	      splits->handler = hfunc;
	      break;
	    }
	  p = g_list_next (p);
	}
    }

  G_UNLOCK (handlers);

  if (!splits->handler)		/* no handler found? - Try default. */
    {
      if (swamigui_splits_default_handler (splits))
	splits->handler = swamigui_splits_default_handler;
    }

  splits->status = SWAMIGUI_SPLITS_NORMAL;

  return (TRUE);
}

/**
 * swamigui_splits_get_selection:
 * @splits: Splits widget
 *
 * Get the list of active items in a splits widget (a parent of split items
 * and/or split items).
 *
 * Returns: New list containing splits with a ref count of one which the
 *   caller owns or %NULL if no active splits.
 */
IpatchList *
swamigui_splits_get_selection (SwamiguiSplits *splits)
{
  g_return_val_if_fail (SWAMIGUI_IS_SPLITS (splits), NULL);

  if (splits->selection) return (ipatch_list_duplicate (splits->selection));
  else return (NULL);
}

/**
 * swamigui_splits_select_items:
 * @splits: Splits widget
 * @items: List of objects to select (%NULL to unselect all)
 * 
 * Set the list of splits currently selected.  Usually only used by
 * #SwamiguiSplit handlers.
 */
void
swamigui_splits_select_items (SwamiguiSplits *splits, GList *items)
{
  SwamiguiSplitsEntry *entry;
  GHashTable *hash;
  gboolean sel;
  GList *p;

  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));

  /* hash the item list for speed in the case of large split lists */
  hash = g_hash_table_new (NULL, NULL);
  for (p = items; p; p = p->next)
    g_hash_table_insert (hash, p->data, GUINT_TO_POINTER (TRUE));

  for (p = splits->entry_list; p; p = g_list_next (p))
    {
      entry = (SwamiguiSplitsEntry *)(p->data);

      sel = GPOINTER_TO_UINT (g_hash_table_lookup (hash, entry->item));
      if (sel != SPLIT_IS_SELECTED (entry))
	{
	  if (sel) entry->flags |= SPLIT_SELECTED;
	  else entry->flags &= ~SPLIT_SELECTED;

	  swamigui_splits_update_item_sel (entry);
	}
    }
}

/**
 * swamigui_splits_select_all:
 * @splits: Splits widget
 *
 * Select all splits in a splits widget.
 */
void
swamigui_splits_select_all (SwamiguiSplits *splits)
{
  SwamiguiSplitsEntry *entry;
  GList *p;

  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));

  for (p = splits->entry_list; p; p = g_list_next (p))
    {
      entry = (SwamiguiSplitsEntry *)(p->data);
      if (!SPLIT_IS_SELECTED (entry))
	{
	  entry->flags |= SPLIT_SELECTED;
	  swamigui_splits_update_item_sel (entry);
	}
    }
}

/**
 * swamigui_splits_unselect_all:
 * @splits: Splits widget
 *
 * Unselect all splits in a splits widget.
 */
void
swamigui_splits_unselect_all (SwamiguiSplits *splits)
{
  SwamiguiSplitsEntry *entry;
  GList *p;

  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));

  for (p = splits->entry_list; p; p = g_list_next (p))
    {
      entry = (SwamiguiSplitsEntry *)(p->data);
      if (SPLIT_IS_SELECTED (entry))
	{
	  entry->flags &= ~SPLIT_SELECTED;
	  swamigui_splits_update_item_sel (entry);
	}
    }
}

/**
 * swamigui_splits_item_changed:
 * @splits: Splits widget
 *
 * Called to indicate that the active "splits-item" has changed and the splits
 * should therefore be updated.
 */
void
swamigui_splits_item_changed (SwamiguiSplits *splits)
{
  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));

  if (!splits->handler) return;

  splits->status = SWAMIGUI_SPLITS_CHANGED;
  if (!splits->handler (splits))
    swamigui_splits_deactivate_handler (splits);
}

/**
 * swamigui_splits_register_handler:
 * @handler: Splits handler function to register
 *
 * Registers a new handler for splits widgets. Split handlers interface
 * patch item's of particular types with note/velocity split parameters and
 * note pointer controls (such as a root note parameter).
 *
 * MT: This function is multi-thread safe and can be called from outside
 * of the GUI thread.
 */
void
swamigui_splits_register_handler (SwamiguiSplitsHandler handler)
{
  g_return_if_fail (handler != NULL);

  G_LOCK (handlers);
  split_handlers = g_list_prepend (split_handlers, handler);
  G_UNLOCK (handlers);
}

/**
 * swamigui_splits_unregister_handler:
 * @handler: Handler function to unregister
 *
 * Unregisters a handler previously registered with
 * swamigui_splits_register_handler().
 *
 * MT: This function is multi-thread safe and can be called from outside
 * of the GUI thread.
 */
void
swamigui_splits_unregister_handler (SwamiguiSplitsHandler handler)
{
  g_return_if_fail (handler != NULL);

  G_LOCK (handlers);
  split_handlers = g_list_remove (split_handlers, handler);
  G_UNLOCK (handlers);
}

/**
 * swamigui_splits_insert:
 * @splits: Splits widget
 * @item: Object for this split
 * @index: Index in list of existing splits in widget (-1 to append).
 *
 * Adds a new entry to a splits widget associated with a given object
 * @item.  An entry is a place holder for a split range (key or velocity)
 * and/or root note controls.
 *
 * Returns: Splits entry which is internal and should only be used with
 *   public accessor functions and should not be modified or freed.
 */
SwamiguiSplitsEntry *
swamigui_splits_insert (SwamiguiSplits *splits, GObject *item, int index)
{
  SwamiguiSplitsEntry *entry;
  GList *p;

  g_return_val_if_fail (SWAMIGUI_IS_SPLITS (splits), NULL);
  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  entry = swamigui_splits_create_entry (splits, item);

  if (index < 0 || index >= splits->entry_count)	/* Append? */
  {
    index = splits->entry_count;
    splits->entry_list = g_list_append (splits->entry_list, entry);
    p = NULL;
  }
  else
  {
    p = g_list_nth (splits->entry_list, index);
    splits->entry_list = g_list_insert_before (splits->entry_list, p, entry);
  }

  entry->index = index;

  splits->entry_count++;					/* increment the split count */
  splits->height += splits->span_height + splits->span_spacing;	/* update splits total height */

  swamigui_splits_update_entries (splits, p, FALSE, TRUE);

  return (entry);
}

/* Update geometry of items in relation to entry changes or width change.
 * Also updates entry->index values */
static void
swamigui_splits_update_entries (SwamiguiSplits *splits, GList *startp,
				gboolean width_change, gboolean height_change)
{
  GnomeCanvasPoints *lpoints;
  double xpos1, xpos2, ypos1, ypos2, halfwidth;
  SwamiguiSplitsEntry *entry;
  int index;
  GList *p;

  /* update lower canvas background rectangle */
  if (width_change && height_change)
    g_object_set (splits->bgrect,
		  "x2", (double)splits->width,
		  "y2", (double)splits->height,
		  NULL);
  else if (width_change) g_object_set (splits->bgrect, "x2", (double)splits->width, NULL);
  else g_object_set (splits->bgrect, "y2", (double)splits->height, NULL);

  lpoints = gnome_canvas_points_new (2);	/* line points */

  if (startp && startp->prev)
    index = ((SwamiguiSplitsEntry *)(startp->prev->data))->index + 1;
  else index = 0;

  /* top of starting span to update */
  ypos1 = splits->span_height + (index * (splits->span_height + splits->span_spacing));

  /* update splits */
  for (p = startp; p; p = p->next, index++)
    {
      entry = (SwamiguiSplitsEntry *)(p->data);
      entry->index = index;

      if (entry->span)
      {
	xpos1 = swamigui_piano_note_to_pos (splits->piano,
					    entry->range.low, -1, FALSE, NULL);
	xpos2 = swamigui_piano_note_to_pos (splits->piano,
					    entry->range.high, 1, FALSE, NULL);

	if (width_change && height_change)
	  g_object_set (entry->span,
			"x1", xpos1,
			"x2", xpos2,
			"y1", ypos1,
			"y2", ypos1 + splits->span_height,
			NULL);
	else if (width_change)
	  g_object_set (entry->span,
			"x1", xpos1,
			"x2", xpos2,
			NULL);
	else
	  g_object_set (entry->span,
			"y1", ypos1,
			"y2", ypos1 + splits->span_height,
			NULL);

	lpoints->coords[1] = 0.0;
	lpoints->coords[3] = ypos1 + splits->span_height;

	/* set the low and high vertical line coordinates */
	lpoints->coords[0] = lpoints->coords[2] = xpos1;
	g_object_set (entry->lowline, "points", lpoints, NULL);

	lpoints->coords[0] = lpoints->coords[2] = xpos2;
	g_object_set (entry->highline, "points", lpoints, NULL);
      }

      if (entry->rootnote)
      {
	xpos1 = swamigui_piano_note_to_pos (splits->piano, entry->rootnote_val,
					    0, FALSE, NULL);
	ypos2 = ypos1 + splits->span_height;	/* Bottom of span */
	halfwidth = splits->span_height / 2.0 - 2.0;

	g_object_set (entry->rootnote,
		      "x1", xpos1 - halfwidth,
		      "x2", xpos1 + halfwidth,
		      "y1", ypos1 + 2.0,
		      "y2", ypos2 - 2.0,
		      NULL);
      }

      ypos1 += splits->span_height + splits->span_spacing;
    }

  gnome_canvas_points_free (lpoints);

  if (width_change)
    gnome_canvas_set_scroll_region (GNOME_CANVAS (splits->top_canvas), 0, 0,
				    splits->width, SWAMIGUI_PIANO_DEFAULT_HEIGHT);

  gnome_canvas_set_scroll_region (GNOME_CANVAS (splits->low_canvas), 0, 0,
				  splits->width, splits->height);
}

/**
 * swamigui_splits_remove:
 * @splits: Splits widget
 * @item: Object of split to remove
 *
 * Remove a split from a splits object by its associated object.
 */
void
swamigui_splits_remove (SwamiguiSplits *splits, GObject *item)
{
  SwamiguiSplitsEntry *entry;
  GList *lookup_item, *p;

  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));
  g_return_if_fail (IPATCH_IS_ITEM (item));

  /* lookup the entry by item */
  lookup_item = swamigui_splits_lookup_item (splits, item);
  g_return_if_fail (lookup_item != NULL);

  p = lookup_item->next;	/* advance to item after */
  entry = (SwamiguiSplitsEntry *)(lookup_item->data);

  splits->entry_list = g_list_delete_link (splits->entry_list, lookup_item);
  swamigui_splits_destroy_entry (entry); /* destroy entry */

  splits->entry_count--;	/* decrement split count */

  /* update splits total height */
  splits->height -= splits->span_height + splits->span_spacing;

  swamigui_splits_update_entries (splits, p, FALSE, TRUE);
}

/**
 * swamigui_splits_remove_all:
 * @splits: Splits widget
 *
 * Remove all splits from a splits object.
 */
void
swamigui_splits_remove_all (SwamiguiSplits *splits)
{
  GList *p;

  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));

  p = splits->entry_list;
  while (p)
    {
      swamigui_splits_destroy_entry ((SwamiguiSplitsEntry *)(p->data));
      p = g_list_delete_link (p, p);
    }

  splits->entry_list = NULL;
  splits->entry_count = 0;

  /* update total split height (just upper blank region now) */
  splits->height = splits->span_height;

  swamigui_splits_update_entries (splits, NULL, FALSE, TRUE);
}

/**
 * swamigui_splits_set_span_range:
 * @splits: Splits object
 * @item: Item of span to set
 * @low: Low value of span range
 * @high: High value of span range
 *
 * A convenience function to set a span control range. One could also
 * set this directly via the control.
 */
void
swamigui_splits_set_span_range (SwamiguiSplits *splits, GObject *item,
				int low, int high)
{
  SwamiguiSplitsEntry *entry;
  GList *lookup_item;

  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));
  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (low <= high);
  g_return_if_fail (low >= 0 && high <= 127);

  lookup_item = swamigui_splits_lookup_item (splits, item);
  g_return_if_fail (lookup_item != NULL);

  entry = (SwamiguiSplitsEntry *)(lookup_item->data);
  swamigui_splits_entry_set_span_control (entry, low, high);
}

/* sets a split span widget range and transmits the change on its control also */
static void
swamigui_splits_entry_set_span_control (SwamiguiSplitsEntry *entry, int low, int high)
{
  IpatchRange range;
  GValue value = { 0 };

  if (low == entry->range.low && high == entry->range.high) return;

  swamigui_splits_entry_set_span (entry, low, high);

  /* transmit the change via the range control */
  range.low = low;
  range.high = high;
  g_value_init (&value, IPATCH_TYPE_RANGE);
  ipatch_value_set_range (&value, &range);
  swami_control_transmit_value (entry->span_control, &value);
}

/* sets a split span widget range */
static void
swamigui_splits_entry_set_span (SwamiguiSplitsEntry *entry, int low, int high)
{
  double pos1, pos2, ypos;
  GnomeCanvasPoints *points;
  SwamiguiSplits *splits = entry->splits;

  entry->range.low = low;
  entry->range.high = high;

  pos1 = swamigui_piano_note_to_pos (entry->splits->piano, low, -1, FALSE, NULL);
  pos2 = swamigui_piano_note_to_pos (entry->splits->piano, high, 1, FALSE, NULL);

  ypos = splits->span_height + (entry->index * (splits->span_height + splits->span_spacing));

  g_object_set (entry->span,
		"x1", pos1,
		"x2", pos2,
		NULL);

  /* set low and high vertical line coordinates */
  points = gnome_canvas_points_new (2);

  points->coords[1] = 0.0;
  points->coords[3] = ypos;

  points->coords[0] = points->coords[2] = pos1;
  g_object_set (entry->lowline, "points", points, NULL);

  points->coords[0] = points->coords[2] = pos2;
  g_object_set (entry->highline, "points", points, NULL);

  gnome_canvas_points_free (points);
}

/**
 * swamigui_splits_set_root_note:
 * @splits: Splits widget
 * @item: Item of the splits entry to change the root note of
 * @val: MIDI root note value (0-127)
 *
 * A convenience function to set the root note value of a splits entry.
 */
void
swamigui_splits_set_root_note (SwamiguiSplits *splits, GObject *item, int val)
{
  SwamiguiSplitsEntry *entry;
  GList *lookup_item;

  g_return_if_fail (SWAMIGUI_IS_SPLITS (splits));
  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (val >= 0 && val <= 127);

  lookup_item = swamigui_splits_lookup_item (splits, item);
  g_return_if_fail (lookup_item != NULL);

  entry = (SwamiguiSplitsEntry *)(lookup_item->data);
  swamigui_splits_entry_set_root_note_control (entry, val);
}

/* sets a split root note widget range and transmits the change on its control also */
static void
swamigui_splits_entry_set_root_note_control (SwamiguiSplitsEntry *entry, int val)
{
  GValue value = { 0 };

  if (val == entry->rootnote_val) return;

  swamigui_splits_entry_set_root_note (entry, val);

  /* transmit the change via the control */
  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, val);
  swami_control_transmit_value (entry->rootnote_control, &value);
}

/* sets a split root note widget value */
static void
swamigui_splits_entry_set_root_note (SwamiguiSplitsEntry *entry, int val)
{
  SwamiguiSplits *splits = entry->splits;
  double xpos, ypos1, ypos2, halfwidth;

  g_return_if_fail (entry->rootnote != NULL);

  entry->rootnote_val = val;

  /* top of starting span to update */
  ypos1 = splits->span_height + (entry->index * (splits->span_height + splits->span_spacing));

  xpos = swamigui_piano_note_to_pos (splits->piano, val, 0, FALSE, NULL);
  ypos2 = ypos1 + splits->span_height;	/* Bottom of span */
  halfwidth = splits->span_height / 2.0 - 2.0;

  g_object_set (entry->rootnote,
		"x1", xpos - halfwidth,
		"x2", xpos + halfwidth,
		"y1", ypos1 + 2.0,
		"y2", ypos2 - 2.0,
		NULL);
}

/**
 * swamigui_splits_entry_get_split_control:
 * @entry: Splits entry pointer
 * 
 * Get the span control for a given splits entry.  The span control is
 * created if it hasn't already been and is used for controlling a note or
 * velocity range.
 * 
 * Returns: The split control for the given @entry. The control has not been
 * referenced and is only valid while the GUI split exists.
 */
SwamiControl *
swamigui_splits_entry_get_span_control (SwamiguiSplitsEntry *entry)
{
  GnomeCanvasGroup *root;
  GnomeCanvasPoints *points;
  SwamiguiSplits *splits;
  double ypos;

  g_return_val_if_fail (entry != NULL, NULL);

  if (entry->span_control) return (entry->span_control);

  splits = entry->splits;
  root = gnome_canvas_root (GNOME_CANVAS (entry->splits->low_canvas));

  g_object_ref (entry->splits);	/* ++ ref the splits object for the control */
  g_atomic_int_inc (&entry->refcount);	/* ++ ref entry structure for span_control */

  /* create control for span range (++ ref new object) */
  entry->span_control = swamigui_control_new (SWAMI_TYPE_CONTROL_FUNC);
  swami_control_set_spec (entry->span_control,
		g_param_spec_boxed ("value", "value", "value",
				    IPATCH_TYPE_RANGE, G_PARAM_READWRITE));
  swami_control_func_assign_funcs (SWAMI_CONTROL_FUNC (entry->span_control),
				   swamigui_splits_span_control_get_func,
				   swamigui_splits_span_control_set_func,
				   swamigui_splits_span_control_destroy_func,
				   entry);

  ypos = splits->span_height + (entry->index * (splits->span_height + splits->span_spacing));

  entry->span =
    gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_RECT,
			   "fill-color-rgba", entry->splits->span_color,
			   "outline-color-rgba", entry->splits->span_outline_color,
			   "x1", 0.0,
			   "x2", (double)(entry->splits->piano->width),
			   "y1", ypos,
			   "y2", ypos + splits->span_height,
			   NULL);

  /* set coordinates of low and high range vertical lines */
  points = gnome_canvas_points_new (2);
  points->coords[1] = 0.0;
  points->coords[3] = ypos + splits->span_height;

  points->coords[0] = points->coords[2] = 0.0;

  entry->lowline =
    gnome_canvas_item_new (entry->splits->vline_group, GNOME_TYPE_CANVAS_LINE,
			   "fill-color-rgba", entry->splits->line_color,
			   "width-pixels", 1,
			   "points", points,
			   NULL);

  points->coords[0] = points->coords[2] = splits->piano->width;

  entry->highline =
    gnome_canvas_item_new (entry->splits->vline_group, GNOME_TYPE_CANVAS_LINE,
			   "fill-color-rgba", entry->splits->line_color,
			   "width-pixels", 1,
			   "points", points,
			   NULL);

  gnome_canvas_points_free (points);

  return (entry->span_control);
}

/**
 * swamigui_splits_entry_get_root_note_control:
 * @entry: Splits entry pointer
 * 
 * Get the root note control for a given splits entry.  The root note control is
 * created if it hasn't already been.
 * 
 * Returns: The root note control for the given @entry. The control has not been
 * referenced and is only valid while the GUI split exists.
 */
SwamiControl *
swamigui_splits_entry_get_root_note_control (SwamiguiSplitsEntry *entry)
{
  GnomeCanvasGroup *root;

  g_return_val_if_fail (entry != NULL, NULL);

  if (entry->rootnote_control) return (entry->rootnote_control);

  g_object_ref (entry->splits);	/* ++ ref the splits object for the control */
  g_atomic_int_inc (&entry->refcount);	/* ++ ref entry structure for rootnote_control */

  root = gnome_canvas_root (GNOME_CANVAS (entry->splits->low_canvas));

  entry->rootnote_control = swamigui_control_new (SWAMI_TYPE_CONTROL_FUNC);
  swami_control_set_spec (entry->rootnote_control,
		g_param_spec_int ("value", "value", "value",
				  0, 127, 60, G_PARAM_READWRITE));
  swami_control_set_value_type (entry->rootnote_control, G_TYPE_INT);
  swami_control_func_assign_funcs (SWAMI_CONTROL_FUNC (entry->rootnote_control),
				   swamigui_splits_root_note_control_get_func,
				   swamigui_splits_root_note_control_set_func,
				   swamigui_splits_root_note_control_destroy_func,
				   entry);
  entry->rootnote =
    gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_ELLIPSE,
			   "fill-color-rgba", entry->splits->root_note_color,
			   NULL);
  gnome_canvas_item_raise_to_top (entry->rootnote);

  return (entry->rootnote_control);
}

/* default splits handler */
static gboolean
swamigui_splits_default_handler (SwamiguiSplits *splits)
{
  SwamiguiSplitsEntry *entry;
  SwamiControl *span_ctrl, *prop_ctrl, *rootnote_ctrl;
  IpatchItem *splitsobj = NULL;
  IpatchList *children;
  IpatchIter iter;
  GValue value = { 0 };
  IpatchRange *range;
  const GType *types;
  GObject *obj;
  GList *sel = NULL;
  int splits_type;
  GObjectClass *klass;

  if (splits->status != SWAMIGUI_SPLITS_INIT
      && splits->status != SWAMIGUI_SPLITS_UPDATE
      && splits->status != SWAMIGUI_SPLITS_MODE
      && splits->status != SWAMIGUI_SPLITS_CHANGED)
    return (TRUE);

  if (!splits->selection) return (FALSE);

  ipatch_list_init_iter (splits->selection, &iter);
  obj = ipatch_iter_first (&iter);
  if (!obj) return (FALSE);

  /* either a single object with its "splits-type" type property set or multiple
     items with the same parent which has a "splits-type" property are handled */
  ipatch_type_object_get (obj, "splits-type", &splits_type, NULL);

  /* does not have splits-type set? */
  if (splits_type == IPATCH_SPLITS_NONE)
    {
      if (!IPATCH_IS_ITEM (obj)) return (FALSE); /* not handled if !IpatchItem */

      splitsobj = ipatch_item_get_parent (IPATCH_ITEM (obj));  /* ++ ref parent */
      if (!splitsobj) return (FALSE);	/* no parent, not handled */

      /* check if parent type supports splits, unhandled if not */
      ipatch_type_object_get (G_OBJECT (splitsobj),
			      "splits-type", &splits_type, NULL);

      if (splits_type == IPATCH_SPLITS_NONE)
	{
	  g_object_unref (splitsobj);	/* -- unref parent */
	  return (FALSE);
	}

      sel = g_list_prepend (sel, obj);	/* add to selection list */

      while ((obj = ipatch_iter_next (&iter)))
	{  /* selection unhandled if not IpatchItem or parent not the same */
	  if (!IPATCH_IS_ITEM (obj)
	      || ipatch_item_peek_parent ((IpatchItem *)obj) != splitsobj)
	    {
	      g_list_free (sel);
	      g_object_unref (splitsobj);	/* -- unref parent */
	      return (FALSE);
	    }

	  sel = g_list_prepend (sel, obj);	/* add to selection list */
	}
    } /* item has splits-type set, selection unhandled if there is another item */
  else
    {
      if (ipatch_iter_next (&iter)) return (FALSE);
      splitsobj = (IpatchItem *)g_object_ref (obj);	/* ++ ref to even things up */
    }

  /* clear and update splits if init, mode change or update with different obj */
  if (splits->status != SWAMIGUI_SPLITS_UPDATE
      || splits->splits_item != splitsobj)
    {
      swamigui_splits_remove_all (splits);

      /* set splits-item */
      g_object_set (splits, "splits-item", splitsobj, NULL);

      types = ipatch_container_get_child_types (IPATCH_CONTAINER (splitsobj));
      for (; *types; types++)	/* loop over child types */
	{
	  /* Verify this child type has velocity or note range property */
	  if (splits->mode == SWAMIGUI_SPLITS_VELOCITY)
	  {
	    klass = g_type_class_peek (*types);
	    if (!klass || !g_object_class_find_property (klass, "velocity-range"))
	      continue;
	  }
	  else
	  {
	    klass = g_type_class_peek (*types);
	    if (!klass || !g_object_class_find_property (klass, "note-range"))
	      continue;
	  }

	  /* ++ ref new list */
	  children = ipatch_container_get_children (IPATCH_CONTAINER (splitsobj),
						    *types);
	  ipatch_list_init_iter (children, &iter);
	  obj = ipatch_iter_first (&iter);

	  for (; obj; obj = ipatch_iter_next (&iter))  /* loop over child items */
	    {
	      g_value_init (&value, IPATCH_TYPE_RANGE);

	      if (splits->mode == SWAMIGUI_SPLITS_VELOCITY)
		g_object_get_property (obj, "velocity-range", &value);
	      else g_object_get_property (obj, "note-range", &value);

	      range = ipatch_value_get_range (&value);

	      /* skip objects with NULL range */
	      if (range->low == -1 && range->high == -1)
		{
		  g_value_unset (&value);
		  continue;
		}

	      entry = swamigui_splits_add (splits, obj);  /* add a new entry */
	      span_ctrl = swamigui_splits_entry_get_span_control (entry);

	      /* set current value of span, we do this instead of using the
		 SWAMI_CONTROL_CONN_INIT flag, since we already read the value */
	      swami_control_set_value (span_ctrl, &value);

	      /* ++ ref new key range property control */
	      if (splits->mode == SWAMIGUI_SPLITS_VELOCITY)
		prop_ctrl = swami_get_control_prop_by_name (obj, "velocity-range");
	      else prop_ctrl = swami_get_control_prop_by_name (obj, "note-range");

	      /* connect the key range property control to the span control */
	      swami_control_connect (prop_ctrl, span_ctrl,
				     SWAMI_CONTROL_CONN_BIDIR);

	      g_object_unref (prop_ctrl); /* -- unref property control */
	      g_value_unset (&value);

	      /* Add root note indicator if NOTE splits mode and has root-note property */
	      if (splits->mode == SWAMIGUI_SPLITS_NOTE
		  && g_object_class_find_property (klass, "root-note"))
	      {
		rootnote_ctrl = swamigui_splits_entry_get_root_note_control (entry);
		prop_ctrl = swami_get_control_prop_by_name (obj, "root-note");	/* ++ ref property control */
		swami_control_connect (prop_ctrl, rootnote_ctrl,
				       SWAMI_CONTROL_CONN_BIDIR
				       | SWAMI_CONTROL_CONN_INIT);
		g_object_unref (prop_ctrl);	/* -- unref prop control */
	      }
	    } /* while (obj) */
      
	  g_object_unref (children); /* -- unref children list */
	}	/* for (; *types; types++) */
    }

  g_object_unref (splitsobj);	/* -- unref splits object */

  swamigui_splits_select_items (splits, sel);
  g_list_free (sel);

  return (TRUE);
}

static GdkPixbuf *
swamigui_splits_create_velocity_gradient (void)
{
  guchar *linebuf;	/* generated gradient image data */
  float rval, gval, bval;
  float rinc, ginc, binc;
  gint i;

  /* allocate buffer space for one line of velocity bar (128 levels * RGB) */
  linebuf = g_new (guchar, 128 * 3);

  rval = velbar_scolor[0];
  gval = velbar_scolor[1];
  bval = velbar_scolor[2];

  rinc = (float) (velbar_ecolor[0] - rval + 1) / 128;
  ginc = (float) (velbar_ecolor[1] - gval + 1) / 128;
  binc = (float) (velbar_ecolor[2] - bval + 1) / 128;

  /* generate the gradient image data */

  for (i = 0; i < 128 * 3;)
    {
      linebuf[i++] = rval + 0.5;
      linebuf[i++] = gval + 0.5;
      linebuf[i++] = bval + 0.5;

      rval += rinc;
      gval += ginc;
      bval += binc;
    }

  return (gdk_pixbuf_new_from_data (linebuf, GDK_COLORSPACE_RGB, FALSE, 8,
				    128, 1, 128 * 3, gradient_data_free, NULL));
}

static void
gradient_data_free (guchar *pixels, gpointer data)
{
  g_free (pixels);
}
