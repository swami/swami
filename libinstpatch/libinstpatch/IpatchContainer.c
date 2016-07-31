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
 * SECTION: IpatchContainer
 * @short_description: Abstract object type used for items containing other
 *   child items.
 * @see_also: 
 * @stability: Stable
 *
 * Objects which are derived from this abstract type can contain other items,
 * thus forming a tree of items in an instrument file for example.
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchContainer.h"
#include "ipatch_priv.h"

/* libInstPatch private functions defined in IpatchContainer_notify.c */
extern void _ipatch_container_notify_init (void);
extern void ipatch_container_add_notify (IpatchContainer *container,
					 IpatchItem *child);
extern void ipatch_container_remove_notify (IpatchContainer *container,
					    IpatchItem *child);

static void ipatch_container_init_class (IpatchContainerClass *klass);
static void ipatch_container_dispose (GObject *object);
static void ipatch_container_item_remove_full (IpatchItem *item, gboolean full);

static GObjectClass *parent_class = NULL;

/* for fast hook method access */
static IpatchContainerClass *real_container_class = NULL;



GType
ipatch_container_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchContainerClass), NULL, NULL,
      (GClassInitFunc) ipatch_container_init_class, NULL, NULL,
      sizeof (IpatchContainer), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (IPATCH_TYPE_ITEM, "IpatchContainer",
					&item_info, G_TYPE_FLAG_ABSTRACT);

    _ipatch_container_notify_init (); /* init container add/remove notify system */
  }

  return (item_type);
}

static void
ipatch_container_init_class (IpatchContainerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  real_container_class = klass;

  obj_class->dispose = ipatch_container_dispose;

  item_class->remove_full = ipatch_container_item_remove_full;
}

static void
ipatch_container_dispose (GObject *object)
{
  /* clear hooks flag, we are finalizing, shouldn't be any hook callbacks
     or should there be?  FIXME */
  ipatch_item_clear_flags (IPATCH_ITEM (object), IPATCH_ITEM_HOOKS_ACTIVE);

  ipatch_container_remove_all (IPATCH_CONTAINER (object));

  if (parent_class->dispose)
    parent_class->dispose (object);
}

static void
ipatch_container_item_remove_full (IpatchItem *item, gboolean full)
{
  if (full)
    ipatch_container_remove_all (IPATCH_CONTAINER (item));

  if (IPATCH_ITEM_CLASS (parent_class)->remove_full)
    IPATCH_ITEM_CLASS (parent_class)->remove_full (item, full);
}

/**
 * ipatch_container_get_children: (skip)
 * @container: Container object
 * @type: GType of child items to get (will match type descendants as well)
 *
 * Get a list of child items from a container object. A new #IpatchList is
 * created containing all matching child items. The returned list object can
 * be iterated over safely even in a multi-thread environment. If
 * performance is an issue, ipatch_container_init_iter() can be used instead,
 * although it requires locking of the container object.
 *
 * Returns: (transfer full): New object list containing all matching items (an empty
 * list if no items matched). The caller owns a reference to the
 * object list and removing the reference will destroy it.
 */
IpatchList *
ipatch_container_get_children (IpatchContainer *container, GType type)
{
  IpatchList *list;
  GList *items;

  items = ipatch_container_get_children_by_type (container, type);
  list = ipatch_list_new ();
  list->items = items;
  return (list);
}

/**
 * ipatch_container_get_children_list: (rename-to ipatch_container_get_children)
 * @container: Container object
 *
 * Get a list of all child items from a container object. The returned list object can
 * be iterated over safely even in a multi-thread environment.
 *
 * Returns: (element-type Ipatch.Item) (transfer full) (nullable): New object list containing all child items (an empty
 * list if no children). Free the list with ipatch_glist_unref_free() when finished using it.
 *
 * Since: 1.1.0
 */
GList *
ipatch_container_get_children_list (IpatchContainer *container)
{
  return ipatch_container_get_children_by_type (container, IPATCH_TYPE_ITEM);
}

/**
 * ipatch_container_get_children_by_type:
 * @container: Container object
 * @type: GType of child items to get (will match type descendants as well)
 *
 * Get a list of child items from a container object. The returned list object can
 * be iterated over safely even in a multi-thread environment.
 *
 * Returns: (element-type Ipatch.Item) (transfer full) (nullable): New object list containing all matching items (an empty
 * list if no items matched). Free the list with ipatch_glist_unref_free() when finished using it.
 *
 * Since: 1.1.0
 */
GList *
ipatch_container_get_children_by_type (IpatchContainer *container, GType type)
{
  GList *list = NULL;
  const GType *child_types;
  IpatchIter iter;
  GObject *obj;

  g_return_val_if_fail (IPATCH_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (g_type_is_a (type, G_TYPE_OBJECT), NULL);

  /* get container child types */
  child_types = ipatch_container_get_child_types (container);
  while (*child_types)		/* loop over child types */
    {
      if (g_type_is_a (*child_types, type)) /* child type matches type? */
	{
	  IPATCH_ITEM_RLOCK (container);
	  ipatch_container_init_iter (container, &iter, *child_types);
	  obj = ipatch_iter_first (&iter);
	  while (obj)		/* add object list to children list */
	    {
	      g_object_ref (obj); /* ++ ref object for list */
	      list = g_list_prepend (list, obj);
	      obj = ipatch_iter_next (&iter);
	    }
	  IPATCH_ITEM_RUNLOCK (container);
	}

      child_types++;
    }

  return g_list_reverse (list);         /* since we prepended */
}

/**
 * ipatch_container_get_child_types:
 * @container: Container object
 *
 * Get an array of child types for a container object. The number of types
 * is the number of individual lists the container has.
 *
 * Returns: (array zero-terminated=1) (transfer none): Pointer to a zero
 * terminated array of types. Array is static and should not be modified or freed.
 */
const GType *
ipatch_container_get_child_types (IpatchContainer *container)
{
  IpatchContainerClass *klass;
  const GType *types;

  g_return_val_if_fail (IPATCH_IS_CONTAINER (container), 0);

  klass = IPATCH_CONTAINER_GET_CLASS (container);
  types = klass->child_types ();

  return (types);
}

/**
 * ipatch_container_get_virtual_types:
 * @container: Container object
 *
 * Get an array of virtual types for a container object.  Virtual types are
 * used to group child items in user interfaces (an example is SoundFont
 * "Percussion Presets" which contains all presets in bank number 128).  To
 * discover what virtual container a child object is part of lookup it's
 * "virtual-parent-type" type property (ipatch_type_get() or
 * ipatch_type_object_get()).
 *
 * Returns: (array zero-terminated=1) (transfer none): Pointer to a zero
 * terminated array of types or %NULL if no virtual types for @container.
 * Array is static and should not be modified or freed.
 */
const GType *
ipatch_container_get_virtual_types (IpatchContainer *container)
{
  IpatchContainerClass *klass;

  g_return_val_if_fail (IPATCH_IS_CONTAINER (container), 0);

  klass = IPATCH_CONTAINER_GET_CLASS (container);

  if (klass->virtual_types)
    return (klass->virtual_types ());
  else return (NULL);
}

/**
 * ipatch_container_type_get_child_types:
 * @container_type: A #IpatchContainer derived type to get child types of
 *
 * Get an array of child types for a container type. The number of types
 * is the number of individual lists the container has. Like
 * ipatch_container_get_child_types() but takes a container GType instead of
 * a container object.
 *
 * Returns: (array zero-terminated=1) (transfer none) (nullable): Pointer to a zero
 * terminated array of types or NULL if none. Array is static and should not be
 * modified or freed.
 */
const GType *
ipatch_container_type_get_child_types (GType container_type)
{
  IpatchContainerClass *klass;
  const GType *types;

  g_return_val_if_fail (g_type_is_a (container_type, IPATCH_TYPE_CONTAINER),
			NULL);

  klass = g_type_class_ref (container_type);

  if (klass->child_types)
    types = klass->child_types ();
  else types = NULL;

  g_type_class_unref (klass);

  return (types);
}

/**
 * ipatch_container_insert:
 * @container: Container item to insert into
 * @item: Item to insert
 * @pos: Index position to insert @item into (@item type is used to
 *   determine what list to insert into). 0 = first, less than 0 = last.
 *
 * Inserts an item into a patch item container.
 *
 * MT-NOTE: If position in list is critical the container item should
 * be locked and ipatch_container_insert_iter() used instead (see other
 * special requirements for using this function).
 * Only inserting in the first or last position (@pos is 0 or less than 0) 
 * is guaranteed.
 */
void
ipatch_container_insert (IpatchContainer *container, IpatchItem *item, int pos)
{
  const GType *child_types;
  IpatchIter iter;
  GType type;

  g_return_if_fail (IPATCH_IS_CONTAINER (container));
  g_return_if_fail (IPATCH_IS_ITEM (item));

  type = G_OBJECT_TYPE (item);

  /* get container child types */
  child_types = ipatch_container_get_child_types (container);

  for (; *child_types; child_types++)	/* loop over child types */
    if (g_type_is_a (type, *child_types)) /* item type matches child type? */
      break;

  if (*child_types)	/* matching child type found? */
    {
      IPATCH_ITEM_WLOCK (container);
      ipatch_container_init_iter (container, &iter, *child_types);

      /* if position is less than 1 or off the end, get last object */
      if (pos < 0 || !ipatch_iter_index (&iter, pos))
	ipatch_iter_last (&iter);

      ipatch_container_insert_iter (container, item, &iter);

      IPATCH_ITEM_WUNLOCK (container);

      ipatch_container_add_notify (container, item);	/* container add notify */
    }
  else
    g_critical (IPATCH_CONTAINER_ERRMSG_INVALID_CHILD_2, g_type_name (type),
		g_type_name (G_OBJECT_TYPE (container)));
}

/**
 * ipatch_container_append:
 * @container: Container item
 * @item: Item to append
 *
 * Appends an item to a container's children.
 */
void
ipatch_container_append (IpatchContainer *container, IpatchItem *item)
{
  ipatch_container_insert (container, item, -1);
}

/**
 * ipatch_container_add:
 * @container: Container item
 * @item: Item to append
 *
 * Just an alias for ipatch_container_append().
 */
void
ipatch_container_add (IpatchContainer *container, IpatchItem *item)
{
  ipatch_container_insert (container, item, -1);
}

/**
 * ipatch_container_prepend:
 * @container: Container item
 * @item: Item to insert
 *
 * Prepends an item to a container's children.
 */
void
ipatch_container_prepend (IpatchContainer *container, IpatchItem *item)
{
  ipatch_container_insert (container, item, 0);
}

/**
 * ipatch_container_remove:
 * @container: Container item to remove from
 * @item: Item to remove from @container
 *
 * Removes an @item from @container.
 */
void
ipatch_container_remove (IpatchContainer *container, IpatchItem *item)
{
  const GType *child_types;
  IpatchIter iter;
  GType type;
  GObject *obj;

  g_return_if_fail (IPATCH_IS_CONTAINER (container));
  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (ipatch_item_peek_parent (item) == (IpatchItem *)container);

  /* do container remove notify (despite the fact that the remove could be
   * invalid, not actually a child of container) */
  ipatch_container_remove_notify (container, item);

  type = G_OBJECT_TYPE (item);

  /* get container child types */
  child_types = ipatch_container_get_child_types (container);
  while (*child_types)		/* loop over child types */
    {
      if (g_type_is_a (type, *child_types)) /* item type matches child type? */
	{
	  IPATCH_ITEM_WLOCK (container);
	  ipatch_container_init_iter (container, &iter, *child_types);

	  /* search for @item */
	  obj = ipatch_iter_first (&iter);
	  while (obj && obj != (GObject *)item)
	    obj = ipatch_iter_next (&iter);

	  /* remove the object if found */
	  if (obj) ipatch_container_remove_iter (container, &iter);

	  IPATCH_ITEM_WUNLOCK (container);

	  if (obj) return; /* return if removed, otherwise keep searching */
	}

      child_types++;
    }

  g_critical ("Child of type '%s' not found in parent of type '%s'",
	      g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
}

/**
 * ipatch_container_remove_all:
 * @container: Container object
 *
 * Removes all items from a @container object.
 */
void
ipatch_container_remove_all (IpatchContainer *container)
{
  IpatchList *list;
  const GType *child_types, *ptype;
  GList *p;

  g_return_if_fail (IPATCH_IS_CONTAINER (container));

  child_types = ipatch_container_get_child_types (container);

  /* loop over child types */
  for (ptype = child_types; *ptype; ptype++)
    {
      list = ipatch_container_get_children (container, *ptype);	/* ++ ref new list */

      for (p = list->items; p; p = p->next)
	ipatch_container_remove (container, (IpatchItem *)(p->data));

      g_object_unref (list);	/* -- unref list */
    }
}

/**
 * ipatch_container_count:
 * @container: Container object to count children of
 * @type: Type of children to count
 *
 * Counts children of a specific @type (or derived thereof) in
 * a @container object.
 *
 * Returns: Count of children of @type in @container.
 */
guint
ipatch_container_count (IpatchContainer *container, GType type)
{
  const GType *child_types;
  IpatchIter iter;
  int count = 0;

  g_return_val_if_fail (IPATCH_IS_CONTAINER (container), 0);
  g_return_val_if_fail (g_type_is_a (type, G_TYPE_OBJECT), 0);

  /* get container child types */
  child_types = ipatch_container_get_child_types (container);
  while (*child_types)		/* loop over child types */
    {
      if (g_type_is_a (*child_types, type)) /* child type matches type? */
	{
	  IPATCH_ITEM_RLOCK (container);
	  ipatch_container_init_iter (container, &iter, *child_types);
	  count += ipatch_iter_count (&iter);
	  IPATCH_ITEM_RUNLOCK (container);
	}

      child_types++;
    }

  return (count);
}

/**
 * ipatch_container_make_unique:
 * @container: Patch item container
 * @item: An item of a type that can be added to @container (but not
 *   necessarily parented to @container).
 *
 * Ensures an @item's duplicate sensitive properties are unique among items of
 * the same type in @container. The item need not be a child of @container, but
 * can be. The initial values of the duplicate sensitive properties are used
 * if already unique, otherwise they are modified (name strings have numbers
 * appended to them, unused MIDI locale is found, etc).
 */
void
ipatch_container_make_unique (IpatchContainer *container, IpatchItem *item)
{
  IpatchContainerClass *klass;

  g_return_if_fail (IPATCH_IS_CONTAINER (container));
  g_return_if_fail (IPATCH_IS_ITEM (item));

  klass = IPATCH_CONTAINER_GET_CLASS (container);
  if (klass->make_unique)
    (*klass->make_unique)(container, item);
}

/**
 * ipatch_container_add_unique:
 * @container: Container to add item to
 * @item: Item to add
 *
 * Adds a patch item to a container and ensures that the item's duplicate
 * sensitive properties are unique (see ipatch_container_make_unique()).
 */
void
ipatch_container_add_unique (IpatchContainer *container, IpatchItem *item)
{
  g_return_if_fail (IPATCH_IS_CONTAINER (container));
  g_return_if_fail (IPATCH_IS_ITEM (item));

  IPATCH_ITEM_WLOCK (container);
  ipatch_container_make_unique (container, item);
  ipatch_container_append (container, item);
  IPATCH_ITEM_WUNLOCK (container);
}

/**
 * ipatch_container_init_iter:
 * @container: Container object
 * @iter: (out): Iterator structure to initialize
 * @type: Container child type list to initialize @iter to
 *
 * Initialize an iterator structure to a child list of the specified
 * @type in a @container object.
 *
 * MT-NOTE: The @container must be locked, or single thread access
 * ensured, for the duration of this call and iteration of @iter as
 * the @container object's lists are used directly. The iterator related
 * functions are meant to be used for latency critical operations, and they
 * should try and minimize the locking duration.
 *
 * Returns: %TRUE on success, %FALSE otherwise in which case the contents of
 * @iter are undefined.
 */
gboolean
ipatch_container_init_iter (IpatchContainer *container, IpatchIter *iter,
			    GType type)
{
  IpatchContainerClass *klass;

  g_return_val_if_fail (IPATCH_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (g_type_is_a (type, IPATCH_TYPE_ITEM), FALSE);

  klass = IPATCH_CONTAINER_GET_CLASS (container);
  g_return_val_if_fail (klass->init_iter != NULL, FALSE);

  return ((*klass->init_iter)(container, iter, type));
}

/**
 * ipatch_container_insert_iter:
 * @container: Container object
 * @item: Patch item to insert
 * @iter: (nullable): Iterator marking position to insert after (%NULL position to
 *   prepend).  Iterator is advanced after insert to point to new
 *   inserted item. Note that this makes appending multiple items rather
 *   fast.
 *
 * This function should not normally be used.  It is provided to allow for
 * custom high performance functions involving container adds.  If used, certain
 * precautions and requirements must be followed.
 *
 * Insert a patch @item into a @container after the position marked by
 * @iter. No checking is done to see if child @item is actually a valid
 * type in @container, this is left up to the caller for added performance.
 * Also left up to the caller is a call to ipatch_container_add_notify() to
 * notify that the item has been added (should be called after this function
 * and outside of @container lock).  It is not necessary to notify if hooks
 * are not enabled for @container or caller doesn't care.
 *
 * MT-NOTE: The @container object should be write locked, or single thread
 * access ensured, for the duration of this call and prior iterator functions.
 */
void
ipatch_container_insert_iter (IpatchContainer *container, IpatchItem *item,
			      IpatchIter *iter)
{
  g_return_if_fail (IPATCH_IS_CONTAINER (container));
  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (iter != NULL);

  ipatch_iter_insert (iter, (GObject *)item);
  g_object_ref ((GObject *)item); /* ++ ref object for container */
  ipatch_item_set_parent (item, (IpatchItem *)container); /* set item parent */
}

/**
 * ipatch_container_remove_iter:
 * @container: Container object
 * @iter: Iterator marking item to remove
 *
 * This function should not normally be used.  It is provided to allow for
 * custom high performance functions involving container removes.  If used,
 * certain precautions and requirements must be followed.
 *
 * Removes an item from a @container object. The item is specified by the
 * current position in @iter.  It is left up to the caller to call
 * ipatch_container_remove_notify() (should be called before this function
 * and out of @container lock) to notify that the item will be removed.
 * It is not necessary to notify if hooks are not enabled for @container or
 * caller doesn't care.
 *
 * MT-NOTE: The @container object should be write locked, or single thread
 * access ensured, for the duration of this call and prior iterator functions.
 */
void
ipatch_container_remove_iter (IpatchContainer *container, IpatchIter *iter)
{
  GObject *obj;

  g_return_if_fail (IPATCH_IS_CONTAINER (container));
  g_return_if_fail (iter != NULL);

  obj = ipatch_iter_get (iter);
  g_return_if_fail (obj != NULL);

  ipatch_iter_remove (iter);
  ipatch_item_unparent (IPATCH_ITEM (obj)); /* unparent item */
  g_object_unref (obj);		/* -- unref object for container */
}
