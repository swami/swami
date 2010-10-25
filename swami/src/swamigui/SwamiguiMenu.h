/*
 * SwamiguiMenu.h - Swami main menu object
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
#ifndef __SWAMIGUI_MENU_H__
#define __SWAMIGUI_MENU_H__

typedef struct _SwamiguiMenu SwamiguiMenu;
typedef struct _SwamiguiMenuClass SwamiguiMenuClass;

#include <gtk/gtk.h>

#include "SwamiguiRoot.h"

#define SWAMIGUI_TYPE_MENU   (swamigui_menu_get_type ())
#define SWAMIGUI_MENU(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_MENU, SwamiguiMenu))
#define SWAMIGUI_MENU_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_MENU, SwamiguiMenuClass))
#define SWAMIGUI_IS_MENU(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_MENU))
#define SWAMIGUI_IS_MENU_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_MENU))

/* Swami Menu object */
struct _SwamiguiMenu
{
  GtkVBox parent_instance;

  GtkUIManager *ui;
};

struct _SwamiguiMenuClass
{
  GtkVBoxClass parent_class;
};

GType swamigui_menu_get_type (void);
GtkWidget *swamigui_menu_new (void);

#endif
