/*
 * SwamiguiTreeStore.c - Swami item tree store object
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

#include <libswami/libswami.h>

#include "SwamiguiTree.h"
#include "SwamiguiRoot.h"
#include "icons.h"
#include "i18n.h"


/* Local Prototypes */

static void swamigui_tree_store_class_init (SwamiguiTreeStoreClass *klass);
static void swamigui_tree_store_init (SwamiguiTreeStore *store);
static void swamigui_tree_store_finalize (GObject *object);
static inline void swamigui_tree_finish_insert (SwamiguiTreeStore *store,
						GObject *item,
						const char *label,
						char *icon,
						GtkTreeIter *iter);
static void tree_store_recursive_remove (SwamiguiTreeStore *store,
					 GtkTreeIter *iter);

static GObjectClass *parent_class = NULL;

/* a cache of icon names (conserve memory, no need to store for every item) */
static GHashTable *icon_name_cache = NULL;

GType
swamigui_tree_store_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info =
      {
	sizeof (SwamiguiTreeStoreClass), NULL, NULL,
	(GClassInitFunc) swamigui_tree_store_class_init, NULL, NULL,
	sizeof (SwamiguiTreeStore), 0,
	(GInstanceInitFunc) swamigui_tree_store_init,
      };

    obj_type = g_type_register_static (GTK_TYPE_TREE_STORE, "SwamiguiTreeStore",
				       &obj_info, G_TYPE_FLAG_ABSTRACT);
  }

  return (obj_type);
}

static void
swamigui_tree_store_class_init (SwamiguiTreeStoreClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = swamigui_tree_store_finalize;

  icon_name_cache = g_hash_table_new_full (NULL, NULL,
					   (GDestroyNotify)g_free, NULL);
}

static void
swamigui_tree_store_init (SwamiguiTreeStore *store)
{
  GType types[SWAMIGUI_TREE_STORE_NUM_COLUMNS] =
    { G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_OBJECT };

  /* we use pointer type for icon since its a static string and we don't want
     it getting duplicated */

  /* set up the tree store */
  gtk_tree_store_set_column_types (GTK_TREE_STORE (store),
				   SWAMIGUI_TREE_STORE_NUM_COLUMNS, types);

  /* create a item->tree_node hash with a destroy notify on the item key to
     unref it, and a destroy on the tree iterator to free it */
  store->item_hash =
    g_hash_table_new_full (NULL, NULL,
			   (GDestroyNotify)g_object_unref,
			   (GDestroyNotify)gtk_tree_iter_free);
}

static void
swamigui_tree_store_finalize (GObject *object)
{
  SwamiguiTreeStore *store = SWAMIGUI_TREE_STORE (object);

  g_hash_table_destroy (store->item_hash);

  if (parent_class->finalize)
    parent_class->finalize (object);
}

/**
 * swamigui_tree_store_insert:
 * @store: Swami tree store to insert tree node into
 * @item: Object to link to tree node or %NULL
 * @label: Label for node or %NULL to try and obtain it other ways.
 * @icon: Stock ID of icon (%NULL to use "icon" type property or category icon)
 * @parent: Pointer to parent tree node or %NULL if inserting a toplevel node
 * @pos: Position to insert new node at (0 = prepend, > sibling list to append)
 * @out_iter: Pointer to a user supplied GtkTreeIter structure to store the
 *   new node in or %NULL to ignore.
 *
 * Creates a new tree node and inserts it at the position given by
 * @parent and @pos, returning the created node in @out_iter (if
 * supplied). @item is an object to link with the created node.
 * It is also used if either @label or @icons is %NULL in which case
 * it obtains the information via other methods.
 */
void
swamigui_tree_store_insert (SwamiguiTreeStore *store, GObject *item,
			    const char *label, char *icon,
			    GtkTreeIter *parent, int pos, GtkTreeIter *out_iter)
{
  GtkTreeIter iter;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (!item || G_IS_OBJECT (item));

  gtk_tree_store_insert (GTK_TREE_STORE (store), &iter, parent, pos);
  swamigui_tree_finish_insert (store, item, label, icon, &iter);

  if (out_iter) *out_iter = iter;
}

/**
 * swamigui_tree_store_insert_before:
 * @store: Swami tree store to insert tree node into
 * @item: Object to link to tree node or %NULL
 * @label: Label for node or %NULL to try and obtain it other ways.
 * @icon: Stock ID of icon (%NULL to use "icon" type property or category icon)
 * @parent: Pointer to parent tree node or %NULL if inserting a toplevel node
 * @sibling: A sibling node to insert the new node before or %NULL to append
 * @out_iter: Pointer to a user supplied GtkTreeIter structure to store the
 *   new node in or %NULL to ignore.
 *
 * Like swamigui_tree_store_insert() but inserts the node before the
 * specified @sibling instead.
 */
void
swamigui_tree_store_insert_before (SwamiguiTreeStore *store, GObject *item,
				   const char *label, char *icon,
				   GtkTreeIter *parent, GtkTreeIter *sibling,
				   GtkTreeIter *out_iter)
{
  GtkTreeIter iter;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (!item || G_IS_OBJECT (item));

  gtk_tree_store_insert_before (GTK_TREE_STORE (store), &iter,
				parent, sibling);
  swamigui_tree_finish_insert (store, item, label, icon, &iter);

  if (out_iter) *out_iter = iter;
}

/**
 * swamigui_tree_store_insert_after:
 * @store: Swami tree store to insert tree node into
 * @item: Object to link to tree node or %NULL
 * @label: Label for node or %NULL to try and obtain it other ways.
 * @icon: Stock ID of icon (%NULL to use "icon" type property or category icon)
 * @parent: Pointer to parent tree node or %NULL if inserting a toplevel node
 * @sibling: A sibling node to insert the new node after or %NULL to prepend
 * @out_iter: Pointer to a user supplied GtkTreeIter structure to store the
 *   new node in or %NULL to ignore.
 *
 * Like swamigui_tree_store_insert() but inserts the node after the
 * specified @sibling instead.
 */
void
swamigui_tree_store_insert_after (SwamiguiTreeStore *store, GObject *item,
				  const char *label, char *icon,
				  GtkTreeIter *parent, GtkTreeIter *sibling,
				  GtkTreeIter *out_iter)
{
  GtkTreeIter iter;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (!item || G_IS_OBJECT (item));

  gtk_tree_store_insert_after (GTK_TREE_STORE (store), &iter, parent, sibling);
  swamigui_tree_finish_insert (store, item, label, icon, &iter);

  if (out_iter) *out_iter = iter;
}

static inline void
swamigui_tree_finish_insert (SwamiguiTreeStore *store, GObject *item,
			     const char *label, char *icon,
			     GtkTreeIter *iter)
{
  char *item_label = NULL;
  char *item_icon = NULL;
  GtkTreeIter *copy;
  gint category;

  if (!label)
    {
      if (IPATCH_IS_ITEM (item)) g_object_get (item, "title", &item_label, NULL);
      else swami_object_get (item, "name", &item_label, NULL);

      if (!item_label) ipatch_type_object_get (item, "name", &item_label, NULL);
      if (!item_label) item_label = g_strdup (_("Untitled"));
      label = item_label;
    }

  if (!icon)
    { /* get item icon from type property */
      ipatch_type_object_get (G_OBJECT (item), "icon", &item_icon,
			      "category", &category,
			      NULL);
      if (item_icon)
	{ /* lookup icon name in hash (no need to store name for every icon) */
	  icon = g_hash_table_lookup (icon_name_cache, item_icon);
	  if (!icon)
	    {
	      icon = g_strdup (item_icon);
	      g_hash_table_insert (icon_name_cache, icon, icon);
	    }

	  g_free (item_icon);
	}

      /* if no type icon get category icon, gets default if category not set */
      if (!icon) icon = swamigui_icon_get_category_icon (category);
    }

  gtk_tree_store_set (GTK_TREE_STORE (store), iter,
		      SWAMIGUI_TREE_STORE_LABEL_COLUMN, label,
		      SWAMIGUI_TREE_STORE_ICON_COLUMN, icon,
		      SWAMIGUI_TREE_STORE_OBJECT_COLUMN, item,
		      -1);
  if (item)
    {
      copy = gtk_tree_iter_copy (iter);
      g_object_ref (item);		/* ++ ref item for item_hash */
      g_hash_table_insert (store->item_hash, item, copy);
    }

  if (item_label) g_free (item_label);
}

/**
 * swamigui_tree_store_change:
 * @store: Swami tree store
 * @item: Object in @store to update
 * @label: New label or %NULL to keep old
 * @icon: GdkPixbuf icon or %NULL to keep old
 *
 * Changes a row in a Swami tree store.
 */
void
swamigui_tree_store_change (SwamiguiTreeStore *store, GObject *item,
			    const char *label, char *icon)
{
  GtkTreeIter iter;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (G_IS_OBJECT (item));

  if (!swamigui_tree_store_item_get_node (store, item, &iter)) return;

  if (label)
    gtk_tree_store_set (GTK_TREE_STORE (store), &iter,
			SWAMIGUI_TREE_STORE_LABEL_COLUMN, label, -1);
  if (icon)
    gtk_tree_store_set (GTK_TREE_STORE (store), &iter,
			SWAMIGUI_TREE_STORE_ICON_COLUMN, icon, -1);
}

/**
 * swamigui_tree_store_remove:
 * @store: Swami tree store
 * @item: Object in @store to remove
 *
 * Removes a node (and all its children) from a Swami tree store.
 */
void
swamigui_tree_store_remove (SwamiguiTreeStore *store, GObject *item)
{
  GtkTreeIter iter;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (G_IS_OBJECT (item));

  if (swamigui_tree_store_item_get_node (store, item, &iter))
    {
      if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (store), &iter))
	tree_store_recursive_remove (store, &iter);
      else
	{
	  gtk_tree_store_remove (GTK_TREE_STORE (store), &iter);
	  g_hash_table_remove (store->item_hash, item);
	}
    }
}

/* recursively remove tree nodes from a Swami tree store and remove the
   item->node links in item_hash table */
static void
tree_store_recursive_remove (SwamiguiTreeStore *store, GtkTreeIter *iter)
{
  GtkTreeIter children, remove;
  gboolean retval;
  register GObject *item; /* register to conserve recursive stack */

  if (gtk_tree_model_iter_children ((GtkTreeModel *)store, &children, iter))
    {
      do {
	remove = children;
	retval = gtk_tree_model_iter_next ((GtkTreeModel *)store, &children);
	tree_store_recursive_remove (store, &remove);
      } while (retval);
    }

  item = swamigui_tree_store_node_get_item (store, iter);
  gtk_tree_store_remove ((GtkTreeStore *)store, iter);

  g_hash_table_remove (store->item_hash, item);
}

/**
 * swamigui_tree_store_move_before:
 * @store: Swami tree store
 * @item: Item to move
 * @position: Location to move item before or %NULL for last position
 * 
 * Move an item from its current location to before @position.  @position
 * must be at the same level as @item in the tree.
 */
void
swamigui_tree_store_move_before (SwamiguiTreeStore *store, GObject *item,
				 GtkTreeIter *position)
{
  GtkTreeIter iter;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (G_IS_OBJECT (item));

  if (swamigui_tree_store_item_get_node (store, item, &iter))
    gtk_tree_store_move_before (GTK_TREE_STORE (store), &iter, position);
}

/**
 * swamigui_tree_store_move_after:
 * @store: Swami tree store
 * @item: Item to move
 * @position: Location to move item after or %NULL for first position
 * 
 * Move an item from its current location to after @position.  @position
 * must be at the same level as @item in the tree.
 */
void
swamigui_tree_store_move_after (SwamiguiTreeStore *store, GObject *item,
				GtkTreeIter *position)
{
  GtkTreeIter iter;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (G_IS_OBJECT (item));

  if (swamigui_tree_store_item_get_node (store, item, &iter))
    gtk_tree_store_move_after (GTK_TREE_STORE (store), &iter, position);
}

/**
 * swamigui_tree_store_item_get_node:
 * @store: Swami tree store
 * @item: Item that is in @store
 * @iter: Pointer to a GtkTreeIter structure to store the linked tree node
 *
 * Gets a tree node for @item in @store. The tree node is stored in @iter which
 * should be a pointer to a user supplied GtkTreeIter structure.
 *
 * Returns: %TRUE if @iter was set (@item has a linked node in @store), %FALSE
 *   otherwise
 */
gboolean
swamigui_tree_store_item_get_node (SwamiguiTreeStore *store, GObject *item,
				   GtkTreeIter *iter)
{
  GtkTreeIter *lookup_iter;

  g_return_val_if_fail (SWAMIGUI_IS_TREE_STORE (store), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (item), FALSE);

  lookup_iter = g_hash_table_lookup (store->item_hash, item);
  if (!lookup_iter) return (FALSE);

  if (iter) *iter = *lookup_iter;
  return (TRUE);
}

/**
 * swamigui_tree_store_node_get_item:
 * @store: Swami tree store
 * @iter: Node in @store to find linked item of
 *
 * Gets the linked item object for @iter node in @store. The returned item
 * is not referenced but can be safely used as long as the tree model isn't
 * changed (possibly causing item to be destroyed). The item should be ref'd
 * if used for extended periods.
 *
 * Returns: The item object linked with @iter node or %NULL if not found.
 *   Item has NOT been referenced, see note above.
 */
GObject *
swamigui_tree_store_node_get_item (SwamiguiTreeStore *store, GtkTreeIter *iter)
{
  GObject *item;

  g_return_val_if_fail (SWAMIGUI_IS_TREE_STORE (store), NULL);
  g_return_val_if_fail (iter != NULL, NULL);

  gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
		      SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &item,
		      -1);
  return (item);
}

/**
 * swamigui_tree_store_add:
 * @store: Swami tree store
 * @item: Item to add
 *
 * Add an item to a tree store using the item_add class method (specific to
 * tree store types).
 */
void
swamigui_tree_store_add (SwamiguiTreeStore *store, GObject *item)
{
  SwamiguiTreeStoreClass *klass;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (G_IS_OBJECT (item));

  klass = SWAMIGUI_TREE_STORE_GET_CLASS (store);
  g_return_if_fail (klass->item_add != NULL);

  klass->item_add (store, item);
}

/**
 * swamigui_tree_store_changed:
 * @store: Swami tree store
 * @item: Item that changed
 *
 * This function updates the title and sorting of an item that changed using
 * the item_changed class method (specific to tree store types).
 * Note that re-ordering of the changed item may occur in
 * a delayed fashion, to prevent delays while user is typing a name for example.
 */
void
swamigui_tree_store_changed (SwamiguiTreeStore *store, GObject *item)
{
  SwamiguiTreeStoreClass *klass;

  g_return_if_fail (SWAMIGUI_IS_TREE_STORE (store));
  g_return_if_fail (G_IS_OBJECT (item));

  klass = SWAMIGUI_TREE_STORE_GET_CLASS (store);
  g_return_if_fail (klass->item_changed != NULL);

  klass->item_changed (store, item);
}
