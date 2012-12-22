/*
 * SwamiguiTreeMenu.c - Swami Tree right-click menu object
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
#include <string.h>
#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

#include "SwamiguiItemMenu.h"
#include "SwamiguiRoot.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_SELECTION,	/* the item selection list */
  PROP_RIGHT_CLICK,	/* the right click item */
  PROP_CREATOR		/* the widget creating the menu */
};

typedef struct
{
  char *action_id;		/* store the hash key, for convenience */
  SwamiguiItemMenuInfo *info;
  SwamiguiItemMenuHandler handler;
} ActionBag;

typedef struct
{
  GType type;		/* type to match */
  gboolean derived;	/* TRUE if derived types should match also */
} TypeMatch;

static void swamigui_item_menu_class_init (SwamiguiItemMenuClass *klass);
static void type_match_list_free (gpointer data);
static void swamigui_item_menu_set_property (GObject *obj, guint property_id,
				      const GValue *value, GParamSpec *pspec);
static void swamigui_item_menu_get_property (GObject *obj, guint property_id,
					     GValue *value, GParamSpec *pspec);

static void swamigui_item_menu_init (SwamiguiItemMenu *menu);
static void swamigui_item_menu_finalize (GObject *object);
static ActionBag *lookup_item_action_bag (const char *action_id);
static void container_foreach_remove (GtkWidget *widg, gpointer data);
static void make_action_list_GHFunc (gpointer key, gpointer value,
				     gpointer user_data);
static void swamigui_item_menu_callback_activate (GtkMenuItem *mitem,
						  gpointer user_data);
static void swamigui_item_menu_accel_activate_callback (gpointer user_data);


/* keyboard accelerator group for item menu actions */
GtkAccelGroup *swamigui_item_menu_accel_group;

/* hash of action ID string -> ActionBag */
G_LOCK_DEFINE_STATIC (menu_action_hash);
static GHashTable *menu_action_hash = NULL;

/* hash of action ID string -> GSList of TypeMatch (for including types) */
G_LOCK_DEFINE_STATIC (item_type_include_hash);
static GHashTable *item_type_include_hash = NULL;

/* hash of action ID string -> GSList of TypeMatch (for excluding types) */
G_LOCK_DEFINE_STATIC (item_type_exclude_hash);
static GHashTable *item_type_exclude_hash = NULL;


static GObjectClass *parent_class = NULL;

void
_swamigui_item_menu_init (void)
{
  /* create key accelerator group */
  swamigui_item_menu_accel_group = gtk_accel_group_new ();

  /* create menu action hash */
  menu_action_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
					    NULL, (GDestroyNotify)g_free);

  /* create menu item type inclusion hash */
  item_type_include_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						  NULL, type_match_list_free);

  /* create menu item type exclusion hash */
  item_type_exclude_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						  NULL, type_match_list_free);
}

GType
swamigui_item_menu_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info =
      {
	sizeof (SwamiguiItemMenuClass), NULL, NULL,
	(GClassInitFunc) swamigui_item_menu_class_init, NULL, NULL,
	sizeof (SwamiguiItemMenu), 0,
	(GInstanceInitFunc) swamigui_item_menu_init,
      };

    obj_type = g_type_register_static (GTK_TYPE_MENU, "SwamiguiItemMenu",
				       &obj_info, 0);
  }

  return (obj_type);
}

static void
swamigui_item_menu_class_init (SwamiguiItemMenuClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = swamigui_item_menu_finalize;
  obj_class->get_property = swamigui_item_menu_get_property;
  obj_class->set_property = swamigui_item_menu_set_property;


  g_object_class_install_property (obj_class, PROP_SELECTION,
		    g_param_spec_object ("selection", "selection",
					 "selection", IPATCH_TYPE_LIST,
					 G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_RIGHT_CLICK,
		    g_param_spec_object ("right-click", "right-click",
					 "right-click", G_TYPE_OBJECT,
					 G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_CREATOR,
		    g_param_spec_object ("creator", "creator", "creator",
					 G_TYPE_OBJECT, G_PARAM_READWRITE));
}

/* free a GSList of TypeMatch structures */
static void
type_match_list_free (gpointer data)
{
  GSList *p = (GSList *)data;

  for (; p; p = g_slist_delete_link (p, p))
    g_free (p->data);
}

static void
swamigui_item_menu_set_property (GObject *obj, guint property_id,
				 const GValue *value, GParamSpec *pspec)
{
  SwamiguiItemMenu *menu = SWAMIGUI_ITEM_MENU (obj);
  GObject *object;

  switch (property_id)
    {
    case PROP_SELECTION:
      if (menu->selection) g_object_unref (menu->selection);
      object = g_value_get_object (value);
      g_return_if_fail (!object || IPATCH_IS_LIST (object));
      menu->selection = g_object_ref (object);
      break;
    case PROP_RIGHT_CLICK:
      if (menu->rclick) g_object_unref (menu->rclick);
      menu->rclick = g_value_dup_object (value);
      break;
    case PROP_CREATOR:
      if (menu->creator) g_object_unref (menu->creator);
      menu->creator = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
swamigui_item_menu_get_property (GObject *obj, guint property_id,
				 GValue *value, GParamSpec *pspec)
{
  SwamiguiItemMenu *menu = SWAMIGUI_ITEM_MENU (obj);

  switch (property_id)
    {
    case PROP_SELECTION:
      g_value_set_object (value, (GObject *)(menu->selection));
      break;
    case PROP_RIGHT_CLICK:
      g_value_set_object (value, (GObject *)(menu->rclick));
      break;
    case PROP_CREATOR:
      g_value_set_object (value, (GObject *)(menu->creator));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
swamigui_item_menu_init (SwamiguiItemMenu *menu)
{
  gtk_menu_set_accel_group (GTK_MENU (menu), swamigui_item_menu_accel_group);
}

static void
swamigui_item_menu_finalize (GObject *object)
{
  SwamiguiItemMenu *menu = SWAMIGUI_ITEM_MENU (object);

  if (menu->selection) g_object_unref (menu->selection);
  if (menu->rclick) g_object_unref (menu->rclick);
  if (menu->creator) g_object_unref (menu->creator);

  menu->selection = NULL;
  menu->rclick = NULL;
  menu->creator = NULL;

  if (parent_class->finalize)
    parent_class->finalize (object);
}

/**
 * swamigui_item_menu_new:
 *
 * Create a new Swami tree store.
 *
 * Returns: New Swami tree store object with a ref count of 1.
 */
SwamiguiItemMenu *
swamigui_item_menu_new (void)
{
  return (SWAMIGUI_ITEM_MENU (g_object_new (SWAMIGUI_TYPE_ITEM_MENU, NULL)));
}

/**
 * swamigui_item_menu_add:
 * @menu: GUI menu to add item to
 * @info: Info describing new menu item to add
 * @action_id: The action ID string that is adding the menu item
 *
 * Add a menu item to a GUI menu.
 *
 * Returns: The new GtkMenuItem that was added to the menu.
 */
GtkWidget *
swamigui_item_menu_add (SwamiguiItemMenu *menu,
			const SwamiguiItemMenuInfo *info, const char *action_id)
{
  GtkWidget *mitem;
  guint key, mods;
  char *accel_path;
  GList *list, *p;
  int order;
  int index;

  g_return_val_if_fail (SWAMIGUI_IS_ITEM_MENU (menu), NULL);
  g_return_val_if_fail (info != NULL, NULL);

  if (info->icon)
    {
      GtkWidget *image;

      mitem = gtk_image_menu_item_new_with_mnemonic (_(info->label));

      image = gtk_image_new_from_stock (info->icon, GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), image);
    }
  else mitem = gtk_menu_item_new_with_mnemonic (_(info->label));

  gtk_widget_show (mitem);

  g_object_set_data (G_OBJECT (mitem), "_order",
		     GUINT_TO_POINTER (info->order));
  g_object_set_data (G_OBJECT (mitem), "_func", (gpointer)(info->func));

  /* connect menu item to callback function */
  if (info->func)
    g_signal_connect (mitem, "activate",
		      G_CALLBACK (swamigui_item_menu_callback_activate),
		      info->data);

  /* parse key accelerator and add it to menu item */
  if (info->accel)
    {
      gtk_accelerator_parse (info->accel, &key, &mods);

      accel_path = g_strdup_printf ("<SwamiguiItemMenu>/%s", action_id);
      gtk_accel_map_add_entry (accel_path, key, mods);

      gtk_menu_item_set_accel_path (GTK_MENU_ITEM (mitem), accel_path);
      g_free (accel_path);
    }

  if (info->flags & SWAMIGUI_ITEM_MENU_INACTIVE)
    gtk_widget_set_sensitive (GTK_WIDGET (mitem), FALSE);

  list = gtk_container_get_children (GTK_CONTAINER (menu));

  /* find insert position from order info parameter */
  for (p = list, index = 0; p; p = p->next, index++)
    {
      order = GPOINTER_TO_INT (g_object_get_data (p->data, "_order"));
      if (info->order < order) break;
    }

  g_list_free (list);

  gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mitem, index);

  return (mitem);  
}

/**
 * swamigui_item_menu_add_registered_info:
 * @menu: GUI menu to add item to
 * @action_id: The action ID string that is adding the menu item
 *
 * Add a menu item to a GUI menu using the default info added when the
 * @action_id was registered.
 *
 * Returns: The new GtkMenuItem that was added to the menu.
 */
GtkWidget *
swamigui_item_menu_add_registered_info (SwamiguiItemMenu *menu,
					const char *action_id)
{
  ActionBag *found_action;

  g_return_val_if_fail (SWAMIGUI_IS_ITEM_MENU (menu), NULL);

  found_action = lookup_item_action_bag (action_id);
  g_return_val_if_fail (found_action != NULL, NULL);
  g_return_val_if_fail (found_action->info != NULL, NULL);

  return (swamigui_item_menu_add (menu, found_action->info, action_id));
}

/**
 * swamigui_item_menu_add_registered_info_inactive:
 * @menu: GUI menu to add item to
 * @action_id: The action ID string that is adding the menu item
 *
 * Add an inactive menu item to a GUI menu using the default info added when the
 * @action_id was registered.
 *
 * Returns: The new GtkMenuItem that was added to the menu.
 */
GtkWidget *
swamigui_item_menu_add_registered_info_inactive (SwamiguiItemMenu *menu,
					         const char *action_id)
{
  ActionBag *found_action;
  SwamiguiItemMenuInfo info;

  g_return_val_if_fail (SWAMIGUI_IS_ITEM_MENU (menu), NULL);

  found_action = lookup_item_action_bag (action_id);
  g_return_val_if_fail (found_action != NULL, NULL);
  g_return_val_if_fail (found_action->info != NULL, NULL);

  memcpy (&info, found_action->info, sizeof (info));
  info.flags |= SWAMIGUI_ITEM_MENU_INACTIVE;

  return (swamigui_item_menu_add (menu, &info, action_id));
}

/* lookup a registered action by its ID */
static ActionBag *
lookup_item_action_bag (const char *action_id)
{
  ActionBag *bag;

  G_LOCK (menu_action_hash);
  bag = g_hash_table_lookup (menu_action_hash, (gpointer)action_id);
  G_UNLOCK (menu_action_hash);

  return (bag);
}

/**
 * swamigui_item_menu_generate:
 * @menu: GUI menu
 *
 * Generate a GUI menu by executing all registered item action handlers which
 * add items to the menu.  Any existing items are removed before generating the
 * new menu.
 */
void
swamigui_item_menu_generate (SwamiguiItemMenu *menu)
{
  GSList *actions = NULL, *p;

  g_return_if_fail (SWAMIGUI_IS_ITEM_MENU (menu));

  /* remove any existing items from the menu */
  gtk_container_foreach (GTK_CONTAINER (menu), container_foreach_remove, menu);

  /* make a list so we can unlock the hash (prevent recursive locks) */
  G_LOCK (menu_action_hash);
  g_hash_table_foreach (menu_action_hash, make_action_list_GHFunc, &actions);
  G_UNLOCK (menu_action_hash);

  for (p = actions; p; p = p->next)
    {
      ActionBag *bag = (ActionBag *)(p->data);

      /* if handler was supplied then execute it, otherwise create item using
	 the item info set when action was registered (one or the other must be
	 set). */
      if (bag->handler)
	bag->handler (menu, bag->action_id);
      else swamigui_item_menu_add (menu, bag->info, bag->action_id);
    }

  g_slist_free (actions);
}

/* turn hash into a list */
static void
make_action_list_GHFunc (gpointer key, gpointer value, gpointer user_data)
{
  GSList **listp = (GSList **)user_data;
  *listp = g_slist_prepend (*listp, value);
}

static void
container_foreach_remove (GtkWidget *widg, gpointer data)
{
  gtk_container_remove (GTK_CONTAINER (data), widg);
}

/* callback when a menu item is activated */
static void
swamigui_item_menu_callback_activate (GtkMenuItem *mitem, gpointer user_data)
{
  SwamiguiItemMenuCallback callback;
  IpatchList *selection;

  callback = g_object_get_data (G_OBJECT (mitem), "_func");
  g_return_if_fail (callback != NULL);

  /* ++ ref selection */
  g_object_get (swamigui_root, "selection", &selection, NULL);

  if (!selection) return;

  (*callback)(selection, user_data);
  g_object_unref (selection);	/* -- unref selection */
}

/**
 * swamigui_register_item_menu_action:
 * @action_id: Menu item action ID (example: "paste", "new", "copy", etc),
 *   should be a static string since it is used directly.
 * @info: Menu item info (optional if @handler specified).  Structure is
 *   used directly and so it should be static as well as the strings inside.
 * @handler: Function to call when generating a menu (may be %NULL in which
 *   case the default handler is used).  Handler function should determine
 *   whether the registered action is appropriate for the current item
 *   selection and right click item and add menu items as appropriate.
 *
 * Registers a menu action.
 */
void
swamigui_register_item_menu_action (char *action_id,
				    SwamiguiItemMenuInfo *info,
				    SwamiguiItemMenuHandler handler)
{
  ActionBag *bag;
  guint key;
  GdkModifierType mods;
  GClosure *closure;

  g_return_if_fail (action_id != NULL && strlen(action_id) > 0);
  g_return_if_fail (info != NULL || handler != NULL);

  bag = g_new (ActionBag, 1);
  bag->action_id = action_id;
  bag->info = info;
  bag->handler = handler;

  G_LOCK (menu_action_hash);
  g_hash_table_insert (menu_action_hash, (gpointer)action_id, bag);
  G_UNLOCK (menu_action_hash);

  if (info->accel)
    {
      /* parse the accelerator */
      gtk_accelerator_parse (info->accel, &key, &mods);
    
      if (key != 0)	/* valid accelerator? */
	{ /* create closure for callback and add accelerator to accel group */
	  closure = g_cclosure_new_swap
	    ((GCallback)swamigui_item_menu_accel_activate_callback,
	     info, NULL);
	  gtk_accel_group_connect (swamigui_item_menu_accel_group, key, mods,
				   GTK_ACCEL_VISIBLE, closure);
	}
    }
}

static void
swamigui_item_menu_accel_activate_callback (gpointer user_data)
{
  SwamiguiItemMenuInfo *info = (SwamiguiItemMenuInfo *)user_data;
  IpatchList *selection;

  g_return_if_fail (info->func != NULL);

  g_object_get (swamigui_root, "selection", &selection, NULL);
  info->func (selection, info->data);
  g_object_unref (selection);
}

/**
 * swamigui_lookup_item_menu_action:
 * @action_id: ID string of item menu action to lookup.
 * @info: Location to store pointer to registered info (or %NULL to ignore).
 * @handler: Location to store pointer to registered handler (or %NULL to
 *   ignore).
 *
 * Lookup item action information registered by @action_id.
 *
 * Returns: %TRUE if action found by @action_id, %FALSE if not found.
 */
gboolean
swamigui_lookup_item_menu_action (const char *action_id,
				  SwamiguiItemMenuInfo **info,
				  SwamiguiItemMenuHandler *handler)
{
  ActionBag *bag;

  if (info) *info = NULL;
  if (handler) *handler = NULL;

  g_return_val_if_fail (action_id != NULL, FALSE);

  bag = lookup_item_action_bag (action_id);
  if (!bag) return (FALSE);

  if (info) *info = bag->info;
  if (handler) *handler = bag->handler;

  return (TRUE);
}

/**
 * swamigui_register_item_menu_include_type:
 * @action_id: The registered action ID string
 * @type: The type to add
 * @derived: Set to %TRUE if derived types should match also
 *
 * Adds a selection item type for inclusion for the given registered item
 * action.
 */
void
swamigui_register_item_menu_include_type (const char *action_id, GType type,
					  gboolean derived)
{
  TypeMatch *typematch;
  GSList *list;

  g_return_if_fail (action_id != NULL);
  g_return_if_fail (type != 0);

  typematch = g_new (TypeMatch, 1);
  typematch->type = type;
  typematch->derived = derived;

  G_LOCK (item_type_include_hash);

  list = g_hash_table_lookup (item_type_include_hash, action_id);
  if (!list)
    {
      list = g_slist_append (list, typematch);
      g_hash_table_insert (item_type_include_hash, (gpointer)action_id, list);
    }	// Assignment to keep warn_unused_result happy (append to non-empty list)
  else list = g_slist_append (list, typematch);

  G_UNLOCK (item_type_include_hash);
}

/**
 * swamigui_register_item_menu_exclude_type:
 * @action_id: The registered action ID string
 * @type: The type to add
 * @derived: Set to %TRUE if derived types should match also
 *
 * Adds a selection item type for exclusion for the given registered item
 * action.
 */
void
swamigui_register_item_menu_exclude_type (const char *action_id, GType type,
					  gboolean derived)
{
  TypeMatch *typematch;
  GSList *list;

  g_return_if_fail (action_id != NULL);
  g_return_if_fail (type != 0);

  typematch = g_new (TypeMatch, 1);
  typematch->type = type;
  typematch->derived = derived;

  G_LOCK (item_type_exclude_hash);

  list = g_hash_table_lookup (item_type_exclude_hash, action_id);
  if (!list)
    {
      list = g_slist_append (list, typematch);
      g_hash_table_insert (item_type_exclude_hash, (gpointer)action_id, list);
    }	// Assignment to keep warn_unused_result happy (append to non-empty list)
  else list = g_slist_append (list, typematch);

  G_UNLOCK (item_type_exclude_hash);
}

/**
 * swamigui_test_item_menu_type:
 * @action_id: Menu item action ID string
 * @type: Type to test
 *
 * Tests if a given item selection @type is in the include list and not in
 * the exclude list for @action_id.
 *
 * Returns: %TRUE if item is in include list and not in exclude list
 *    for @action_id, %FALSE otherwise.
 */
gboolean
swamigui_test_item_menu_type (const char *action_id, GType type)
{
  return (swamigui_test_item_menu_include_type (action_id, type)
	  && swamigui_test_item_menu_exclude_type (action_id, type));
}

/**
 * swamigui_test_item_menu_include_type:
 * @action_id: Menu item action ID string
 * @type: Type to test
 *
 * Tests if a given item selection @type is in the include list for @action_id.
 *
 * Returns: %TRUE if item is in include list for @action_id, %FALSE otherwise.
 */
gboolean
swamigui_test_item_menu_include_type (const char *action_id, GType type)
{
  TypeMatch *match;
  GSList *p;

  g_return_val_if_fail (action_id != NULL, FALSE);
  g_return_val_if_fail (type != 0, FALSE);

  G_LOCK (item_type_include_hash);

  p = g_hash_table_lookup (item_type_include_hash, action_id);
  for (; p; p = p->next)
    {
      match = (TypeMatch *)(p->data);

      if ((match->derived && g_type_is_a (type, match->type))
	  || (!match->derived && type == match->type))
	break;
    }

  G_UNLOCK (item_type_include_hash);

  return (p != NULL);
}

/**
 * swamigui_test_item_menu_exclude_type:
 * @action_id: Menu item action ID string
 * @type: Type to test
 *
 * Tests if a given item selection @type is not in the exclude list for
 * @action_id.
 *
 * Returns: %TRUE if item is not in exclude list for @action_id (should be
 *   included), %FALSE otherwise.
 */
gboolean
swamigui_test_item_menu_exclude_type (const char *action_id, GType type)
{
  TypeMatch *match;
  GSList *p;

  g_return_val_if_fail (action_id != NULL, FALSE);
  g_return_val_if_fail (type != 0, FALSE);

  G_LOCK (item_type_exclude_hash);

  p = g_hash_table_lookup (item_type_exclude_hash, action_id);
  for (; p; p = p->next)
    {
      match = (TypeMatch *)(p->data);

      if ((match->derived && g_type_is_a (type, match->type))
	  || (!match->derived && type == match->type))
	break;
    }

  G_UNLOCK (item_type_exclude_hash);

  return (p == NULL);
}

/**
 * swamigui_item_menu_get_selection_single:
 * @menu: GUI menu object
 *
 * Test if a menu object has a single selected item and return it if so.
 *
 * Returns: The single selected item object or %NULL if not a single selection.
 *   Returned object does NOT have a reference added and should be referenced
 *   if used outside of calling function.
 */
GObject *
swamigui_item_menu_get_selection_single (SwamiguiItemMenu *menu)
{
  g_return_val_if_fail (SWAMIGUI_IS_ITEM_MENU (menu), FALSE);

  if (menu->selection && menu->selection->items
      && !menu->selection->items->next)
    return (menu->selection->items->data);
  else return (NULL);
}

/**
 * swamigui_item_menu_get_selection:
 * @menu: GUI menu object
 *
 * Test if a menu object has any selected items and return the selection list
 * if so.
 *
 * Returns: Selected items or %NULL if no selection object.  No reference is
 * added for caller, so caller must reference if used outside of calling
 * context.
 */
IpatchList *
swamigui_item_menu_get_selection (SwamiguiItemMenu *menu)
{
  g_return_val_if_fail (SWAMIGUI_IS_ITEM_MENU (menu), NULL);

  if (menu->selection) return (menu->selection);
  return (NULL);
}

/**
 * swamigui_item_menu_handler_single:
 * @menu: The menu object
 * @action_id: The action ID
 *
 * A #SwamiguiItemMenuHandler type that can be used when registering a menu
 * action.  It will add a single menu item, using the info provided during
 * action registration, if a single item is selected and is of a type found
 * in the include type list and not found in exclude list.
 */
void
swamigui_item_menu_handler_single (SwamiguiItemMenu *menu,
				   const char *action_id)
{
  ActionBag *bag;
  GObject *item;

  g_return_if_fail (SWAMIGUI_IS_ITEM_MENU (menu));
  g_return_if_fail (action_id != NULL);

  /* make sure there is only 1 item selected */
  item = swamigui_item_menu_get_selection_single (menu);
  if (!item) return;

  /* item type is not in include list or in exclude list? - then return */
  if (!swamigui_test_item_menu_type (action_id, G_OBJECT_TYPE (item)))
    return;

  bag = lookup_item_action_bag (action_id);  /* lookup the registered action */

  /* add a menu item from the registered info */
  swamigui_item_menu_add (menu, bag->info, action_id);
}

/**
 * swamigui_item_menu_handler_multi:
 * @menu: The menu object
 * @action_id: The action ID
 *
 * A #SwamiguiItemMenuHandler type that can be used when registering a menu
 * action.  It will add a single menu item, using the info provided during
 * action registration, if a single item is selected and is of a type found
 * in the include type list and not in exclude list or multiple items are
 * selected.
 */
void
swamigui_item_menu_handler_multi (SwamiguiItemMenu *menu, const char *action_id)
{
  ActionBag *bag;
  IpatchList *list;
  GList *items;

  g_return_if_fail (SWAMIGUI_IS_ITEM_MENU (menu));
  g_return_if_fail (action_id != NULL);

  /* make sure there is at least 1 item selected */
  list = swamigui_item_menu_get_selection (menu);
  if (!list || !list->items) return;

  items = list->items;

  /* if only 1 item is selected.. */
  if (!items->next)
  {
    GObject *item = G_OBJECT (items->data);

    /* item type is not in include list or in exclude list? - then return */
    if (!swamigui_test_item_menu_type (action_id, G_OBJECT_TYPE (item)))
      return;
  }

  bag = lookup_item_action_bag (action_id);  /* lookup the registered action */

  /* add a menu item from the registered info */
  swamigui_item_menu_add (menu, bag->info, action_id);
}

/**
 * swamigui_item_menu_handler_single_all:
 * @menu: The menu object
 * @action_id: The action ID
 *
 * A #SwamiguiItemMenuHandler type that can be used when registering a menu
 * action.  It will add a single menu item, using the info provided during
 * action registration, if there is a single item selected of any type.
 */
void
swamigui_item_menu_handler_single_all (SwamiguiItemMenu *menu,
				       const char *action_id)
{
  ActionBag *bag;

  g_return_if_fail (SWAMIGUI_IS_ITEM_MENU (menu));
  g_return_if_fail (action_id != NULL);

  /* make sure there is only 1 item selected */
  if (!swamigui_item_menu_get_selection_single (menu)) return;

  bag = lookup_item_action_bag (action_id);  /* lookup the registered action */

  /* add a menu item from the registered info */
  swamigui_item_menu_add (menu, bag->info, action_id);
}

/**
 * swamigui_item_menu_handler_multi_all:
 * @menu: The menu object
 * @action_id: The action ID
 *
 * A #SwamiguiItemMenuHandler type that can be used when registering a menu
 * action.  It will add a single menu item, using the info provided during
 * action registration, if there is at least one item selected of any type.
 */
void
swamigui_item_menu_handler_multi_all (SwamiguiItemMenu *menu,
				      const char *action_id)
{
  ActionBag *bag;
  IpatchList *list;

  g_return_if_fail (SWAMIGUI_IS_ITEM_MENU (menu));
  g_return_if_fail (action_id != NULL);

  /* make sure there is at least 1 item selected */
  list = swamigui_item_menu_get_selection (menu);
  if (!list || !list->items) return;

  bag = lookup_item_action_bag (action_id);  /* lookup the registered action */

  /* add a menu item from the registered info */
  swamigui_item_menu_add (menu, bag->info, action_id);
}
