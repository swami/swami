/*
 * SwamiguiBar.h - Bar canvas item
 * A horizontal bar canvas item for displaying pointers or ranges.
 *
 * Swami
 * Copyright (C) 1999-2018 Element Green <element@elementsofsound.org>
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
#ifndef __SWAMIGUI_BAR_H__
#define __SWAMIGUI_BAR_H__

#include <gtk/gtk.h>

typedef struct _SwamiguiBar SwamiguiBar;
typedef struct _SwamiguiBarClass SwamiguiBarClass;

#include "SwamiguiBarPtr.h"

#define SWAMIGUI_TYPE_BAR   (swamigui_bar_get_type ())
#define SWAMIGUI_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_BAR, SwamiguiBar))
#define SWAMIGUI_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_BAR, SwamiguiBarClass))
#define SWAMIGUI_IS_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_BAR))
#define SWAMIGUI_IS_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_BAR))

/**
 * SwamiguiBarOverlapPos:
 * @SWAMIGUI_BAR_OVERLAP_POS_TOP: Overlaps are shown at the top of the widget
 * @SWAMIGUI_BAR_OVERLAP_POS_BOTTOM: Overlaps are shown at the bottom of the widget
 *
 * Position where pointer overlaps are indicated.
 */
typedef enum
{
  SWAMIGUI_BAR_OVERLAP_POS_TOP,
  SWAMIGUI_BAR_OVERLAP_POS_BOTTOM
} SwamiguiBarOverlapPos;

/* Bar Object */
struct _SwamiguiBar
{
  GtkDrawingArea parent_instance;

  /*< private >*/

  int height;			/* Height of bar */
  int overlap_pos;		/* Where pointer overlaps are shown (SwamiguiBarOverlapPos) */
  int overlap_height;		/* Height of overlaps in pixels */
  GList *ptrlist;		/* list of PtrInfo structs (SwamiguiBar.c) - top to bottom order */  

  gpointer move_pointer;      /* Active pointer being moved (PtrInfo *) or NULL */
  gboolean move_toler_done;   /* Pointer move already satisfied move tolerance for activation? */
  int move_click_xpos;	      /* Active move X coordinate of click */
  int move_click_xofs;	      /* Active move X coordinate to pointer start offset of click */
  int move_sel;		      /* -1 for left range, 0 for entire range or pointer, 1 for right range */
};

struct _SwamiguiBarClass
{
  GtkDrawingAreaClass parent_class;

  void (* pointer_changed)(SwamiguiBar *bar, const char *id, int start, int end);
};


GType swamigui_bar_get_type (void);
GtkWidget *swamigui_bar_new (void);
void swamigui_bar_create_pointer (SwamiguiBar *bar, const char *id,
				  const char *first_property_name, ...);
SwamiguiBarPtr *swamigui_bar_get_pointer (SwamiguiBar *bar, const char *id);
void swamigui_bar_set_pointer_position (SwamiguiBar *bar, const char *id,
					int position);
void swamigui_bar_set_pointer_range (SwamiguiBar *bar, const char *id,
				     int start, int end);
int swamigui_bar_get_pointer_order (SwamiguiBar *bar, const char *id);
void swamigui_bar_set_pointer_order (SwamiguiBar *bar, const char *id, int pos);
void swamigui_bar_raise_pointer_to_top (SwamiguiBar *bar, const char *id);
void swamigui_bar_lower_pointer_to_bottom (SwamiguiBar *bar, const char *id);

#endif
