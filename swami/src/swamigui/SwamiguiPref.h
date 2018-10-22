/*
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
/**
 * SECTION: SwamiguiPref
 * @short_description: Swami preferences widget and registration
 * @see_also:
 * @stability: Stable
 *
 * Swami GUI preferences widget and preference interface registration.
 */
#ifndef __SWAMIGUI_PREF_H__
#define __SWAMIGUI_PREF_H__

#include <gtk/gtk.h>

typedef struct _SwamiguiPref SwamiguiPref;
typedef struct _SwamiguiPrefClass SwamiguiPrefClass;

#define SWAMIGUI_TYPE_PREF   (swamigui_pref_get_type ())
#define SWAMIGUI_PREF(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_PREF, SwamiguiPref))
#define SWAMIGUI_PREF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_PREF, SwamiguiPrefClass))
#define SWAMIGUI_IS_PREF(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_PREF))
#define SWAMIGUI_IS_PREF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_PREF))

/* Swami preferences widget */
struct _SwamiguiPref
{
  GtkDialog parent;

  /*< private >*/
  GtkWidget *notebook;	/* invisible notebook with preference sections */
};

/* Swami preferences widget class */
struct _SwamiguiPrefClass
{
  GtkDialogClass parent_class;
};

/**
 * SwamiguiPrefHandler:
 *
 * Function prototype to create a GUI preference interface.
 *
 * Returns: The toplevel widget of the preference interface.
 */
typedef GtkWidget * (*SwamiguiPrefHandler)(void);

/**
 * SWAMIGUI_PREF_ORDER_NAME:
 *
 * Value to use for order parameter of swamigui_register_pref_handler() to
 * sort by name.  This should be used for plugins and other interfaces where
 * specific placement in the preferences list is not needed.
 */
#define SWAMIGUI_PREF_ORDER_NAME	0

void swamigui_register_pref_handler (const char *name, const char *icon,
				     int order, SwamiguiPrefHandler handler);

GType swamigui_pref_get_type (void);
GtkWidget *swamigui_pref_new (void);

#endif
