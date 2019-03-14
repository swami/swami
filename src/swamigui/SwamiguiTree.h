/*
 * SwamiguiTree.h - Swami tabbed tree object header file
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
#ifndef __SWAMIGUI_TREE_H__
#define __SWAMIGUI_TREE_H__

#include <gtk/gtk.h>
#include <swamigui/SwamiguiTreeStore.h>

typedef struct _SwamiguiTree SwamiguiTree;
typedef struct _SwamiguiTreeClass SwamiguiTreeClass;

#define SWAMIGUI_TYPE_TREE   (swamigui_tree_get_type ())
#define SWAMIGUI_TREE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_TREE, SwamiguiTree))
#define SWAMIGUI_TREE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_TREE, SwamiguiTreeClass))
#define SWAMIGUI_IS_TREE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_TREE))
#define SWAMIGUI_IS_TREE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_TREE))

/* Swami Tree Object (all fields private) */
struct _SwamiguiTree
{
  GtkVBox parent_instance;

  /*< private >*/

  GtkNotebook *notebook;	/* notebook widget */
  GtkWidget *search_box;	/* the box containing the search widgets */
  GtkEntry *search_entry;	/* the search entry */

  IpatchList *stores;	/* list of SwamiguiTreeStore objects for each tab */
  GList *treeviews;	/* list of GtkTreeView widgets for each tab */

  SwamiguiTreeStore *selstore;	/* currently selected tree store (not ref'd) */
  GtkTreeView *seltree;		/* currently selected tree (not ref'd) */

  IpatchList *selection;	/* current selection of GObjects */
  gboolean sel_single;		/* TRUE if single item selected */

  char *search_text;		/* current search text */
  GObject *search_start;	/* search start item (item in tree, not ref'd) */
  GObject *search_match;	/* current matching search item (not ref'd) */
  int search_start_pos;		/* start char pos in search_match label */
  int search_end_pos;		/* start char pos in search_match label */
  GList *search_expanded;	/* branches which should be collapsed (items) */
};

/* Swami Tree Object class (all fields private) */
struct _SwamiguiTreeClass
{
  GtkVBoxClass parent_class;
};

GType swamigui_tree_get_type (void);
GtkWidget *swamigui_tree_new (IpatchList *stores);

void swamigui_tree_set_store_list (SwamiguiTree *tree, IpatchList *list);
IpatchList *swamigui_tree_get_store_list (SwamiguiTree *tree);
void swamigui_tree_set_selected_store (SwamiguiTree *tree,
				       SwamiguiTreeStore *store);
SwamiguiTreeStore *swamigui_tree_get_selected_store (SwamiguiTree *tree);

GObject *swamigui_tree_get_selection_single (SwamiguiTree *tree);
IpatchList *swamigui_tree_get_selection (SwamiguiTree *tree);
void swamigui_tree_clear_selection (SwamiguiTree *tree);
void swamigui_tree_set_selection (SwamiguiTree *tree, IpatchList *list);
void swamigui_tree_spotlight_item (SwamiguiTree *tree, GObject *item);

void swamigui_tree_search_set_start (SwamiguiTree *tree, GObject *start);
void swamigui_tree_search_set_text (SwamiguiTree *tree, const char *text);
void swamigui_tree_search_set_visible (SwamiguiTree *tree, gboolean visible);
void swamigui_tree_search_next (SwamiguiTree *tree);
void swamigui_tree_search_prev (SwamiguiTree *tree);

#endif
