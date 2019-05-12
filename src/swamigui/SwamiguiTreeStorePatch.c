/*
 * SwamiguiTreeStorePatch.c - Patch tree store (for instruments).
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libinstpatch/libinstpatch.h>

#include "SwamiguiTreeStorePatch.h"
#include "SwamiguiRoot.h"
#include <libswami/SwamiLog.h>


/* used by swamigui_tree_store_real_item_add to qsort child items by title */
typedef struct
{
  GObject *tree_parent;	/* tree parent object (primary sort field) */
  char *title;	/* text title (secondary sort field, for sorted types) */
  GObject *item;
} ChildSortBag;

/* Local Prototypes */

static void swamigui_tree_store_patch_class_init
  (SwamiguiTreeStorePatchClass *klass);
static IpatchItem *swamigui_tree_store_create_virtual_child
  (IpatchItem *container, GType virtual_child_type);
static IpatchItem *swamigui_tree_store_lookup_virtual_child
  (IpatchItem *container, GType virtual_child_type);
static void
swamigui_tree_store_patch_real_item_add (
  SwamiguiTreeStore *store, GObject *item,
  GtkTreeIter *sibling, GtkTreeIter *out_iter,
  GtkTreeIter *in_parent_iter,
  ChildSortBag *inbag);
static int title_sort_compar_func (const void *a, const void *b);
static gboolean
get_item_sort_info (SwamiguiTreeStore *store, GObject *item, GType item_type,
		    GObject *parent, GObject **out_tree_parent,
		    GtkTreeIter *out_parent_iter);
static gboolean
find_sibling_title_sort (SwamiguiTreeStore *store, GObject *item, char *title,
			 GtkTreeIter *parent_iter, GtkTreeIter *sibling_iter);
static gboolean
find_sibling_container_sort (SwamiguiTreeStore *store, GObject *item,
			     GObject *parent, GtkTreeIter *parent_iter,
			     GtkTreeIter *sibling_iter);

GType
swamigui_tree_store_patch_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info =
      {
	sizeof (SwamiguiTreeStorePatchClass), NULL, NULL,
	(GClassInitFunc) swamigui_tree_store_patch_class_init, NULL, NULL,
	sizeof (SwamiguiTreeStorePatch), 0,
	(GInstanceInitFunc) NULL,
      };

    obj_type = g_type_register_static (SWAMIGUI_TYPE_TREE_STORE,
				       "SwamiguiTreeStorePatch",
				       &obj_info, 0);
  }

  return (obj_type);
}

static void
swamigui_tree_store_patch_class_init (SwamiguiTreeStorePatchClass *klass)
{
  SwamiguiTreeStoreClass *store_class = SWAMIGUI_TREE_STORE_CLASS (klass);

  store_class->item_add = swamigui_tree_store_patch_item_add;
  store_class->item_changed = swamigui_tree_store_patch_item_changed;
}

/**
 * swamigui_tree_store_patch_new:
 *
 * Create a new patch tree store for instruments.
 *
 * Returns: New patch tree store object with a ref count of 1.
 */
SwamiguiTreeStorePatch *
swamigui_tree_store_patch_new (void)
{
  return (SWAMIGUI_TREE_STORE_PATCH
	  (g_object_new (SWAMIGUI_TYPE_TREE_STORE_PATCH, NULL)));
}

/*
 * swamigui_tree_store_create_virtual_child:
 * @container: Item to create virtual container child instance in
 * @virtual_child_type: The virtual child type
 *
 * Create a virtual container instance in a container. If @container already
 * has the specified @virtual_child_type it is simply returned.
 *
 * Returns: Virtual container instance
 */
static IpatchItem *
swamigui_tree_store_create_virtual_child (IpatchItem *container,
					  GType virtual_child_type)
{
  IpatchItem *virt_container;
  gpointer data;
  const char *keyname;

  g_return_val_if_fail (IPATCH_IS_ITEM (container), NULL);
  g_return_val_if_fail (g_type_is_a (virtual_child_type, IPATCH_TYPE_VIRTUAL_CONTAINER),
			NULL);

  /* use the virtual type name as the GObject data key */
  keyname = g_type_name (virtual_child_type);
  data = g_object_get_data (G_OBJECT (container), keyname);

  if (data) return (IPATCH_ITEM (data));

  virt_container = g_object_new (virtual_child_type, NULL);	/* ++ ref */
  ipatch_item_set_parent (IPATCH_ITEM (virt_container),IPATCH_ITEM (container));

  g_object_set_data (G_OBJECT (container), keyname, virt_container);

  /* !! Reference is held until @container is removed from tree */

  return (virt_container);
}

/*
 * swamigui_tree_store_lookup_virtual_child:
 * @container: Container item to look up a virtual child instance in
 * @virtual_child_type: The virtual child type
 *
 * Lookup a virtual container child instance in a container.
 *
 * Returns: Virtual container instance or %NULL if no virtual child of
 *   requested type.
 */
static IpatchItem *
swamigui_tree_store_lookup_virtual_child (IpatchItem *container,
					  GType virtual_child_type)
{
  gpointer data;
  const char *keyname;

  g_return_val_if_fail (IPATCH_IS_ITEM (container), NULL);
  g_return_val_if_fail (g_type_is_a (virtual_child_type, IPATCH_TYPE_VIRTUAL_CONTAINER),
			NULL);

  /* use the virtual type name as the GObject data key */
  keyname = g_type_name (virtual_child_type);
  data = g_object_get_data (G_OBJECT (container), keyname);

  if (data) return (IPATCH_ITEM (data));
  else return (NULL);
}

/**
 * swamigui_tree_store_patch_item_add:
 * @store: Patch tree store
 * @item: Item to add
 *
 * Function used as item_add method of #SwamiguiTreeStorePatchClass.
 * Might be useful to other tree store types.
 */
void
swamigui_tree_store_patch_item_add (SwamiguiTreeStore *store, GObject *item)
{
  swamigui_tree_store_patch_real_item_add (store, item, NULL, NULL, NULL, NULL);
}

/* some tricks are done to speed up adding a container item (children are
   pre-sorted to decrease exponential list iterations). Yeah looks pretty
   ugly, but in theory it should provide a bit of a speedup. */
static void
swamigui_tree_store_patch_real_item_add (
  SwamiguiTreeStore *store, GObject *item,
  GtkTreeIter *sibling, GtkTreeIter *out_iter,
  GtkTreeIter *in_parent_iter,
  ChildSortBag *inbag)
{
  GtkTreeIter *parent_iter = NULL, real_parent_iter;
  GtkTreeIter sibling_iter, item_iter, *pitem_iter, tmp_iter;
  GObject *parent, *childitem;
  GObject *tree_parent;
  IpatchList *list;
  ChildSortBag bag, *bagp;
  char *alloc_title = NULL, *title;
  gboolean sort = FALSE;
  GArray *title_sort_array = NULL;
  GHashTable *prev_child_hash = NULL;
  GList *p;
  guint i;

  /* ++ ref parent */
  parent = (GObject *)ipatch_item_get_parent (IPATCH_ITEM (item));
  g_return_if_fail (parent != NULL);

  if (inbag)	/* recursive call? - Use already fetched values. */
    {
      title = inbag->title;
      tree_parent = inbag->tree_parent;
      parent_iter = in_parent_iter;
    }
  else		/* not a recursive call, no pre-cached values */
    {
      gboolean sibling_set;

      /* get title of item (if not supplied to function) */
      g_object_get (item, "title", &title, NULL);
      if (swami_log_if_fail (title != NULL)) goto ret;

      alloc_title = title;

      /* get sort boolean, tree parent object and node */
      sort = get_item_sort_info (store, item, 0, parent,
				 &tree_parent, &real_parent_iter);
      if (tree_parent) parent_iter = &real_parent_iter;

      if (sort)
	sibling_set = find_sibling_title_sort (store, item, title,
					       parent_iter, &sibling_iter);
      else sibling_set = find_sibling_container_sort (store, item, parent,
						    parent_iter, &sibling_iter);
      if (sibling_set) sibling = &sibling_iter;
    }	/* else - !inbag */

  /* insert the node into the tree */
  swamigui_tree_store_insert_after (store, item, title, NULL,
				    parent_iter, sibling, &item_iter);

  if (out_iter) *out_iter = item_iter;

  /* is added item a container? */
  if (IPATCH_IS_CONTAINER (item))
    {
      const GType *types;

      types = ipatch_container_get_virtual_types (IPATCH_CONTAINER (item));
      if (types)	/* any virtual container child types? */
	{
	  IpatchItem *virt;

	  for (; *types; types++)	/* create virtual containers */
	    {
	      virt = swamigui_tree_store_create_virtual_child
		  (IPATCH_ITEM (item), *types);

	      swamigui_tree_store_insert_before (store, G_OBJECT (virt),
				  NULL, NULL, &item_iter, NULL, NULL);
	    }
	}

      types = ipatch_container_get_child_types (IPATCH_CONTAINER (item));
      if (!types) goto ret;	/* should always be child types, but.. */

      title_sort_array = g_array_new (FALSE, FALSE, sizeof (ChildSortBag));
      prev_child_hash = g_hash_table_new_full (NULL, NULL,
					       NULL, (GDestroyNotify)g_free);

      for (; *types; types++)	/* loop over container child types */
	{
	  gboolean static_parent;

	  list = ipatch_container_get_children (IPATCH_CONTAINER (item),
						*types);  /* ++ ref list */
	  parent_iter = NULL;

	  /* virtual parent type is static? */
	  if (!ipatch_type_get_dynamic_func (*types, "virtual-parent-type"))
	    {
	      sort = get_item_sort_info (store, NULL, *types, item,
					 &tree_parent, &real_parent_iter);
	      if (tree_parent) parent_iter = &real_parent_iter;
	      static_parent = TRUE;
	    }
	  else static_parent = FALSE;

	  sibling = NULL;

	  /* loop over child items of the current type */
	  for (p = list->items; p; p = p->next)
	    {
	      childitem = (GObject *)(p->data);
	      /* dynamic virtual parent type? */
	      if (!static_parent)
		{
		  sort = get_item_sort_info (store, childitem, 0,
					     item, &tree_parent,
					     &real_parent_iter);
		  if (tree_parent) parent_iter = &real_parent_iter;
		}

	      /* store item's values in the bag structure, may or may not get
		 added to the title sort array, depending on if child is
                 title sorted */
	      bag.tree_parent = tree_parent;

	      g_object_get (childitem, "title", &title, NULL);
	      if (swami_log_if_fail (title != NULL))
		{
		  g_object_unref (list);	/* -- unref list */
		  goto ret;
		}

	      bag.title = title;
	      bag.item = childitem;

	      if (sort)	/* item should be sorted by title? */
		{
		  g_array_append_val (title_sort_array, bag);
		}
	      else		/* not title sorted, just add in order */
		{ /* keep track of the last child for each parent */
		  pitem_iter = g_hash_table_lookup (prev_child_hash, tree_parent);

		  sibling = pitem_iter;	/* sibling set if previous item */

		  if (!pitem_iter)
		    {
		      pitem_iter = g_new (GtkTreeIter, 1);
		      g_hash_table_insert (prev_child_hash, tree_parent,
					   pitem_iter);
		    }

		  swamigui_tree_store_patch_real_item_add
		    (store, G_OBJECT (p->data), sibling, &tmp_iter,
		     parent_iter, &bag);

		  *pitem_iter = tmp_iter;
		  g_free (bag.title);
		}
	    }	/* for .. p in list */

	  g_object_unref (list);	/* -- unref list */

	  if (title_sort_array->len > 0)
	    {
	      GObject *prev_parent = NULL;

	      /* sort the items by parent object and title */
	      qsort (title_sort_array->data, title_sort_array->len,
		     sizeof (ChildSortBag), title_sort_compar_func);

	      /* loop over sorted items */
	      for (i = 0; i < title_sort_array->len; i++)
		{
		  bagp = &g_array_index (title_sort_array, ChildSortBag, i);

		  /* dynamic virtual parent type? */
		  if (!static_parent)
		    {
		      sort = get_item_sort_info (store, G_OBJECT (bagp->item), 0,
						 item, &tree_parent,
						 &real_parent_iter);
		      if (tree_parent) parent_iter = &real_parent_iter;
		    }

		  /* previous and current item are in different containers? */
		  if (prev_parent != tree_parent) sibling = NULL;
		  else sibling = &item_iter;

		  swamigui_tree_store_patch_real_item_add
		    (store, G_OBJECT (bagp->item), sibling, &tmp_iter,
		     parent_iter, bagp);

		  item_iter = tmp_iter;
		  prev_parent = tree_parent;
		  g_free (bagp->title);
		}

	      g_array_set_size (title_sort_array, 0);	/* clear array */
	    }  /* if (title_sort_array->len > 0) */
	}  /* for (*types) - container child types */
    }  /* if (IPATCH_IS_CONTAINER (item)) */

 ret:

  g_object_unref (parent);	/* -- unref parent */
  if (alloc_title) g_free (alloc_title);
  if (title_sort_array) g_array_free (title_sort_array, TRUE);
  if (prev_child_hash) g_hash_table_destroy (prev_child_hash);
}

static int
title_sort_compar_func (const void *a, const void *b)
{
  ChildSortBag *baga = (ChildSortBag *)a, *bagb = (ChildSortBag *)b;

  if (baga->tree_parent == bagb->tree_parent)
    return (strcmp (baga->title, bagb->title));

  /* parent object is the primary sort */
  if (baga->tree_parent < bagb->tree_parent)
    return (1);
  else return (-1);
}

/* a helper function to get sort-children property that applies to an @item
   (can be NULL if item_type is set instead) and can also get the tree parent
   object and iterator if pointers are supplied.
   Returns: TRUE if item should be title sorted, FALSE otherwise
*/
static gboolean
get_item_sort_info (SwamiguiTreeStore *store, GObject *item, GType item_type,
		    GObject *parent, GObject **out_tree_parent,
		    GtkTreeIter *out_parent_iter)
{
  GObject *tree_parent;
  gboolean sort, has_parent_iter;
  GType type;

  if (item) ipatch_type_object_get (item, "virtual-parent-type", &type, NULL);
  else ipatch_type_get (item_type, "virtual-parent-type", &type, NULL);

  if (out_tree_parent || out_parent_iter)
    { /* item has a virtual parent type? Get the instance of it. */
      if (type != G_TYPE_NONE)
	tree_parent = (GObject *)swamigui_tree_store_lookup_virtual_child
			(IPATCH_ITEM (parent), type);
      else if (SWAMI_IS_CONTAINER (parent)) /* NULL parent to append to root */
	tree_parent = NULL;
      else tree_parent = parent;  /* otherwise use real parent */

      if (tree_parent)
	{
	  /* get the tree node for the tree parent item */
	  has_parent_iter
	    = swamigui_tree_store_item_get_node (store, G_OBJECT (tree_parent),
						 out_parent_iter);
	  if (swami_log_if_fail (has_parent_iter))
	      return (FALSE);
	}

      *out_tree_parent = tree_parent;
    }

  /* use real parent type if no virtual type */
  if (type == G_TYPE_NONE && item) type = G_OBJECT_TYPE (item);

  /* children should be sorted? */
  if (type != G_TYPE_NONE) ipatch_type_get (type, "sort-children", &sort, NULL);
  else sort = FALSE;

  return (sort);
}

/* find the closest sibling node already in tree store to insert after, sorted
 *   by title.
 * Returns: TRUE if sibling_iter is set, FALSE if item should be first.
 */
static gboolean
find_sibling_title_sort (SwamiguiTreeStore *store, GObject *item, char *title,
			 GtkTreeIter *parent_iter, GtkTreeIter *sibling_iter)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  gboolean haveprev = FALSE;
  GtkTreeIter child;
  GObject *cmpitem;
  char *curtitle;

  /* parent has any children? */
  if (!gtk_tree_model_iter_children (model, &child, parent_iter))
    return (FALSE);

  do	/* search for sibling to insert before */
    {
      gtk_tree_model_get (model, &child,
                          SWAMIGUI_TREE_STORE_LABEL_COLUMN, &curtitle,  /* ++ alloc */
                          SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &cmpitem,  /* ++ ref */
                          -1);

      g_object_unref (cmpitem); /* -- unref, we only need the pointer to compare */

      /* compare titles and make sure its not the same item (if called to re-sort item) */
      if (strcmp (title, curtitle) <= 0 && item != cmpitem)
	{
	  g_free (curtitle);
	  break;
	}

      g_free (curtitle);
      *sibling_iter = child;
      haveprev = TRUE;
    }
  while (gtk_tree_model_iter_next (model, &child));

  return (haveprev == TRUE);
}

/* find the closest sibling node already in tree store to insert after,
 *   sorted as found in container child list.
 * Returns: TRUE if sibling_iter is set, FALSE if item should be first.
 */
static gboolean
find_sibling_container_sort (SwamiguiTreeStore *store, GObject *item,
			     GObject *parent, GtkTreeIter *parent_iter,
			     GtkTreeIter *sibling_iter)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GtkTreeIter tmpiter;
  GtkTreePath *parent_path = NULL, *sibparent_path;
  IpatchList *list;
  gboolean parent_match;
  const GType *childtypes;
  GType type;
  GList *p;

  /* Find matching child type which item is a decendent of - or use item's type */
  childtypes = ipatch_container_get_child_types (IPATCH_CONTAINER (parent));

  type = G_OBJECT_TYPE (item);

  for (; *childtypes; childtypes++)
  {
    if (g_type_is_a (type, *childtypes))
    {
      type = *childtypes;
      break;
    }
  }

  /* ++ ref list */
  list = ipatch_container_get_children (IPATCH_CONTAINER (parent), type);

  p = g_list_find (list->items, item);		/* find item in list */
  if (p)
    {	/* search for previous sibling already in tree */
      for (p = p->prev; p; p = p->prev)
	{ /* item in tree? */
	  if (swamigui_tree_store_item_get_node (store, p->data,
						 sibling_iter))
	    { /* Make sure its the same parent (no parent if child of root) */
	      if (gtk_tree_model_iter_parent (model, &tmpiter, sibling_iter))
		{
		  if (!parent_path)	/* get path of parent iterator in tree store */
		    parent_path = gtk_tree_model_get_path (model, parent_iter);

		  sibparent_path = gtk_tree_model_get_path (model, &tmpiter);

		  /* is item really a sibling (same parent node?) */
		  parent_match = gtk_tree_path_compare (parent_path,
							sibparent_path) == 0;
		  gtk_tree_path_free (sibparent_path);

		  if (parent_match) break;
		}
	      else break;	/* no parent in tree (child of root) */
	    }
	}
    }

  if (parent_path) gtk_tree_path_free (parent_path);

  g_object_unref (list);	/* -- unref list */

  return (p != NULL);
}

/**
 * swamigui_tree_store_patch_item_changed:
 * @store: Patch tree store
 * @item: Item that changed
 *
 * Function used as item_changed method of #SwamiguiTreeStorePatchClass.
 * Might be useful to other tree store types.
 */
void
swamigui_tree_store_patch_item_changed (SwamiguiTreeStore *store, GObject *item)
{
  GtkTreeIter item_iter, parent_iter, curparent_iter, sibling_iter;
  gboolean found_item_node;
  gboolean found_parent_node;
  GObject *parent = NULL, *curparent = NULL, *tree_parent;
  char *title, *curtitle = NULL;

  /* get title of item */
  g_object_get (item, "title", &title, NULL);   /* ++ alloc */
  if (!title) title = g_strdup ("");

  found_item_node = swamigui_tree_store_item_get_node (store, item, &item_iter);
  if (swami_log_if_fail (found_item_node)) goto ret;

  gtk_tree_model_get (GTK_TREE_MODEL (store), &item_iter,
                      SWAMIGUI_TREE_STORE_LABEL_COLUMN, &curtitle,  /* ++ alloc */
                      -1);
  if (strcmp (title, curtitle) == 0) goto ret;  /* No change in title? - Return */

  swamigui_tree_store_change (store, item, title, NULL);

  parent = (GObject *)ipatch_item_get_parent (IPATCH_ITEM (item));      /* ++ ref */
  if (swami_log_if_fail (parent != NULL)) goto ret;

  /* Is item under a sorted parent? */
  if (get_item_sort_info (store, item, 0, parent, &tree_parent, &parent_iter))
  {
    /* Check if parent node has changed (switched from melodic to percussion for example) */
    found_parent_node = gtk_tree_model_iter_parent (GTK_TREE_MODEL (store),
                                                    &curparent_iter, &item_iter);
    if (swami_log_if_fail (found_parent_node)) goto ret;

    gtk_tree_model_get (GTK_TREE_MODEL (store), &curparent_iter,
                        SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &curparent,  /* ++ ref */
                        -1);

    /* If the parent is the same, we can use the tree move functions */
    if (curparent == tree_parent)
    {
      if (find_sibling_title_sort (store, item, title, &parent_iter, &sibling_iter))
        swamigui_tree_store_move_after (store, item, &sibling_iter);
      else swamigui_tree_store_move_after (store, item, NULL);
    }
    else        /* Parent changed, remove and add it back (yeah, PITA!) */
    {
      swamigui_tree_store_remove (store, item);
      swamigui_tree_store_patch_item_add (store, item);
    }
  }

ret:
  g_free (title);       /* -- free title */
  g_free (curtitle);    /* -- free curtitle */
  if (parent) g_object_unref (parent);  /* -- unref parent */
  if (curparent) g_object_unref (curparent);    /* -- unref curparent */
}
