/*
 * SwamiguiBarPtr.h - Pointer object added to SwamiguiBar canvas items
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
#ifndef __SWAMIGUI_BAR_PTR_H__
#define __SWAMIGUI_BAR_PTR_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

typedef struct _SwamiguiBarPtr SwamiguiBarPtr;
typedef struct _SwamiguiBarPtrClass SwamiguiBarPtrClass;

#define SWAMIGUI_TYPE_BAR_PTR   (swamigui_bar_ptr_get_type ())
#define SWAMIGUI_BAR_PTR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_BAR_PTR, SwamiguiBarPtr))
#define SWAMIGUI_BAR_PTR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_BAR_PTR, SwamiguiBarPtrClass))
#define SWAMIGUI_IS_BAR_PTR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_BAR_PTR))
#define SWAMIGUI_IS_BAR_PTR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_BAR_PTR))

/* Pointer type */
typedef enum
{
  SWAMIGUI_BAR_PTR_POSITION,	/* pointer position indicator */
  SWAMIGUI_BAR_PTR_RANGE	/* range selection */
} SwamiguiBarPtrType;

/* Bar pointer object */
struct _SwamiguiBarPtr
{
  GnomeCanvasGroup parent_instance;

  /*< private >*/

  GnomeCanvasItem *rect;	/* pointer rectangle */
  GnomeCanvasItem *ptr;		/* triangle pointer (if SWAMIGUI_BAR_POINTER) */
  GnomeCanvasItem *icon;	/* icon for this pointer */

  int width;			/* width in pixels */
  int height;			/* height in pixels */
  int pointer_height;		/* height of pointer in pixels */
  SwamiguiBarPtrType type;	/* pointer interface type */
  gboolean interactive;		/* TRUE if user can change this pointer */
  guint32 color;
  char *label;
  char *tooltip;
};

struct _SwamiguiBarPtrClass
{
  GnomeCanvasGroupClass parent_class;
};


GType swamigui_bar_ptr_get_type (void);
GnomeCanvasItem *swamigui_bar_ptr_new (void);

#endif
