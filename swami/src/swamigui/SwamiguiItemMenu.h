/*
 * SwamiguiItemMenu.h - Swami item action (right click) menu routines
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
#ifndef __SWAMIGUI_ITEM_MENU_H__
#define __SWAMIGUI_ITEM_MENU_H__

#include <gtk/gtk.h>

typedef struct _SwamiguiItemMenu SwamiguiItemMenu;
typedef struct _SwamiguiItemMenuClass SwamiguiItemMenuClass;
typedef struct _SwamiguiItemMenuInfo SwamiguiItemMenuInfo;
typedef struct _SwamiguiItemMenuEntry SwamiguiItemMenuEntry;

#define SWAMIGUI_TYPE_ITEM_MENU   (swamigui_item_menu_get_type ())
#define SWAMIGUI_ITEM_MENU(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_ITEM_MENU, \
   SwamiguiItemMenu))
#define SWAMIGUI_ITEM_MENU_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_ITEM_MENU, \
   SwamiguiItemMenuClass))
#define SWAMIGUI_IS_ITEM_MENU(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_ITEM_MENU))
#define SWAMIGUI_IS_ITEM_MENU_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_ITEM_MENU))

/**
 * SwamiguiItemMenuCallback:
 * @selection: Item selection
 * @data: Data defined by the menu item
 *
 * A callback function type that is used when a menu item is activated.
 */
typedef void (*SwamiguiItemMenuCallback)(IpatchList *selection, gpointer data);

/**
 * SwamiguiItemMenuHandler:
 * @menu: GUI menu object
 * @action_id: Menu action ID (set when action was registered)
 *
 * A handler for a menu item type.  Called when generating a menu for an
 * item selection and right click item.  This function should determine if
 * its action type (example: paste, delete, copy, new, etc) is valid for the
 * given selection and add one or more menu items if so.
 */
typedef void (*SwamiguiItemMenuHandler)(SwamiguiItemMenu *menu,
					const char *action_id);

typedef enum  /*< flags >*/
{
  SWAMIGUI_ITEM_MENU_INACTIVE = 1 << 0,	/* set if menu item should be inactive */
  SWAMIGUI_ITEM_MENU_PLUGIN   = 1 << 1	/* set if menu item is for a plugin */
} SwamiguiItemMenuFlags;

/* menu item info */
struct _SwamiguiItemMenuInfo
{
  guint order; /* an integer used to sort items (lower values first) */
  char *label;			/* menu label text */
  char *accel;			/* key accelerator */
  char *icon;			/* stock ID of icon */
  guint flags;			/* SwamiguiItemMenuFlags */
  SwamiguiItemMenuCallback func; /* function to call when item is activated */
  gpointer data;		/* data to pass to callback function */
};

struct _SwamiguiItemMenu
{
  GtkMenu parent_instance;

  IpatchList *selection;	/* current item selection or NULL */
  GObject *rclick;		/* current right click item or NULL */
  GObject *creator; /* object that created menu (SwamiguiTree for example) */
};

struct _SwamiguiItemMenuClass
{
  GtkMenuClass parent_class;
};


extern GtkAccelGroup *swamigui_item_menu_accel_group;


GType swamigui_item_menu_get_type ();
SwamiguiItemMenu *swamigui_item_menu_new ();
GtkWidget *swamigui_item_menu_add (SwamiguiItemMenu *menu,
				   const SwamiguiItemMenuInfo *info,
				   const char *action_id);
GtkWidget *swamigui_item_menu_add_registered_info (SwamiguiItemMenu *menu,
						   const char *action_id);
GtkWidget *swamigui_item_menu_add_registered_info_inactive (SwamiguiItemMenu *menu,
                                                            const char *action_id);

void swamigui_item_menu_generate (SwamiguiItemMenu *menu);

void swamigui_register_item_menu_action (char *action_id,
				         SwamiguiItemMenuInfo *info,
				         SwamiguiItemMenuHandler handler);
gboolean swamigui_lookup_item_menu_action (const char *action_id,
					   SwamiguiItemMenuInfo **info,
					   SwamiguiItemMenuHandler *handler);
void swamigui_register_item_menu_include_type (const char *action_id,
					       GType type, gboolean derived);
void swamigui_register_item_menu_exclude_type (const char *action_id,
					       GType type, gboolean derived);
gboolean swamigui_test_item_menu_type (const char *action_id, GType type);
gboolean swamigui_test_item_menu_include_type (const char *action_id,
					       GType type);
gboolean swamigui_test_item_menu_exclude_type (const char *action_id,
					       GType type);
GObject *swamigui_item_menu_get_selection_single (SwamiguiItemMenu *menu);
IpatchList *swamigui_item_menu_get_selection (SwamiguiItemMenu *menu);

void swamigui_item_menu_handler_single (SwamiguiItemMenu *menu,
					const char *action_id);
void swamigui_item_menu_handler_multi (SwamiguiItemMenu *menu,
				       const char *action_id);
void swamigui_item_menu_handler_single_all (SwamiguiItemMenu *menu,
					    const char *action_id);
void swamigui_item_menu_handler_multi_all (SwamiguiItemMenu *menu,
					   const char *action_id);

#endif
