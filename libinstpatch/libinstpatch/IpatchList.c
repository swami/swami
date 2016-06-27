/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
/**
 * SECTION: IpatchList
 * @short_description: An object containing a list of object pointers
 * @see_also: 
 * @stability: Stable
 *
 * An object which defines a list of object pointers.  A #GObject reference
 * is held to all objects until the #IpatchList itself is released.
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchList.h"

static void ipatch_list_class_init (IpatchListClass *klass);
static void ipatch_list_finalize (GObject *gobject);

static GObjectClass *parent_class = NULL;


GType
ipatch_list_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchListClass), NULL, NULL,
      (GClassInitFunc) ipatch_list_class_init, NULL, NULL,
      sizeof (IpatchList), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (G_TYPE_OBJECT, "IpatchList",
					&item_info, 0);
  }

  return (item_type);
}

static void
ipatch_list_class_init (IpatchListClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);
  obj_class->finalize = ipatch_list_finalize;
}

static void
ipatch_list_finalize (GObject *gobject)
{
  IpatchList *list = IPATCH_LIST (gobject);
  GList *p;

  p = list->items;
  while (p)	  /* unref all objects in list and destroy the list */
    {
      g_object_unref (p->data);
      p = g_list_delete_link (p, p);
    }
  list->items = NULL;

  if (parent_class->finalize)
    parent_class->finalize (gobject);
}

/**
 * ipatch_list_new:
 *
 * Create a new object list object. #IpatchList objects are often used to
 * duplicate multi-thread sensitive object lists, so they can be iterated over
 * at one's own leasure.
 *
 * Returns: New object list container object.
 */
IpatchList *
ipatch_list_new (void)
{
  return (IPATCH_LIST (g_object_new (IPATCH_TYPE_LIST, NULL)));
}

/**
 * ipatch_list_duplicate:
 * @list: Object list to duplicate
 *
 * Duplicate an object list.
 *
 * Returns: (transfer full): New duplicate object list with a ref count of one which the
 *   caller owns.
 */
IpatchList *
ipatch_list_duplicate (IpatchList *list)
{
  IpatchList *newlist;
  GList *p;

  g_return_val_if_fail (IPATCH_IS_LIST (list), NULL);

  newlist = ipatch_list_new ();	/* ++ ref new list */
  p = list->items;
  while (p)
    {
      if (p->data) g_object_ref (p->data); /* ++ ref object for new list */
      newlist->items = g_list_prepend (newlist->items, p->data);
      p = g_list_next (p);
    }
  newlist->items = g_list_reverse (newlist->items);

  return (newlist);	   /* !! caller owns the new list reference */
}

/**
 * ipatch_list_get_items:
 * @list: List object
 *
 * Get the items list in a #IpatchList object. Mainly for the benefit of
 * GObject Introspection, since it is common practice to just access the
 * <structfield>items</structfield> field directly.
 *
 * Returns: (element-type GObject) (transfer none): The list of items in
 *   the @list object, which is owned by the @list and should not be
 *   modified or freed directly.
 *
 * Since: 1.1.0
 */
GList *
ipatch_list_get_items (IpatchList *list)
{
  g_return_val_if_fail (IPATCH_IS_LIST (list), NULL);

  return list->items;
}

/**
 * ipatch_list_set_items:
 * @list: List object
 * @items: (element-type GObject) (transfer full): List of #GObject pointers to assign,
 *   @list takes over ownership, each object should be referenced already for the list
 *
 * Set the items list in a #IpatchList object. Mainly for the benefit of
 * GObject Introspection, since it is common practice to just access the
 * <structfield>items</structfield> field directly.  Replaces existing items
 * (list if any).
 *
 * Since: 1.1.0
 */
void
ipatch_list_set_items (IpatchList *list, GList *items)
{
  GList *p;

  g_return_if_fail (IPATCH_IS_LIST (list));

  for (p = list->items; p; p = g_list_delete_link (p, p))
    g_object_unref (p->data);

  list->items = items;  // !! list takes over items
}

/**
 * ipatch_list_append:
 * @list: List object
 * @object: Object to append to the list
 *
 * Append an object to an #IpatchList.
 *
 * Since: 1.1.0
 */
void
ipatch_list_append (IpatchList *list, GObject *object)
{
  g_return_if_fail (IPATCH_IS_LIST (list));
  g_return_if_fail (G_IS_OBJECT (object));

  g_object_ref (object);        // ++ ref for list
  list->items = g_list_append (list->items, object);
}

/**
 * ipatch_list_prepend:
 * @list: List object
 * @object: Object to prepend to the list
 *
 * Prepend an object to an #IpatchList.
 *
 * Since: 1.1.0
 */
void
ipatch_list_prepend (IpatchList *list, GObject *object)
{
  g_return_if_fail (IPATCH_IS_LIST (list));
  g_return_if_fail (G_IS_OBJECT (object));

  g_object_ref (object);        // ++ ref for list
  list->items = g_list_prepend (list->items, object);
}

/**
 * ipatch_list_insert:
 * @list: List object
 * @object: Object to insert into the list
 * @pos: Position to insert into (0 for start of list, -1 to append)
 *
 * Append an object to an #IpatchList.
 *
 * Since: 1.1.0
 */
void
ipatch_list_insert (IpatchList *list, GObject *object, int pos)
{
  g_return_if_fail (IPATCH_IS_LIST (list));
  g_return_if_fail (G_IS_OBJECT (object));

  g_object_ref (object);        // ++ ref for list
  list->items = g_list_insert (list->items, object, pos);
}

/**
 * ipatch_list_remove:
 * @list: List object
 * @object: Object to remove from the list
 *
 * Remove an object from an #IpatchList.
 *
 * Returns: %TRUE if found and removed, %FALSE otherwise
 *
 * Since: 1.1.0
 */
gboolean
ipatch_list_remove (IpatchList *list, GObject *object)
{
  GList *p;

  g_return_val_if_fail (IPATCH_IS_LIST (list), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);

  p = g_list_find (list->items, object);
  if (!p) return (FALSE);

  g_object_unref (p->data);     // -- unref object
  list->items = g_list_delete_link (list->items, p);

  return (TRUE);
}

/**
 * ipatch_list_init_iter: (skip)
 * @list: List object
 * @iter: Iterator to initialize
 *
 * Initializes a user supplied iterator (usually stack allocated) to iterate
 * over the object @list. Further operations on @iter will use the @list.
 */
void
ipatch_list_init_iter (IpatchList *list, IpatchIter *iter)
{
  g_return_if_fail (IPATCH_IS_LIST (list));
  g_return_if_fail (iter != NULL);

  ipatch_iter_GList_init (iter, &list->items);
}
