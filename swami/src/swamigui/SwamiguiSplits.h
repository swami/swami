/*
 * SwamiguiSplits.h - Key/velocity splits widget header file
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
#ifndef __SWAMIGUI_SPLITS_H__
#define __SWAMIGUI_SPLITS_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>

typedef struct _SwamiguiSplits SwamiguiSplits;
typedef struct _SwamiguiSplitsClass SwamiguiSplitsClass;
typedef struct _SwamiguiSplitsEntry SwamiguiSplitsEntry;

#include "SwamiguiPiano.h"

#define SWAMIGUI_TYPE_SPLITS   (swamigui_splits_get_type ())
#define SWAMIGUI_SPLITS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_SPLITS, SwamiguiSplits))
#define SWAMIGUI_SPLITS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_SPLITS, \
   SwamiguiSplitsClass))
#define SWAMIGUI_IS_SPLITS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_SPLITS))
#define SWAMIGUI_IS_SPLITS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_SPLITS))

/* number of white keys in MIDI 128 note range */
#define SWAMIGUI_SPLITS_WHITE_KEY_COUNT 75

/* splits mode */
typedef enum
{
  SWAMIGUI_SPLITS_NOTE,
  SWAMIGUI_SPLITS_VELOCITY
} SwamiguiSplitsMode;

typedef enum
{
  SWAMIGUI_SPLITS_NORMAL,	/* no particular status */
  SWAMIGUI_SPLITS_INIT,		/* check selection and initialize splits */
  SWAMIGUI_SPLITS_MODE,		/* note/velocity mode change */
  SWAMIGUI_SPLITS_UPDATE,	/* selection changed */
  SWAMIGUI_SPLITS_CHANGED       /* splits-item changed */
} SwamiguiSplitsStatus;

/**
 * SwamiguiSplitsMoveFlags:
 * @SWAMIGUI_SPLITS_MOVE_RANGES: Move note ranges
 * @SWAMIGUI_SPLITS_MOVE_PARAM1: Move parameter 1 (default is root notes)
 */
typedef enum
{
  SWAMIGUI_SPLITS_MOVE_RANGES = 1 << 0,
  SWAMIGUI_SPLITS_MOVE_PARAM1 = 1 << 1
} SwamiguiSplitsMoveFlags;

/**
 * SwamiguiSplitsHandler:
 * @splits: Splits object
 *
 * This function type is used to handle specific patch item types with note
 * or velocity split parameters. The @splits object
 * <structfield>status</structfield> field indicates the current operation
 * which is one of:
 *
 * %SWAMIGUI_SPLITS_INIT - Check selection and install splits and note
 * pointers if the selection can be handled. Return %TRUE if selection
 * was handled, which will activate this handler, %FALSE otherwise.
 *
 * %SWAMIGUI_SPLITS_MODE - Split mode change (from note to velocity mode for
 * example). Re-configure splits and note pointers. Return %TRUE if mode
 * change was handled, %FALSE otherwise which will de-activate this handler.
 *
 * %SWAMIGUI_SPLITS_UPDATE - Item selection has changed, update splits and
 * note pointers. Return %TRUE if selection change was handled, %FALSE
 * otherwise which will de-activate this handler.
 *
 * Other useful fields of a #SwamiguiSplits object include
 * <structfield>mode</structfield> which defines the current mode
 * (#SwamiguiSplitsMode) and <structfield>selection</structfield> which
 * defines the current item selection.
 *
 * Returns: Should return %TRUE if operation was handled, %FALSE otherwise.
 */
typedef gboolean (*SwamiguiSplitsHandler)(SwamiguiSplits *splits);

/* Swami Splits Object */
struct _SwamiguiSplits
{
  GtkVBox parent_instance;	/* derived from GtkVBox */

  /*< public >*/

  SwamiguiSplitsStatus status;	/* current status (for handlers) */
  int mode;			/* current mode (SWAMIGUI_SPLITS_NOTE or
				   SWAMIGUI_SPLITS_VELOCITY) */
  int move_flags;		/* current move flags (SwamiguiSplitsMoveFlags) */
  IpatchList *selection;        /* selected items (parent OR child splits) */
  IpatchItem *splits_item;      /* active item which contains splits */
  SwamiguiSplitsHandler handler;	/* active splits handler or NULL */
  gpointer handler_data;	/* handler defined pointer */

  /*< private >*/

  GtkWidget *gladewidg;         /* The embedded glade widget */
  GtkWidget *top_canvas;	/* piano/velocity/pointers canvas */
  GtkWidget *low_canvas;	/* lower spans canvas */
  GtkWidget *vertical_scrollbar; /* vertical scroll bar */

  GtkWidget *notes_btn;		/* Notes toggle button */
  GtkWidget *velocity_btn;	/* Velocity toggle button */
  GtkWidget *move_combo;	/* Move combo box */

  gboolean width_set;	/* TRUE when width set, FALSE to resize to window */

  GnomeCanvasGroup *vline_group; /* vertical line group */

  SwamiguiPiano *piano;		/* piano canvas item */
  GnomeCanvasItem *velgrad;	/* velocity gradient canvas item */
  GnomeCanvasItem *bgrect;	/* lower canvas background rectangle */

  int flags;			/* some flags for optimized updating */

  GList *entry_list; 		/* list of SwamiguiSplitsEntry */
  guint entry_count;		/* count of entries in entry_list */

  int active_drag; 		/* The active drag mode (see ActiveDrag) */
  int active_drag_btn;		/* The mouse button which caused the drag */
  int anchor;			/* last clicked split index or -1 */
  double active_xpos;		/* X coordinate of active click */
  double threshold_value;	/* threshold pixel movement value */
  GList *active_split;		/* split being edited (->data = SwamiguiSplitsEntry) */
  int move_note_ofs;		/* middle click move note click offset */

  int height;			/* total height of splits lower canvas */
  int width;			/* Width of splits canvas */
  int span_height;		/* height of span handles */
  int span_spacing;		/* vertical spacing between splits */
  int vert_lines_width;		/* width of background vertical lines */
  int move_threshold;		/* movement threshold */

  guint bg_color;		/* background color */
  guint span_color;		/* span color */
  guint span_sel_color;		/* selected span color */
  guint span_outline_color;	/* span outline color */
  guint span_sel_outline_color;	/* selected span outline color */
  guint line_color;		/* line color */
  guint line_sel_color;		/* selected line color */
  guint root_note_color;	/* root note indicator color */
};

struct _SwamiguiSplitsClass
{
  GtkVBoxClass parent_class;
};

GType swamigui_splits_get_type (void);
GtkWidget *swamigui_splits_new (void);

void swamigui_splits_set_mode (SwamiguiSplits *splits, SwamiguiSplitsMode mode);
void swamigui_splits_set_width (SwamiguiSplits *splits, int width);
void swamigui_splits_set_selection (SwamiguiSplits *splits, IpatchList *items);
IpatchList *swamigui_splits_get_selection (SwamiguiSplits *splits);

void swamigui_splits_select_items (SwamiguiSplits *splits, GList *items);
void swamigui_splits_select_all (SwamiguiSplits *splits);
void swamigui_splits_unselect_all (SwamiguiSplits *splits);

void swamigui_splits_item_changed (SwamiguiSplits *splits);

void swamigui_splits_register_handler (SwamiguiSplitsHandler handler);
void swamigui_splits_unregister_handler (SwamiguiSplitsHandler handler);

#define swamigui_splits_add(splits, item) \
    swamigui_splits_insert (splits, item, -1)
SwamiguiSplitsEntry *swamigui_splits_insert (SwamiguiSplits *splits,
					     GObject *item, int index);
SwamiguiSplitsEntry *swamigui_splits_lookup_entry (SwamiguiSplits *splits,
						   GObject *item);
void swamigui_splits_remove (SwamiguiSplits *splits, GObject *item);
void swamigui_splits_remove_all (SwamiguiSplits *splits);
void swamigui_splits_set_span_range (SwamiguiSplits *splits, GObject *item,
				     int low, int high);
void swamigui_splits_set_root_note (SwamiguiSplits *splits, GObject *item, int val);

SwamiControl *swamigui_splits_entry_get_span_control (SwamiguiSplitsEntry *entry);
SwamiControl *swamigui_splits_entry_get_root_note_control (SwamiguiSplitsEntry *entry);
int swamigui_splits_entry_get_index (SwamiguiSplitsEntry *entry);

#endif
