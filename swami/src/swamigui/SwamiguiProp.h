/*
 * SwamiguiProp.h - GObject property GUI control object header
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
#ifndef __SWAMIGUI_PROP_H__
#define __SWAMIGUI_PROP_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiProp SwamiguiProp;
typedef struct _SwamiguiPropClass SwamiguiPropClass;

#define SWAMIGUI_TYPE_PROP   (swamigui_prop_get_type ())
#define SWAMIGUI_PROP(obj) \
  (GTK_CHECK_CAST ((obj), SWAMIGUI_TYPE_PROP, SwamiguiProp))
#define SWAMIGUI_PROP_CLASS(klass) \
  (GTK_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_PROP, SwamiguiPropClass))
#define SWAMIGUI_IS_PROP(obj) \
  (GTK_CHECK_TYPE ((obj), SWAMIGUI_TYPE_PROP))
#define SWAMIGUI_IS_PROP_CLASS(klass) \
  (GTK_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_PROP))

/* Swami Properties Object (all fields private) */
struct _SwamiguiProp
{
  GtkScrolledWindow parent;
  GtkWidget *viewport;		/* viewport for child interface widget */
  IpatchList *selection;	/* Selection list or NULL (one item only) */
};

/* Swami Properties Object class (all fields private) */
struct _SwamiguiPropClass
{
  GtkScrolledWindowClass parent_class;
};

/**
 * SwamiguiPropHandler:
 * @widg: A previously created widget (if changing @obj) or %NULL if a new
 *   widget should be created.
 * @obj: Object to create a GUI property control interface for.
 *
 * Function prototype to create a GUI property control interface.
 *
 * Returns: The toplevel widget of the interface which controls @obj.
 */
typedef GtkWidget * (*SwamiguiPropHandler)(GtkWidget *widg, GObject *obj);

void swamigui_register_prop_glade_widg (GType objtype, const char *name);
void swamigui_register_prop_handler (GType objtype, SwamiguiPropHandler handler);

GType swamigui_prop_get_type (void);
GtkWidget *swamigui_prop_new (void);
void swamigui_prop_set_selection (SwamiguiProp *prop, IpatchList *selection);

#endif
