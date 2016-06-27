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
 * SECTION: IpatchIter
 * @short_description: Iterator instance
 * @see_also: 
 * @stability: Stable
 *
 * A boxed type (structure) used for abstracting manipulation of object lists.
 */
#include <glib.h>
#include "IpatchIter.h"

/* IpatchIter methods for GSList type lists */
IpatchIterMethods ipatch_iter_GSList_methods =
{
  ipatch_iter_GSList_get,
  ipatch_iter_GSList_next,
  ipatch_iter_GSList_first,
  ipatch_iter_GSList_last,
  ipatch_iter_GSList_index,
  ipatch_iter_GSList_insert,
  ipatch_iter_GSList_remove,
  ipatch_iter_GSList_count
};

/* IpatchIter methods for GList type lists */
IpatchIterMethods ipatch_iter_GList_methods =
{
  ipatch_iter_GList_get,
  ipatch_iter_GList_next,
  ipatch_iter_GList_first,
  ipatch_iter_GList_last,
  ipatch_iter_GList_index,
  ipatch_iter_GList_insert,
  ipatch_iter_GList_remove,
  ipatch_iter_GList_count
};

/* IpatchIter methods for arrays */
IpatchIterMethods ipatch_iter_array_methods =
{
  ipatch_iter_array_get,
  ipatch_iter_array_next,
  ipatch_iter_array_first,
  ipatch_iter_array_last,
  ipatch_iter_array_index,
  ipatch_iter_array_insert,
  ipatch_iter_array_remove,
  ipatch_iter_array_count
};

/**
 * ipatch_iter_get_type:
 *
 * Gets the GBoxed derived type for #IpatchIter structures.
 *
 * Returns: GType of #IpatchIter structures.
 */
GType
ipatch_iter_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchIter",
					 (GBoxedCopyFunc)ipatch_iter_duplicate,
					 (GBoxedFreeFunc)ipatch_iter_free);
  return (type);
}

/**
 * ipatch_iter_alloc: (skip)
 *
 * Allocates an item iterator. This function is seldom used since
 * #IpatchIter structures are usually allocated on the stack.
 *
 * Returns: Newly allocated item iterator. Should be freed with
 *   ipatch_iter_free() when finished with it.
 */
IpatchIter *
ipatch_iter_alloc (void)
{
  IpatchIter *iter;
  iter = g_new0 (IpatchIter, 1);
  return (iter);
}

/**
 * ipatch_iter_free: (skip)
 * @iter: Item iterator
 *
 * Frees an item iterator that was allocated with ipatch_iter_alloc().
 * Seldom used since #IpatchIter structures are usually allocated on the
 * stack.
 */
void
ipatch_iter_free (IpatchIter *iter)
{
  g_free (iter);
}

/**
 * ipatch_iter_duplicate: (skip)
 * @iter: Patch iterator to duplicate
 *
 * Duplicates a patch iterator. Seldom used since #IpatchIter
 * structures are usually allocated on the stack and can be copied
 * directly.
 *
 * Returns: Newly allocated patch iter identical to @iter. Free it with
 * ipatch_iter_free() when finished.
 */
IpatchIter *
ipatch_iter_duplicate (IpatchIter *iter)
{
  IpatchIter *newiter;

  newiter = ipatch_iter_alloc ();
  *newiter = *iter;
  return (newiter);
}

/**
 * ipatch_iter_GSList_init: (skip)
 * @iter: Iterator to initialize
 * @list: Pointer to root GSList pointer to initialize iterator to
 *
 * Initialize an iterator to iterate over a GSList.
 */
void
ipatch_iter_GSList_init (IpatchIter *iter, GSList **list)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (list != NULL);

  iter->methods = &ipatch_iter_GSList_methods;
  IPATCH_ITER_GSLIST_SET_LIST (iter, list);
  IPATCH_ITER_GSLIST_SET_POS (iter, NULL);
}

/**
 * ipatch_iter_GSList_get: (skip)
 * @iter: Item iterator initialized with a GSList
 *
 * GSList item iterator method to get the current item.
 *
 * Returns: Current item or %NULL if no current item.
 */
gpointer
ipatch_iter_GSList_get (IpatchIter *iter)
{
  GSList *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  pos = IPATCH_ITER_GSLIST_GET_POS (iter);
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GSList_next: (skip)
 * @iter: Item iterator initialized with a GSList
 *
 * GSList item iterator method to get the next item and advance the
 * iterator's position.
 *
 * Returns: Next item or %NULL if no more items.
 */
gpointer
ipatch_iter_GSList_next (IpatchIter *iter)
{
  GSList *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  pos = IPATCH_ITER_GSLIST_GET_POS (iter);
  if (pos) pos = g_slist_next (pos);
  IPATCH_ITER_GSLIST_SET_POS (iter, pos); /* set current position */
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GSList_first: (skip)
 * @iter: Item iterator initialized with a GSList
 *
 * GSList item iterator method to get the first item and set the
 * iterator's position to it.
 *
 * Returns: First item or %NULL if GSList is empty.
 */
gpointer
ipatch_iter_GSList_first (IpatchIter *iter)
{
  GSList **list, *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  list = IPATCH_ITER_GSLIST_GET_LIST (iter); /* list pointer */
  g_return_val_if_fail (list != NULL, NULL);

  pos = *list;
  IPATCH_ITER_GSLIST_SET_POS (iter, pos); /* set position */
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GSList_last: (skip)
 * @iter: Item iterator initialized with a GSList
 *
 * GSList item iterator method to get the last item and set the
 * iterator's position to it.
 *
 * Returns: Last item or %NULL if GSList is empty.
 */
gpointer
ipatch_iter_GSList_last (IpatchIter *iter)
{
  GSList **list, *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  list = IPATCH_ITER_GSLIST_GET_LIST (iter);
  g_return_val_if_fail (list != NULL, NULL);

  pos = g_slist_last (*list);
  IPATCH_ITER_GSLIST_SET_POS (iter, pos); /* set current position */
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GSList_index: (skip)
 * @iter: Item iterator initialized with a GSList
 * @index: Index, from 0, of item to get
 *
 * GSList item iterator method to get an item at a given index and set the
 * iterator's position to it.
 *
 * Returns: item at the @index position or %NULL if index is off
 * the end of the GSList.
 */
gpointer
ipatch_iter_GSList_index (IpatchIter *iter, int index)
{
  GSList **list, *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  list = IPATCH_ITER_GSLIST_GET_LIST (iter);
  g_return_val_if_fail (list != NULL, NULL);

  pos = g_slist_nth (*list, index);
  IPATCH_ITER_GSLIST_SET_POS (iter, pos); /* set current position */
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GSList_insert: (skip)
 * @iter: Item iterator initialized with a GSList
 * @item: Pointer to insert
 *
 * GSList item iterator method to insert an item pointer.
 */
void
ipatch_iter_GSList_insert (IpatchIter *iter, gpointer item)
{
  GSList **list, *pos;

  g_return_if_fail (iter != NULL);

  if ((pos = IPATCH_ITER_GSLIST_GET_POS (iter))) /* position set? */
    {
      pos = g_slist_insert (pos, item, 1); /* insert after position */
      IPATCH_ITER_GSLIST_SET_POS (iter, g_slist_next (pos)); /* update pos */
    }
  else				/* position not set */
    {
      list = IPATCH_ITER_GSLIST_GET_LIST (iter);
      g_return_if_fail (list != NULL);

      pos = g_slist_prepend (*list, item); /* prepend */
      IPATCH_ITER_GSLIST_SET_POS (iter, pos); /* set current position */
      *list = pos;		/* set root of list */
    }
}

/**
 * ipatch_iter_GSList_remove: (skip)
 * @iter: Item iterator initialized with a GSList
 *
 * GSList item iterator method to remove the current item and advance
 * the current position.
 */
void
ipatch_iter_GSList_remove (IpatchIter *iter)
{
  GSList **list, *pos;

  g_return_if_fail (iter != NULL);

  list = IPATCH_ITER_GSLIST_GET_LIST (iter);
  g_return_if_fail (list != NULL);

  /* advance current position if set */
  pos = IPATCH_ITER_GSLIST_GET_POS (iter);
  if (pos)
    {
      IPATCH_ITER_GSLIST_SET_POS (iter, g_slist_next (pos));
      *list = g_slist_delete_link (*list, pos);
    }
}

/**
 * ipatch_iter_GSList_count: (skip)
 * @iter: Item iterator initialized with a GSList
 *
 * GSList item iterator method to get the count of items.
 *
 * Returns: Count of items in GSList iterator.
 */
int
ipatch_iter_GSList_count (IpatchIter *iter)
{
  GSList **list;

  g_return_val_if_fail (iter != NULL, 0);

  list = IPATCH_ITER_GSLIST_GET_LIST (iter);
  g_return_val_if_fail (list != NULL, 0);

  return (g_slist_length (*list));
}


/**
 * ipatch_iter_GList_init: (skip)
 * @iter: Iterator to initialize
 * @list: Pointer to root GList pointer to initialize iterator to
 *
 * Initialize an iterator to iterate over a GList.
 */
void
ipatch_iter_GList_init (IpatchIter *iter, GList **list)
{
  g_return_if_fail (iter != NULL);

  iter->methods = &ipatch_iter_GList_methods;
  IPATCH_ITER_GLIST_SET_LIST (iter, list);
  IPATCH_ITER_GLIST_SET_POS (iter, NULL);
}

/**
 * ipatch_iter_GList_get: (skip)
 * @iter: Item iterator initialized with a GList
 *
 * GList item iterator method to get the current item.
 *
 * Returns: Current item or %NULL if no current item.
 */
gpointer
ipatch_iter_GList_get (IpatchIter *iter)
{
  GList *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  pos = IPATCH_ITER_GLIST_GET_POS (iter);
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GList_next: (skip)
 * @iter: Item iterator initialized with a GList
 *
 * GList item iterator method to get the next item and advance the
 * iterator's position.
 *
 * Returns: Next item or %NULL if no more items.
 */
gpointer
ipatch_iter_GList_next (IpatchIter *iter)
{
  GList *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  pos = IPATCH_ITER_GLIST_GET_POS (iter);
  if (pos) pos = g_list_next (pos);
  IPATCH_ITER_GLIST_SET_POS (iter, pos); /* set current position */
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GList_first: (skip)
 * @iter: Item iterator initialized with a GList
 *
 * GList item iterator method to get the first item and set the
 * iterator's position to it.
 *
 * Returns: First item or %NULL if GList is empty.
 */
gpointer
ipatch_iter_GList_first (IpatchIter *iter)
{
  GList **list, *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  list = IPATCH_ITER_GLIST_GET_LIST (iter); /* list pointer */
  g_return_val_if_fail (list != NULL, NULL);

  pos = *list;
  IPATCH_ITER_GLIST_SET_POS (iter, pos); /* set position */
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GList_last: (skip)
 * @iter: Item iterator initialized with a GList
 *
 * GList item iterator method to get the last item and set the
 * iterator's position to it.
 *
 * Returns: Last item or %NULL if GList is empty.
 */
gpointer
ipatch_iter_GList_last (IpatchIter *iter)
{
  GList **list, *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  list = IPATCH_ITER_GLIST_GET_LIST (iter);
  g_return_val_if_fail (list != NULL, NULL);

  pos = g_list_last (*list);
  IPATCH_ITER_GLIST_SET_POS (iter, pos); /* set current position */
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GList_index: (skip)
 * @iter: Item iterator initialized with a GList
 * @index: Index, from 0, of item to get
 *
 * GList item iterator method to get an item at a given index and set the
 * iterator's position to it.
 *
 * Returns: item at the @index position or %NULL if index is off
 * the end of the GList.
 */
gpointer
ipatch_iter_GList_index (IpatchIter *iter, int index)
{
  GList **list, *pos;

  g_return_val_if_fail (iter != NULL, NULL);

  list = IPATCH_ITER_GLIST_GET_LIST (iter);
  g_return_val_if_fail (list != NULL, NULL);

  pos = g_list_nth (*list, index);
  IPATCH_ITER_GLIST_SET_POS (iter, pos); /* set current position */
  return (pos ? (gpointer)(pos->data) : NULL);
}

/**
 * ipatch_iter_GList_insert: (skip)
 * @iter: Item iterator initialized with a GList
 * @item: Pointer to insert
 *
 * GList item iterator method to insert an item pointer.
 */
void
ipatch_iter_GList_insert (IpatchIter *iter, gpointer item)
{
  GList **list, *pos;

  g_return_if_fail (iter != NULL);

  if ((pos = IPATCH_ITER_GLIST_GET_POS (iter))) /* position set? */
    {
      pos = g_list_insert (pos, item, 1); /* insert after position */
      IPATCH_ITER_GLIST_SET_POS (iter, g_list_next (pos)); /* update pos */
    }
  else				/* position not set */
    {
      list = IPATCH_ITER_GLIST_GET_LIST (iter);
      g_return_if_fail (list != NULL);

      pos = g_list_prepend (*list, item); /* prepend */
      IPATCH_ITER_GLIST_SET_POS (iter, pos); /* set current position */
      *list = pos;		/* set root of list */
    }
}

/**
 * ipatch_iter_GList_remove: (skip)
 * @iter: Item iterator initialized with a GList
 *
 * GList item iterator method to remove the current item and advance
 * the current position.
 */
void
ipatch_iter_GList_remove (IpatchIter *iter)
{
  GList **list, *pos;

  g_return_if_fail (iter != NULL);

  list = IPATCH_ITER_GLIST_GET_LIST (iter);
  g_return_if_fail (list != NULL);

  /* advance current position if set */
  pos = IPATCH_ITER_GLIST_GET_POS (iter);
  if (pos)
    {
      IPATCH_ITER_GLIST_SET_POS (iter, g_list_next (pos));
      *list = g_list_delete_link (*list, pos);
    }
}

/**
 * ipatch_iter_GList_count: (skip)
 * @iter: Item iterator initialized with a GList
 *
 * GList item iterator method to get the count of items.
 *
 * Returns: Count of items in GList iterator.
 */
int
ipatch_iter_GList_count (IpatchIter *iter)
{
  GList **list;

  g_return_val_if_fail (iter != NULL, 0);

  list = IPATCH_ITER_GLIST_GET_LIST (iter);
  g_return_val_if_fail (list != NULL, 0);

  return (g_list_length (*list));
}


/**
 * ipatch_iter_array_init: (skip)
 * @iter: Iterator to initialize
 * @array: Pointer to an array of pointers
 * @size: Count of elements in @array.
 *
 * Initialize an iterator to iterate over an array (read only).
 */
void
ipatch_iter_array_init (IpatchIter *iter, gpointer *array, guint size)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (array != NULL);

  iter->methods = &ipatch_iter_array_methods;
  IPATCH_ITER_ARRAY_SET_ARRAY (iter, array);
  IPATCH_ITER_ARRAY_SET_SIZE (iter, size);
  IPATCH_ITER_ARRAY_SET_POS (iter, -1);		/* init to no position */
}

/**
 * ipatch_iter_array_get: (skip)
 * @iter: Item iterator initialized with an array
 *
 * Array item iterator method to get the current item.
 *
 * Returns: Current item or %NULL if no current item.
 */
gpointer
ipatch_iter_array_get (IpatchIter *iter)
{
  gpointer *array;
  int pos;

  g_return_val_if_fail (iter != NULL, NULL);

  array = IPATCH_ITER_ARRAY_GET_ARRAY (iter);
  g_return_val_if_fail (array != NULL, NULL);

  pos = IPATCH_ITER_ARRAY_GET_POS (iter);
  return ((pos != -1) ? array[pos] : NULL);
}

/**
 * ipatch_iter_array_next: (skip)
 * @iter: Item iterator initialized with an array
 *
 * Array item iterator method to get the next item and advance the
 * iterator's position.
 *
 * Returns: Next item or %NULL if no more items.
 */
gpointer
ipatch_iter_array_next (IpatchIter *iter)
{
  gpointer *array;
  int pos;
  guint size;

  g_return_val_if_fail (iter != NULL, NULL);

  array = IPATCH_ITER_ARRAY_GET_ARRAY (iter);
  g_return_val_if_fail (array != NULL, NULL);

  pos = IPATCH_ITER_ARRAY_GET_POS (iter);
  size = IPATCH_ITER_ARRAY_GET_SIZE (iter);

  if (pos >= 0 && (pos + 1) < size) pos++;
  else pos = -1;

  IPATCH_ITER_ARRAY_SET_POS (iter, pos); /* update position */

  return ((pos != -1) ? array[pos] : NULL);
}

/**
 * ipatch_iter_array_first: (skip)
 * @iter: Item iterator initialized with an array
 *
 * Array item iterator method to get the first item and set the
 * iterator's position to it.
 *
 * Returns: First item or %NULL if array is empty.
 */
gpointer
ipatch_iter_array_first (IpatchIter *iter)
{
  gpointer *array;
  int pos = 0;
  guint size;

  g_return_val_if_fail (iter != NULL, NULL);

  array = IPATCH_ITER_ARRAY_GET_ARRAY (iter);
  g_return_val_if_fail (array != NULL, NULL);

  size = IPATCH_ITER_ARRAY_GET_SIZE (iter);
  if (size == 0) pos = -1;

  IPATCH_ITER_ARRAY_SET_POS (iter, pos);

  return ((pos != -1) ? array[pos] : NULL);
}

/**
 * ipatch_iter_array_last: (skip)
 * @iter: Item iterator initialized with an array
 *
 * Array item iterator method to get the last item and set the
 * iterator's position to it.
 *
 * Returns: Last item or %NULL if array is empty.
 */
gpointer
ipatch_iter_array_last (IpatchIter *iter)
{
  gpointer *array;
  int pos;
  guint size;

  g_return_val_if_fail (iter != NULL, NULL);

  array = IPATCH_ITER_ARRAY_GET_ARRAY (iter);
  g_return_val_if_fail (array != NULL, NULL);

  size = IPATCH_ITER_ARRAY_GET_SIZE (iter);
  if (size > 0) pos = size - 1;
  else pos = -1;

  IPATCH_ITER_ARRAY_SET_POS (iter, pos);

  return ((pos != -1) ? array[pos] : NULL);
}

/**
 * ipatch_iter_array_index: (skip)
 * @iter: Item iterator initialized with an array
 * @index: Index, from 0, of item to get
 *
 * Array item iterator method to get an item at a given index and set the
 * iterator's position to it.
 *
 * Returns: item at the @index position or %NULL if index is off
 * the end of the array.
 */
gpointer
ipatch_iter_array_index (IpatchIter *iter, int index)
{
  gpointer *array;
  guint size;

  g_return_val_if_fail (iter != NULL, NULL);

  array = IPATCH_ITER_ARRAY_GET_ARRAY (iter);
  g_return_val_if_fail (array != NULL, NULL);

  size = IPATCH_ITER_ARRAY_GET_SIZE (iter);
  if (index < 0 || index >= size) index = -1;

  IPATCH_ITER_ARRAY_SET_POS (iter, index);

  return ((index != -1) ? array[index] : NULL);
}

/**
 * ipatch_iter_array_insert: (skip)
 * @iter: Item iterator initialized with a array
 * @item: Pointer to insert
 *
 * array item iterator method to insert an item pointer.
 */
void
ipatch_iter_array_insert (IpatchIter *iter, gpointer item)
{
  g_return_if_reached ();
}

/**
 * ipatch_iter_array_remove: (skip)
 * @iter: Item iterator initialized with a array
 *
 * array item iterator method to remove the current item and advance
 * the current position.
 */
void
ipatch_iter_array_remove (IpatchIter *iter)
{
  g_return_if_reached ();
}

/**
 * ipatch_iter_array_count: (skip)
 * @iter: Item iterator initialized with a array
 *
 * array item iterator method to get the count of items.
 *
 * Returns: Count of items in array iterator.
 */
int
ipatch_iter_array_count (IpatchIter *iter)
{
  guint size;

  g_return_val_if_fail (iter != NULL, 0);

  size = IPATCH_ITER_ARRAY_GET_SIZE (iter);
  return (size);
}
