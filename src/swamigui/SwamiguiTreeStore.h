/*
 * SwamiguiTreeStore.h - Swami item tree store object
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
#ifndef __SWAMIGUI_TREE_STORE_H__
#define __SWAMIGUI_TREE_STORE_H__

typedef struct _SwamiguiTreeStore SwamiguiTreeStore;
typedef struct _SwamiguiTreeStoreClass SwamiguiTreeStoreClass;

#include <gtk/gtk.h>

#define SWAMIGUI_TYPE_TREE_STORE   (swamigui_tree_store_get_type ())
#define SWAMIGUI_TREE_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_TREE_STORE, \
   SwamiguiTreeStore))
#define SWAMIGUI_TREE_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_TREE_STORE, \
   SwamiguiTreeStoreClass))
#define SWAMIGUI_IS_TREE_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_TREE_STORE))
#define SWAMIGUI_IS_TREE_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_TREE_STORE))
#define SWAMIGUI_TREE_STORE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SWAMIGUI_TYPE_TREE_STORE, \
   SwamiguiTreeStoreClass))

/* Swami GUI tree store object */
struct _SwamiguiTreeStore
{
  GtkTreeStore parent_instance;	/* derived from GtkTreeStore */
  GHashTable *item_hash;	/* hash of GObject -> GtkTreeIter* */
};

/* Swami GUI tree store class */
struct _SwamiguiTreeStoreClass
{
  GtkTreeStoreClass parent_class;

  void (*item_add)(SwamiguiTreeStore *store, GObject *item);
  void (*item_changed)(SwamiguiTreeStore *store, GObject *item);
};

/* GtkTreeStore columns */
enum
{
  SWAMIGUI_TREE_STORE_LABEL_COLUMN, /* label column */
  SWAMIGUI_TREE_STORE_ICON_COLUMN, /* pointer (static string) */
  SWAMIGUI_TREE_STORE_OBJECT_COLUMN, /* pointer to patch item (invisible) */
  SWAMIGUI_TREE_STORE_NUM_COLUMNS
};

/* some developer targeted error messages */
#define SWAMIGUI_TREE_ERRMSG_PARENT_NOT_IN_TREE "Parent not in tree store"
#define SWAMIGUI_TREE_ERRMSG_ITEM_NOT_IN_TREE "Item not in tree store"

GType swamigui_tree_store_get_type (void);

void swamigui_tree_store_insert (SwamiguiTreeStore *store, GObject *item,
				 const char *label, char *icon,
				 GtkTreeIter *parent, int pos,
				 GtkTreeIter *out_iter);
void swamigui_tree_store_insert_before (SwamiguiTreeStore *store,
					GObject *item,
					const char *label,
					char *icon,
					GtkTreeIter *parent,
					GtkTreeIter *sibling,
					GtkTreeIter *out_iter);
void swamigui_tree_store_insert_after (SwamiguiTreeStore *store,
				       GObject *item,
				       const char *label,
				       char *icon,
				       GtkTreeIter *parent,
				       GtkTreeIter *sibling,
				       GtkTreeIter *out_iter);
void swamigui_tree_store_change (SwamiguiTreeStore *store, GObject *item,
				 const char *label, char *icon);
void swamigui_tree_store_remove (SwamiguiTreeStore *store, GObject *item);
void swamigui_tree_store_move_before (SwamiguiTreeStore *store,
				      GObject *item,
				      GtkTreeIter *position);
void swamigui_tree_store_move_after (SwamiguiTreeStore *store,
				     GObject *item, GtkTreeIter *position);
gboolean swamigui_tree_store_item_get_node (SwamiguiTreeStore *store,
					    GObject *item,
					    GtkTreeIter *iter);
GObject *swamigui_tree_store_node_get_item (SwamiguiTreeStore *store,
					    GtkTreeIter *iter);
void swamigui_tree_store_add (SwamiguiTreeStore *store, GObject *item);
void swamigui_tree_store_changed (SwamiguiTreeStore *store, GObject *item);

#endif
