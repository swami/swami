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
 * SECTION: IpatchItem
 * @short_description: Abstract base item object
 * @see_also: 
 * @stability: Stable
 *
 * The abstract base item type from which all instrument objects are derived
 * and many other object types as well.
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchItem.h"
#include "IpatchIter.h"
#include "IpatchContainer.h"
#include "IpatchBase.h"		/* IPATCH_BASE_CHANGED flag */
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"
#include "i18n.h"
#include "util.h"

enum
{
  PROP_0,
  PROP_FLAGS,
  PROP_PARENT,
  PROP_BASE,
  PROP_TITLE
};

/* for unique property caching */
typedef struct
{
  GParamSpec **pspecs;	/* unique prop paramspecs (groups are consecutive) */
  guint32 groups;	/* 1 bit per pspec, pspecs of same group are the same */
} UniqueBag;

/* a private bag structure for ipatch_item_set_parent */
typedef struct
{
  IpatchItem *base;
  int hooks_active;
} SetParentBag;

/* defined in IpatchItemProp.c */
extern void _ipatch_item_prop_init (void);

/* defined in IpatchBase.c */
extern GParamSpec *ipatch_base_pspec_changed;


static void ipatch_item_base_init (IpatchItemClass *klass);
static void ipatch_item_class_init (IpatchItemClass *klass);
static void ipatch_item_set_property (GObject *object, guint property_id,
				      const GValue *value, GParamSpec *pspec);
static void ipatch_item_get_property (GObject *object, guint property_id,
				      GValue *value, GParamSpec *pspec);
static void ipatch_item_set_property_override (GObject *object,
					       guint property_id,
					       const GValue *value,
					       GParamSpec *pspec);
static void ipatch_item_init (IpatchItem *item);
static void ipatch_item_finalize (GObject *object);
static void ipatch_item_item_remove_full (IpatchItem *item, gboolean full);
static void ipatch_item_recursive_base_set (IpatchContainer *container,
					    SetParentBag *bag);
static void ipatch_item_recursive_base_unset (IpatchContainer *container);
static void ipatch_item_real_remove_full (IpatchItem *item, gboolean full);
static void ipatch_item_real_remove_recursive (IpatchItem *item, gboolean full);
static void copy_hash_to_list_GHFunc (gpointer key, gpointer value,
				      gpointer user_data);
static UniqueBag *item_lookup_unique_bag (GType type);
static gint unique_group_list_sort_func (gconstpointer a, gconstpointer b);


static GObjectClass *parent_class = NULL;

/* item class for fast hook function method calls */
static IpatchItemClass *real_item_class = NULL;

/* define the lock for the unique property cache hash */
G_LOCK_DEFINE_STATIC (unique_prop_cache);

/* cache of IpatchItem unique properties (hash: GType => UniqueBag) */
static GHashTable *unique_prop_cache = NULL;

/* a dummy bag pointer used to indicate that an item type has no unique props */
static UniqueBag no_unique_props;

/* store title property GParamSpec as a convenience to derived types */
GParamSpec *ipatch_item_pspec_title = NULL;


GType
ipatch_item_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchItemClass),
      (GBaseInitFunc) ipatch_item_base_init, NULL,
      (GClassInitFunc) ipatch_item_class_init, NULL, NULL,
      sizeof (IpatchItem), 0,
      (GInstanceInitFunc)ipatch_item_init,
    };

    item_type = g_type_register_static (G_TYPE_OBJECT, "IpatchItem",
					&item_info, G_TYPE_FLAG_ABSTRACT);

    /* create unique property cache (elements are never removed) */
    unique_prop_cache = g_hash_table_new (NULL, NULL);

    _ipatch_item_prop_init ();	/* Initialize property change callback system */
  }

  return (item_type);
}

static void
ipatch_item_base_init (IpatchItemClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  /* override the set property method for all derived IpatchItem types */
  obj_class->set_property = ipatch_item_set_property_override;
}

/* set_property override method that calls item_set_property method, derived
 * classes should use the item_set_property method instead of set_property. */
static void
ipatch_item_set_property_override (GObject *object, guint property_id,
				   const GValue *value, GParamSpec *pspec)
{
  IpatchItemClass *klass;
  GObjectClass *obj_class;
  gboolean hooks_active;
  GValue oldval = { 0 };
  GType type, prev_type;

  /* Get the class for the owning type of this parameter.
   * NOTE: This wont work for interfaces (handled below) or overridden
   * properties.  FIXME - Not sure how to handle overridden properties. */
  klass = g_type_class_peek (pspec->owner_type);

  /* property belongs to an interface? */
  if (!klass && G_TYPE_IS_INTERFACE (pspec->owner_type))
  {
    /* Unfortunately GObject doesn't give us an easy way to determine which
     * class implements an interface in an object's derived ancestry, so we're
     * left with this hack.. */

    prev_type = G_OBJECT_TYPE (object);
    type = prev_type;

    /* find type in object's ancestry that implements the interface by searching
     * for the type just before the first parent which doesn't conform to the
     * interface. */
    while ((type = g_type_parent (type)))
    {
      if (!g_type_is_a (type, pspec->owner_type)) break;
      prev_type = type;
    }

    if (prev_type) klass = g_type_class_peek (prev_type);
  }

  g_return_if_fail (klass != NULL);
  g_return_if_fail (klass->item_set_property != NULL);

  /* hook functions can be inactive for greater performance */
  hooks_active = (ipatch_item_get_flags (object) & IPATCH_ITEM_HOOKS_ACTIVE) != 0;

  /* fetch parameter's current value to use as oldval in property notify */
  if (hooks_active)
    {
      obj_class = (GObjectClass *)klass;
      g_return_if_fail (obj_class->get_property != NULL);

      g_value_init (&oldval, G_PARAM_SPEC_VALUE_TYPE (pspec));
      obj_class->get_property (object, property_id, &oldval, pspec);
    }

  klass->item_set_property (object, property_id, value, pspec);

  /* call property change  */
  if (hooks_active)
    {
      ipatch_item_prop_notify ((IpatchItem *)object, pspec, value, &oldval);
      g_value_unset (&oldval);
    }
}

static void
ipatch_item_class_init (IpatchItemClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  real_item_class = klass;
  parent_class = g_type_class_peek_parent (klass);

  klass->item_set_property = ipatch_item_set_property;

  obj_class->get_property = ipatch_item_get_property;
  obj_class->finalize = ipatch_item_finalize;

  klass->remove_full = ipatch_item_item_remove_full;

  g_object_class_install_property (obj_class, PROP_FLAGS,
		    g_param_spec_uint ("flags", _("Flags"), _("Flags"),
				       0, G_MAXUINT, 0,
				       G_PARAM_READABLE | IPATCH_PARAM_HIDE
				       | IPATCH_PARAM_NO_SAVE_CHANGE
				       | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, PROP_PARENT,
			  g_param_spec_object ("parent", _("Parent"),
					       _("Parent"),
					       IPATCH_TYPE_ITEM,
					       G_PARAM_READWRITE
					       | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, PROP_BASE,
			  g_param_spec_object ("base", _("Base"), _("Base"),
					       IPATCH_TYPE_BASE,
					       G_PARAM_READABLE
					       | IPATCH_PARAM_NO_SAVE));
  ipatch_item_pspec_title
    = g_param_spec_string ("title", _("Title"), _("Title"),
			   NULL, G_PARAM_READABLE | IPATCH_PARAM_NO_SAVE_CHANGE
			   | IPATCH_PARAM_NO_SAVE);
  g_object_class_install_property (obj_class, PROP_TITLE,
				   ipatch_item_pspec_title);
}

static void
ipatch_item_set_property (GObject *object, guint property_id,
			  const GValue *value, GParamSpec *pspec)
{
  IpatchItem *item = IPATCH_ITEM (object);

  switch (property_id)
    {
    case PROP_FLAGS:
      ipatch_item_set_flags (item, g_value_get_uint (value));
      break;
    case PROP_PARENT:
      ipatch_item_set_parent (item, IPATCH_ITEM (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_item_get_property (GObject *object, guint property_id,
			  GValue *value, GParamSpec *pspec)
{
  IpatchItem *item = IPATCH_ITEM (object);
  char *name;

  switch (property_id)
    {
    case PROP_FLAGS:
      g_value_set_uint (value, ipatch_item_get_flags (item));
      break;
    case PROP_PARENT:
      g_value_take_object (value, ipatch_item_get_parent (item));
      break;
    case PROP_BASE:
      g_value_take_object (value, ipatch_item_get_base (item));
      break;
    case PROP_TITLE:
      /* see if the "name" type property is set */
      ipatch_type_object_get ((GObject *)item, "name", &name, NULL);

      if (name) g_value_take_string (value, name);
      else g_value_set_string (value, IPATCH_UNTITLED);	/* "untitled" */
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_item_init (IpatchItem *item)
{
  /* always assign a mutex, will be freed and set to parent's mutex if the
     class has mutex_slave set (during ipatch_item_set_parent) */
  item->mutex = g_malloc (sizeof (GStaticRecMutex));
  g_static_rec_mutex_init (item->mutex);

  ipatch_item_set_flags (item, IPATCH_ITEM_FREE_MUTEX);
}

static void
ipatch_item_finalize (GObject *object)
{
  IpatchItem *item = IPATCH_ITEM (object);

  if (ipatch_item_get_flags (item) & IPATCH_ITEM_FREE_MUTEX)
    {
      g_static_rec_mutex_free (item->mutex);
      g_free (item->mutex);
    }

  item->mutex = NULL;

  if (parent_class->finalize)
    parent_class->finalize (object);
}

static void
ipatch_item_item_remove_full (IpatchItem *item, gboolean full)
{
  IpatchItem *parent;

  parent = ipatch_item_get_parent (item);       // ++ ref parent

  if (parent)
  {
    ipatch_container_remove (IPATCH_CONTAINER (parent), item);
    g_object_unref (parent);                    // -- unref parent
  }
}

/**
 * ipatch_item_get_flags:
 * @item: (type Ipatch.Item): #IpatchItem to get flag field from
 *
 * Get the value of the flags field in a patch item.
 *
 * Returns: Value of flags field (some of which is user definable).
 */
int
ipatch_item_get_flags (gpointer item)
{
  g_return_val_if_fail (IPATCH_IS_ITEM (item), 0);

  return (item ? g_atomic_int_get (&((IpatchItem *)item)->flags) : 0);
}

/**
 * ipatch_item_set_flags:
 * @item: (type Ipatch.Item): #IpatchItem to set flags in
 * @flags: Flags to set
 *
 * Set flags in a patch item. All bits that are set in @flags are set in
 * the @item flags field.
 */
void
ipatch_item_set_flags (gpointer item, int flags)
{
  int oldval, newval;

  g_return_if_fail (IPATCH_IS_ITEM (item));

  do
  {
    oldval = g_atomic_int_get (&((IpatchItem *)item)->flags);
    newval = oldval | flags;
  } while (!g_atomic_int_compare_and_exchange (&((IpatchItem *)item)->flags,
                                               oldval, newval));
}

/**
 * ipatch_item_clear_flags:
 * @item: (type Ipatch.Item): #IpatchItem object to clear flags in
 * @flags: Flags to clear
 *
 * Clear (unset) flags in a patch item. All bits that are set in @flags are
 * cleared in the @item flags field.
 */
void
ipatch_item_clear_flags (gpointer item, int flags)
{
  int oldval, newval;

  g_return_if_fail (IPATCH_IS_ITEM (item));

  do
  {
    oldval = g_atomic_int_get (&((IpatchItem *)item)->flags);
    newval = oldval & ~flags;
  } while (!g_atomic_int_compare_and_exchange (&((IpatchItem *)item)->flags,
                                               oldval, newval));
}

/**
 * ipatch_item_set_parent:
 * @item: Item to set parent of (should not have a parent).
 * @parent: New parent of item.
 *
 * Usually only used by #IpatchItem type implementations. Sets the
 * parent of a patch item. Also recursively sets base parent and
 * #IPATCH_ITEM_HOOKS_ACTIVE flag if set in @parent. Also assigns the item's
 * thread mutex to that of the parent if its class has mutex_slave set.
 * The @parent container is responsible for adding a reference to @item - this
 * function does not do so.
 *
 * NOTE: If the @item has mutex_slave set in its class then the item will use
 * @parent object's mutex.  During the call to this function @item should not
 * be accessed by any other threads, otherwise problems may occur when the
 * mutex is changed.  Normally this shouldn't be much of an issue, since new
 * item's are often only accessed by 1 thread until being parented.
 */
void
ipatch_item_set_parent (IpatchItem *item, IpatchItem *parent)
{
  IpatchItemClass *klass;
  gboolean is_container;
  SetParentBag bag;
  guint depth;
  int i;

  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (IPATCH_IS_ITEM (parent));
  g_return_if_fail (item != parent);

  bag.base = ipatch_item_get_base (parent); /* ++ ref base patch item */
  is_container = IPATCH_IS_CONTAINER (item);

  /* get value of parent's hook flag */
  bag.hooks_active = ipatch_item_get_flags (parent) & IPATCH_ITEM_HOOKS_ACTIVE;

  IPATCH_ITEM_WLOCK (item);
  if (log_if_fail (item->parent == NULL))
    {
      IPATCH_ITEM_WUNLOCK (item);
      if (bag.base) g_object_unref (bag.base);
      return;
    }

  klass = IPATCH_ITEM_GET_CLASS (item);
  if (klass->mutex_slave)
    {
      depth = g_static_rec_mutex_unlock_full (item->mutex);
      if (ipatch_item_get_flags (item) & IPATCH_ITEM_FREE_MUTEX)
	{
	  g_static_rec_mutex_free (item->mutex);
	  g_free (item->mutex);
	}

      item->mutex = parent->mutex;
      ipatch_item_clear_flags (item, IPATCH_ITEM_FREE_MUTEX);

      /* lock it the number of times old mutex was locked, we don't use
	 g_static_rec_mutex_lock_full because it could set depth rather than
	 add to it */
      for (i = 0; i < depth; i++)
	g_static_rec_mutex_lock (item->mutex);
    }

  item->parent = parent;
  if (bag.base) item->base = bag.base; /* inherit base parent item if set */

  /* inherit active hooks flag, has no effect if not set */
  ipatch_item_set_flags (item, bag.hooks_active);

  /* if item is a container and base or hooks active flag is set, recursively
     set children hooks active flags */
  if (is_container && (bag.base || bag.hooks_active))
    ipatch_item_recursive_base_set ((IpatchContainer *)item, &bag);

  IPATCH_ITEM_WUNLOCK (item);

  if (bag.base) g_object_unref (bag.base); /* -- unref base patch item */
}

/* recursively sets base field of a tree of items, expects container to
   be write locked */
static void
ipatch_item_recursive_base_set (IpatchContainer *container, SetParentBag *bag)
{
  IpatchIter iter;
  GObject *child;
  const GType *types;

  /* get container child list types */
  types = ipatch_container_get_child_types (container);
  while (*types)		/* loop over list types */
    { /* initialize iterator to child list */
      ipatch_container_init_iter (container, &iter, *types);
      child = ipatch_iter_first (&iter);
      while (child)		/* loop over child list */
	{
	  IPATCH_ITEM_WLOCK (child);

	  if (bag->base)	/* inherit base pointer if set */
	    ((IpatchItem *)child)->base = bag->base;

	  /* inherit active hooks flag, has no effect if not set */
	  ipatch_item_set_flags (child, bag->hooks_active);

	  if (IPATCH_IS_CONTAINER (child)) /* recurse if a container */
	    ipatch_item_recursive_base_set ((IpatchContainer *)child, bag);

	  IPATCH_ITEM_WUNLOCK (child);

	  child = ipatch_iter_next (&iter);
	} /* child loop */
      types++;
    } /* child type loop */
}

/**
 * ipatch_item_unparent:
 * @item: Item to unparent
 *
 * Usually only used by #IpatchItem type implementations. Unparent an
 * item. Also recursively unsets base parent and
 * #IPATCH_ITEM_HOOKS_ACTIVE flag. Parent container object is
 * responsible for removing reference of @item - this function does
 * not do so.
 */
void
ipatch_item_unparent (IpatchItem *item)
{
  gboolean is_container;

  g_return_if_fail (IPATCH_IS_ITEM (item));

  is_container = IPATCH_IS_CONTAINER (item);

  IPATCH_ITEM_WLOCK (item);
  if (!item->parent)
    {
      IPATCH_ITEM_WUNLOCK (item);
      return;
    }

  /* clear parent, base and active hooks flag of item */
  item->parent = NULL;
  item->base = NULL;
  ipatch_item_clear_flags (item, IPATCH_ITEM_HOOKS_ACTIVE);

  /* recursively unset base field and active hooks flag of all children */
  if (is_container)
    ipatch_item_recursive_base_unset ((IpatchContainer *)item);

  IPATCH_ITEM_WUNLOCK (item);
}

/* recursively unsets base field of a tree of items, expects container to
   be write locked */
static void
ipatch_item_recursive_base_unset (IpatchContainer *container)
{
  IpatchIter iter;
  GObject *child;
  const GType *types;

  types = ipatch_container_get_child_types (container);
  while (*types)		/* loop over list types */
    { /* initialize iterator to child list */
      ipatch_container_init_iter (container, &iter, *types);
      child = ipatch_iter_first (&iter);
      while (child)		/* loop over child list */
	{
	  IPATCH_ITEM_WLOCK (child);

	  /* unset base pointer and clear active hooks flag */
	  ((IpatchItem *)child)->base = NULL;
	  ipatch_item_clear_flags (child, IPATCH_ITEM_HOOKS_ACTIVE);

	  if (IPATCH_IS_CONTAINER (child)) /* recurse if a container */
	    ipatch_item_recursive_base_unset ((IpatchContainer *)child);

	  IPATCH_ITEM_WUNLOCK (child);

	  child = ipatch_iter_next (&iter);
	} /* child loop */
      types++;
    } /* child type loop */
}

/**
 * ipatch_item_get_parent:
 * @item: Item to get parent of
 *
 * Gets the parent of @item after incrementing its reference count. The
 * caller is responsible for decrementing the reference count with
 * g_object_unref when finished with it. This function is useful because
 * all code paths must contain references to the items that they are working
 * with in a multi-thread environment (there is no guarantee an item won't
 * be destroyed unless a refcount is held).
 *
 * Returns: (transfer full): The parent of @item or %NULL if not parented or a root item. If
 * not %NULL then the reference count of parent has been incremented and the
 * caller is responsible for removing it when finished with parent.
 */
IpatchItem *
ipatch_item_get_parent (IpatchItem *item)
{
  IpatchItem *parent;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  IPATCH_ITEM_RLOCK (item);
  parent = item->parent;
  if (parent) g_object_ref (parent);
  IPATCH_ITEM_RUNLOCK (item);

  return (parent);
}

/**
 * ipatch_item_peek_parent: (skip)
 * @item: Item to get the parent of
 *
 * This function is the same as ipatch_item_get_parent() (gets an
 * item's parent) but does not increment the parent's ref
 * count. Therefore this function should only be used when the caller
 * already holds a reference or when only the value of the pointer
 * will be used (not the contents), to avoid multi-thread problems.
 *
 * Returns: The parent of @item or %NULL if not parented or a root item.
 */
IpatchItem *
ipatch_item_peek_parent (IpatchItem *item)
{
  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);
  return (item->parent);
}

/**
 * ipatch_item_get_base:
 * @item: Item to get base parent of
 *
 * Gets the base parent of @item (toplevel patch file object) after
 * incrementing its reference count. The caller is responsible for
 * decrementing the reference count with g_object_unref when finished
 * with it. If @item is itself a #IpatchBase object then it is returned.
 *
 * Returns: (transfer full): The base parent of @item or %NULL if no #IpatchBase type
 * in parent ancestry. If not %NULL then the reference count of
 * parent has been incremented and the caller is responsible for
 * removing it when finished with the base parent.
 */
IpatchItem *
ipatch_item_get_base (IpatchItem *item)
{
  IpatchItem *base;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  /* return the item if it is itself an #IpatchBase object */
  if (IPATCH_IS_BASE (item)) return (g_object_ref (item));

  IPATCH_ITEM_RLOCK (item);
  base = item->base;
  if (base) g_object_ref (base);
  IPATCH_ITEM_RUNLOCK (item);

  return (base);
}

/**
 * ipatch_item_peek_base: (skip)
 * @item: Item to get the base parent of
 *
 * This function is the same as ipatch_item_get_base() (gets an
 * item's toplevel base parent) but does not increment the parent's ref
 * count. Therefore this function should only be used when the caller
 * already holds a reference or when only the value of the pointer
 * will be used (not the contents), to avoid multi-thread problems.
 *
 * Returns: The base parent of @item or %NULL if no #IpatchBase type
 * in parent ancestry.
 */
IpatchItem *
ipatch_item_peek_base (IpatchItem *item)
{
  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  /* return the item if it is itself an #IpatchBase object */
  if (IPATCH_IS_BASE (item)) return (item);

  return (item->base);
}

/**
 * ipatch_item_get_ancestor_by_type:
 * @item: Item to search for parent of a given type
 * @ancestor_type: Type of parent in the @item object's ancestry
 *
 * Searches for the first parent item that is derived from
 * @ancestor_type in the @item object's ancestry. @ancestor_type must
 * be an #IpatchItem derived type (as well as the ancestry of
 * @item). Of note is that @item can also match. The returned item's
 * reference count has been incremented and it is the responsiblity of
 * the caller to remove it with g_object_unref() when finished with
 * it.
 *
 * Returns: (transfer full): The matched item (can be the @item itself) or %NULL if no
 * item matches @ancestor_type in the ancestry. Remember to unref the
 * matched item when done with it.
 */
IpatchItem *
ipatch_item_get_ancestor_by_type (IpatchItem *item, GType ancestor_type)
{
#define MAX_ITEM_DEPTH 10
  IpatchItem *p, *ancestry[MAX_ITEM_DEPTH];
  int i, depth = -1;

  g_return_val_if_fail (IPATCH_ITEM (item), NULL);
  g_return_val_if_fail (g_type_is_a (ancestor_type, IPATCH_TYPE_ITEM), NULL);

  p = item;
  do
    {
      if (g_type_is_a (G_OBJECT_TYPE (p), ancestor_type))
	break;

      depth++;
      g_assert (depth < MAX_ITEM_DEPTH);
      p = ancestry[depth] = ipatch_item_get_parent (p);
    }
  while (p);

  if (depth >= 0)		/* unreference ancestry */
    for (i = 0; i <= depth; i++)
      if (ancestry[i] != p) g_object_unref (ancestry[i]);

  if (p == item) g_object_ref (p);

  return (p);
}

/**
 * ipatch_item_peek_ancestor_by_type: (skip)
 * @item: Item to search for parent of a given type
 * @ancestor_type: Type of parent in the @item object's ancestry
 *
 * Just like ipatch_item_get_ancestor_by_type() but doesn't ref returned item.
 * This should only be used when caller already owns a reference or only the
 * pointer value is of importance, since otherwise there is a potential
 * multi-thread race condition.
 *
 * Returns: The matched item (can be the @item itself) or %NULL if no
 * item matches @ancestor_type in the ancestry. Remember that the returned
 * item is not referenced.
 */
IpatchItem *
ipatch_item_peek_ancestor_by_type (IpatchItem *item, GType ancestor_type)
{
  IpatchItem *match;

  match = ipatch_item_get_ancestor_by_type (item, ancestor_type);
  if (match) g_object_unref (match);
  return (match);
}

/**
 * ipatch_item_remove:
 * @item: Item to remove
 *
 * This function removes an item from its parent container and removes other
 * references from within the same patch (e.g., a #IpatchSF2Sample
 * will be removed from its parent #IpatchSF2 and all #IpatchSF2IZone objects
 * referencing the sample will also be removed).
 */
void
ipatch_item_remove (IpatchItem *item)
{
  g_return_if_fail (IPATCH_IS_ITEM (item));
  ipatch_item_real_remove_full (item, FALSE);
}

/**
 * ipatch_item_remove_full:
 * @item: Item to remove
 * @full: %TRUE removes all references to and from @item, %FALSE removes only references to @item
 *
 * Like ipatch_item_remove() but will also remove all references from @item (in the same #IpatchBase
 * object) if @full is %TRUE, behaves like ipatch_item_remove() if @full is %FALSE.
 *
 * Since: 1.1.0
 */
void
ipatch_item_remove_full (IpatchItem *item, gboolean full)
{
  g_return_if_fail (IPATCH_IS_ITEM (item));
  ipatch_item_real_remove_full (item, full);
}

static void
ipatch_item_real_remove_full (IpatchItem *item, gboolean full)
{
  IpatchItemClass *klass;
  IpatchItem *parent;

  klass = IPATCH_ITEM_GET_CLASS (item);

  if (klass->remove_full)
    (*klass->remove_full)(item, full);  /* call the remove_full class method */
  else if (klass->remove)
  {
    (*klass->remove)(item);     /* call the remove class method */

    // If full removal specified and item only has older remove method, remove all children
    if (full && IPATCH_IS_CONTAINER (item))
      ipatch_container_remove_all (IPATCH_CONTAINER (item));
  }
  else
  { // Default remove if no class method
    parent = ipatch_item_get_parent (item);

    if (parent) // Remove item from parent
    {
      ipatch_container_remove (IPATCH_CONTAINER (parent), item);
      g_object_unref (parent);
    }

    // If full removal specified, remove all children
    if (full && IPATCH_IS_CONTAINER (item))
      ipatch_container_remove_all (IPATCH_CONTAINER (item));
  }
}

/**
 * ipatch_item_remove_recursive:
 * @item: Item to recursively remove
 * @full: %TRUE removes all references to and from items, %FALSE removes only references to items
 *
 * This function calls ipatch_item_remove_full() on @item and all of its children recursively.
 *
 * Since: 1.1.0
 */
void
ipatch_item_remove_recursive (IpatchItem *item, gboolean full)
{
  g_return_if_fail (IPATCH_IS_ITEM (item));
  ipatch_item_real_remove_recursive (item, full);
}

static void
ipatch_item_real_remove_recursive (IpatchItem *item, gboolean full)
{
  const GType *child_types, *ptype;
  IpatchList *list;
  GList *p;

  if (IPATCH_IS_CONTAINER (item))
  {
    child_types = ipatch_container_get_child_types ((IpatchContainer *)item);

    /* loop over child types */
    for (ptype = child_types; *ptype; ptype++)
    {
      list = ipatch_container_get_children ((IpatchContainer *)item, *ptype);   /* ++ ref new list */

      if (g_type_is_a (*ptype, IPATCH_TYPE_CONTAINER))
      {
        for (p = list->items; p; p = p->next)
          ipatch_item_real_remove_recursive ((IpatchItem *)(p->data), full);
      }
      else      // Shortcut for non-container types (removes a level of unnecessary recursiveness)
      {
        for (p = list->items; p; p = p->next)
          ipatch_item_real_remove_full ((IpatchItem *)(p->data), full);
      }

      g_object_unref (list);      /* -- unref list */
    }
  }

  ipatch_item_real_remove_full (item, full);
}

/**
 * ipatch_item_changed:
 * @item: Item that has changed
 *
 * This function should be called when an item's saveable state (what ends
 * up in a patch file) has changed. Causes the @item object's base parent
 * to have its changed flag set, indicating that the file has changes that
 * have not been saved.  Note that this function is automatically called when
 * an ipatch_item_prop_notify() occurs for a property without the
 * #IPATCH_PARAM_NO_SAVE_CHANGE flag in its #GParamSpec and therefore this
 * function should not be called additionally.
 */
void
ipatch_item_changed (IpatchItem *item)
{
  IpatchItem *base = NULL;

  g_return_if_fail (IPATCH_IS_ITEM (item));

  IPATCH_ITEM_RLOCK (item);

  if (item->base) base = item->base;
  else if (IPATCH_IS_BASE (item)) base = item;

  if (base && !(base->flags & IPATCH_BASE_CHANGED))
  {
    g_object_ref (base);	/* ++ ref base object */
    ipatch_item_set_flags (base, IPATCH_BASE_CHANGED);
  }
  else base = NULL;

  IPATCH_ITEM_RUNLOCK (item);

  if (base)	/* do property notify for base "changed" flag */
  {
    ipatch_item_prop_notify (base, ipatch_base_pspec_changed,
			     ipatch_util_value_bool_true,
			     ipatch_util_value_bool_false);
    g_object_unref (base);	/* -- unref base parent object */
  }
}

/**
 * ipatch_item_get_property_fast:
 * @item: Item to get property value from
 * @pspec: Parameter spec of property to get
 * @value: Uninitialized caller supplied value to store the current value in.
 *   Remember to call g_value_unset when done with it.
 *
 * Usually only used by #IpatchItem implementations.  A faster way to retrieve
 * an object property.  Often used for fetching current value for property
 * notifies.  Notice that this function currently doesn't work with interface
 * or class overridden properties.
 */
void
ipatch_item_get_property_fast (IpatchItem *item, GParamSpec *pspec,
			       GValue *value)
{
  GObjectClass *obj_class;

  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (value != NULL);

  obj_class = g_type_class_peek (pspec->owner_type);
  g_return_if_fail (obj_class != NULL);
  g_return_if_fail (obj_class->get_property != NULL);

  g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  obj_class->get_property ((GObject *)item, IPATCH_PARAM_SPEC_ID (pspec),
			   value, pspec);
}

/**
 * ipatch_item_copy:
 * @dest: New destination item to copy to (should be same type or derived
 *   from @src type)
 * @src: Item to copy from
 *
 * Copy an item using the item class' "copy" method.  Object links are
 * copied as is, meaning the resulting object is a local object to the same
 * #IpatchBase.
 */
void
ipatch_item_copy (IpatchItem *dest, IpatchItem *src)
{
  IpatchItemClass *klass;
  GType dest_type, src_type;

  g_return_if_fail (IPATCH_IS_ITEM (dest));
  g_return_if_fail (IPATCH_IS_ITEM (src));
  dest_type = G_OBJECT_TYPE (dest);
  src_type = G_OBJECT_TYPE (src);
  g_return_if_fail (g_type_is_a (dest_type, src_type));

  klass = IPATCH_ITEM_GET_CLASS (src);
  g_return_if_fail (klass->copy != NULL);

  klass->copy (dest, src, NULL, NULL);
}

/**
 * ipatch_item_copy_link_func:
 * @dest: New destination item to copy to (should be same type or derived
 *   from @src type)
 * @src: Item to copy from
 * @link_func: (scope call) (closure user_data): Function to call for each
 *   object link pointer in @src.  This function can alter the copied link if desired.
 * @user_data: (nullable): User defined data to pass to @link_func.
 *
 * Copy an item using the item class' "copy" method.  Like ipatch_item_copy()
 * but takes a @link_func which can be used for handling replication of
 * externally linked objects (recursively deep duplicate for example, although
 * there is ipatch_item_duplicate_deep() specifically for that).
 */
void
ipatch_item_copy_link_func (IpatchItem *dest, IpatchItem *src,
			    IpatchItemCopyLinkFunc link_func,
			    gpointer user_data)
{
  IpatchItemClass *klass;
  GType dest_type, src_type;

  g_return_if_fail (IPATCH_IS_ITEM (dest));
  g_return_if_fail (IPATCH_IS_ITEM (src));
  g_return_if_fail (link_func != NULL);

  dest_type = G_OBJECT_TYPE (dest);
  src_type = G_OBJECT_TYPE (src);
  g_return_if_fail (g_type_is_a (dest_type, src_type));

  klass = IPATCH_ITEM_GET_CLASS (src);
  g_return_if_fail (klass->copy != NULL);

  klass->copy (dest, src, link_func, user_data);
}

/**
 * ipatch_item_copy_replace:
 * @dest: New destination item to copy to (should be same type or derived
 *   from @src type)
 * @src: Item to copy from
 * @repl_hash: (element-type IpatchItem IpatchItem): Hash of link pointer
 *   substitutions.  The original link pointer is the hash key, and the
 *   hash value is the replacement pointer.
 *
 * Like ipatch_item_copy() but takes a link replacement hash, which can be
 * used for substituting different objects for object links.  See
 * ipatch_item_copy_link_func() if even more flexibility is desired.
 */
void
ipatch_item_copy_replace (IpatchItem *dest, IpatchItem *src,
			  GHashTable *repl_hash)
{
  IpatchItemClass *klass;
  GType dest_type, src_type;

  g_return_if_fail (IPATCH_IS_ITEM (dest));
  g_return_if_fail (IPATCH_IS_ITEM (src));
  dest_type = G_OBJECT_TYPE (dest);
  src_type = G_OBJECT_TYPE (src);
  g_return_if_fail (g_type_is_a (dest_type, src_type));

  klass = IPATCH_ITEM_GET_CLASS (src);
  g_return_if_fail (klass->copy != NULL);

  klass->copy (dest, src, ipatch_item_copy_link_func_hash, repl_hash);
}

/**
 * ipatch_item_duplicate:
 * @item: Item to duplicate
 *
 * A convenience function to duplicate an item. Like ipatch_item_copy() but
 * creates a new duplicate item, rather than using an existing item.  Note
 * that externally linked objects are not duplicated for non #IpatchBase
 * derived types, making the item local to the same #IpatchBase object.
 *
 * Returns: (transfer full): New duplicate item. Caller owns the reference to the new item.
 */
IpatchItem *
ipatch_item_duplicate (IpatchItem *item)
{
  IpatchItem *newitem;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  newitem = g_object_new (G_OBJECT_TYPE (item), NULL); /* ++ ref new item */
  g_return_val_if_fail (newitem != NULL, NULL);

  ipatch_item_copy (newitem, item);
  return (newitem);		/* !! caller owns reference */
}

/**
 * ipatch_item_duplicate_link_func:
 * @item: Item to duplicate
 * @link_func: (scope call) (closure user_data): Function to call for each
 *   object link pointer in @item.  This function can alter the copied link if desired.
 * @user_data: (nullable): User defined data to pass to @link_func.
 *
 * Like ipatch_item_copy_link_func() but duplicates the item instead of
 * copying to an existing item.
 *
 * Returns: (transfer full): New duplicate item. Caller owns the reference to the new item.
 */
IpatchItem *
ipatch_item_duplicate_link_func (IpatchItem *item,
				 IpatchItemCopyLinkFunc link_func,
				 gpointer user_data)
{
  IpatchItem *newitem;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  newitem = g_object_new (G_OBJECT_TYPE (item), NULL); /* ++ ref new item */
  g_return_val_if_fail (newitem != NULL, NULL);

  ipatch_item_copy_link_func (newitem, item, link_func, user_data);
  return (newitem);		/* !! caller owns reference */
}

/**
 * ipatch_item_duplicate_replace:
 * @item: Item to duplicate
 * @repl_hash: (element-type IpatchItem IpatchItem): Hash of link pointer
 *   substitutions.  The original link pointer is the hash key, and the
 *   hash value is the replacement pointer.
 *
 * Like ipatch_item_copy_replace() but duplicates the item instead of
 * copying it to an already created item.
 *
 * Returns: (transfer full): New duplicate item. Caller owns the reference to the new item.
 */
IpatchItem *
ipatch_item_duplicate_replace (IpatchItem *item, GHashTable *repl_hash)
{
  IpatchItem *newitem;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  newitem = g_object_new (G_OBJECT_TYPE (item), NULL); /* ++ ref new item */
  g_return_val_if_fail (newitem != NULL, NULL);

  ipatch_item_copy_replace (newitem, item, repl_hash);
  return (newitem);		/* !! caller owns reference */
}

/**
 * ipatch_item_duplicate_deep:
 * @item: Item to deep duplicate
 *
 * Recursively duplicate an item (i.e., its dependencies also).
 *
 * Returns: (transfer full): List of duplicated @item and dependencies
 * (duplicated @item is the first in the list).  List object is owned by
 * the caller and should be unreferenced when finished using it.
 */
IpatchList *
ipatch_item_duplicate_deep (IpatchItem *item)
{
  IpatchItemClass *klass;
  IpatchItem *newitem;
  IpatchList *list;
  GHashTable *linkhash;	/* link substitutions */

  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  klass = IPATCH_ITEM_GET_CLASS (item);
  g_return_val_if_fail (klass->copy != NULL, NULL);

  newitem = g_object_new (G_OBJECT_TYPE (item), NULL); /* ++ ref new item */
  g_return_val_if_fail (newitem != NULL, NULL);

  /* create hash to track new duplicated items */
  linkhash = g_hash_table_new (NULL, NULL);
  g_hash_table_insert (linkhash, item, newitem);

  /* do the copy using the deep copy link function callback */
  klass->copy (newitem, item, ipatch_item_copy_link_func_deep, linkhash);

  list = ipatch_list_new ();	/* ++ref new list */

  /* remove newitem, before convert to list */
  g_hash_table_remove (linkhash, newitem);

  /* convert hash to IpatchList (!! list takes over references) */
  g_hash_table_foreach (linkhash, copy_hash_to_list_GHFunc, list);

  /* prepend newitem (!! List takes over creator's reference) */
  list->items = g_list_prepend (list->items, newitem);

  g_hash_table_destroy (linkhash);

  return (list);
}

/* GHFunc to convert hash table to IpatchList */
static void copy_hash_to_list_GHFunc (gpointer key, gpointer value,
				      gpointer user_data)
{
  IpatchList *list = (IpatchList *)user_data;

  list->items = g_list_prepend (list->items, value);
}

/**
 * ipatch_item_copy_link_func_deep: (skip)
 * @item: Item which is being linked (contains a reference to @link).
 * @link: The item being referenced (can be %NULL).
 * @user_data: User data supplied to copy/duplicate function.
 *
 * IpatchItemCopyLinkFunc for deep duplicating an object and dependencies.
 *
 * Returns: Pointer to item to use for link property (duplicated).
 */
IpatchItem *
ipatch_item_copy_link_func_deep (IpatchItem *item, IpatchItem *link,
				 gpointer user_data)
{
  GHashTable *linkhash = (GHashTable *)user_data;
  IpatchItem *dup = NULL;

  if (!link) return (NULL);

  /* look up link item in duplicate hash. */
  if (linkhash) dup = g_hash_table_lookup (linkhash, link);

  if (!dup)	/* link not in hash? - Duplicate link and add it to hash. */
    {
      dup = g_object_new (G_OBJECT_TYPE (link), NULL); /* ++ ref new item */
      g_return_val_if_fail (dup != NULL, NULL);

      /* !! hash table takes over reference */
      g_hash_table_insert (linkhash, link, dup);
      ipatch_item_copy_link_func (dup, link, ipatch_item_copy_link_func_deep,
				  user_data);
    }

  return (dup);
}

/**
 * ipatch_item_copy_link_func_hash: (skip)
 * @item: Item which is being linked (contains a reference to @link).
 * @link: The item being referenced.
 * @user_data: (type GHashTable): User data supplied to copy/duplicate function.
 *
 * A predefined #IpatchItemCopyLinkFunc which takes a hash for the @user_data
 * component, which is used for replacing link pointers.  @link is used as the
 * hash key, the hash value is used in its place if present otherwise @link is
 * used unchanged.
 *
 * Returns: Value in hash (@user_data) if @link is present in hash, or @link
 * itself if not.
 */
IpatchItem *
ipatch_item_copy_link_func_hash (IpatchItem *item, IpatchItem *link,
				 gpointer user_data)
{
  GHashTable *hash = (GHashTable *)user_data;
  IpatchItem *repl = NULL;

  if (!link) return (NULL);

  if (hash) repl = g_hash_table_lookup (hash, link);
  return (repl ? repl : link);
}

/**
 * ipatch_item_type_can_conflict:
 * @item_type: Type to test
 *
 * Test if a given #IpatchItem derived type can conflict with another item
 * (only applies to items of the same type in the same #IpatchBase object).
 *
 * Returns: %TRUE if @item_type can conflict (has properties which should be
 * unique among its siblings), %FALSE otherwise.
 */
gboolean
ipatch_item_type_can_conflict (GType item_type)
{
  UniqueBag *unique;

  unique = item_lookup_unique_bag (item_type);
  return (unique != NULL);
}

/**
 * ipatch_item_type_get_unique_specs:
 * @item_type: Type to get unique parameter specs for
 * @groups: (out) (optional): Location to store an integer describing groups.
 *   Each bit corresponds to a GParamSpec in the returned array.  GParamSpecs of the
 *   same group will have the same bit value (0 or 1) and be consecutive.
 *   Unique properties in the same group must all match (logic AND) for a
 *   conflict to occur.  Pass %NULL to ignore.
 *
 * Get the list of unique parameter specs which can conflict for a given
 * item type.
 *
 * Returns: (array zero-terminated=1) (transfer none): %NULL terminated list of
 * parameter specs or %NULL if @item_type can never conflict.  The returned
 * array is internal and should not be modified or freed.
 */
GParamSpec **
ipatch_item_type_get_unique_specs (GType item_type, guint32 *groups)
{
  UniqueBag *unique;

  unique = item_lookup_unique_bag (item_type);

  if (unique)
    {
      if (groups) *groups = unique->groups;
      return (unique->pspecs);
    }
  else
    {
      if (groups) *groups = 0;
      return (NULL);
    }
}

/**
 * ipatch_item_get_unique_props:
 * @item: Item to get unique properties of
 *
 * Get the values of the unique properties for @item.  This is useful when
 * doing conflict detection processing and more flexibility is desired than
 * with ipatch_item_test_conflict().
 *
 * Returns: GValueArray of the unique property values of @item in the same
 * order as the parameter specs returned by ipatch_item_type_get_unique_specs().
 * %NULL is returned if @item doesn't have any unique properties.
 * Use g_value_array_free() to free the array when finished using it.
 */
GValueArray *
ipatch_item_get_unique_props (IpatchItem *item)
{
  GParamSpec **ps;
  UniqueBag *unique;
  GValueArray *vals;
  GValue *value;
  int count, i;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);

  /* item has any unique properties? */
  unique = item_lookup_unique_bag (G_OBJECT_TYPE (item));
  if (!unique) return (NULL);

  /* count param specs */
  for (count = 0, ps = unique->pspecs; *ps; count++, ps++);

  vals = g_value_array_new (count);

  for (i = 0, ps = unique->pspecs; i < count; i++, ps++)
    { /* NULL will append an unset value, saves a value copy operation */
      g_value_array_append (vals, NULL);
      value = g_value_array_get_nth (vals, i);
      ipatch_item_get_property_fast (item, *ps, value);
    }

  return (vals);
}

/**
 * ipatch_item_test_conflict:
 * @item1: First item to test
 * @item2: Second item to test
 *
 * Test if two items would conflict if they were siblings (doesn't actually
 * test if they are siblings).
 *
 * Returns: Flags of unique properties which conflict.  The bit index
 * corresponds to the parameter index in the array returned by
 * ipatch_item_type_get_unique_specs().  0 is returned if the items do not
 * conflict.
 */
guint
ipatch_item_test_conflict (IpatchItem *item1, IpatchItem *item2)
{
  GValue val1 = { 0 }, val2 = { 0 };
  UniqueBag *unique;
  GParamSpec *pspec;
  GType type;
  guint conflicts;
  guint mask;
  int i, count, toggle, groupcount;

  g_return_val_if_fail (IPATCH_IS_ITEM (item1), 0);
  g_return_val_if_fail (IPATCH_IS_ITEM (item2), 0);

  type = G_OBJECT_TYPE (item1);

  /* can't conflict if items not the same type */
  if (type != G_OBJECT_TYPE (item2)) return (0);

  unique = item_lookup_unique_bag (type);
  if (!unique) return (0);	/* no unique properties? */

  conflicts = 0;
  for (i = 0; TRUE; i++)
    {
      pspec = unique->pspecs[i];
      if (!pspec) break;

      ipatch_item_get_property_fast (item1, pspec, &val1);
      ipatch_item_get_property_fast (item2, pspec, &val2);

      if (g_param_values_cmp (pspec, &val1, &val2) == 0)
	conflicts |= (1 << i);

      g_value_unset (&val1);
      g_value_unset (&val2);
    }

  /* mask out any unique groups which don't have all its props conflicting */

  count = i;
  mask = 1;
  groupcount = 1;
  toggle = (unique->groups & 1);	/* to track group changes */

  for (i = 1; i < count; i++)
    {	/* same group as prev. property? */
      if (toggle == ((unique->groups & (1 << i)) != 0))
	{
	  mask |= (1 << i);	/* add another mask bit */
	  groupcount++;		/* increment group property count */
	}
      else	/* group changed */
	{ /* prev group is larger than 1 prop. and not all props conflicting? */
	  if (groupcount > 1 && (conflicts & mask) != mask)
	    conflicts &= ~mask;	/* clear all conflicts for prev group */

	  toggle ^= 1;
	  mask = (1 << i);
	  groupcount = 1;
	}
    }

  /* check if last gorup is larger than 1 prop. and not all props conflicting */
  if (groupcount > 1 && (conflicts & mask) != mask)
    conflicts &= ~mask;	/* clear all conflicts for last group */

  return (conflicts);
}

/* lookup cached unique property info for a given type */
static UniqueBag *
item_lookup_unique_bag (GType type)
{
  GParamSpec **pspecs, *ps;
  GList *speclist = NULL;	/* sorted spec list (sorted by unique group) */
  UniqueBag *unique;
  gpointer klass;
  guint n_props, count;
  guint i;

  G_LOCK (unique_prop_cache);

  unique = g_hash_table_lookup (unique_prop_cache, GUINT_TO_POINTER (type));

  if (!unique)			/* has not been cached yet? */
    {
      klass = g_type_class_ref (type);
      g_return_val_if_fail (klass != NULL, NULL);

      /* get property list and add unique properties to speclist */
      pspecs = g_object_class_list_properties (klass, &n_props);
      for (i = 0, count = 0; i < n_props; i++)
	{
	  ps = pspecs[i];
	  if (ps->flags & IPATCH_PARAM_UNIQUE)	/* unique property? */
	    {	/* add to speclist sorted by unique group (0 = nogroup) */
	      speclist = g_list_insert_sorted (speclist, ps,
					       unique_group_list_sort_func);
	      count++;
	    }
	}

      g_free (pspecs);

      if (speclist)	/* any unique properties? */
	{
	  guint group, lastgroup = 0;
	  int toggle = 0;
	  GList *p;

	  /* create unique bag */
	  unique = g_new (UniqueBag, 1);
	  unique->pspecs = g_new (GParamSpec *, count + 1);
	  unique->groups = 0;

	  /* add pointers to unique spec array and set group bit toggle field */
	  for (i = 0, p = speclist; i < count; i++, p = p->next)
	    {
	      unique->pspecs[i] = (GParamSpec *)(p->data);
	      ipatch_param_get ((GParamSpec *)(p->data),
				"unique-group-id", &group, NULL);
	      if (group != lastgroup) toggle ^= 1;
	      if (toggle) unique->groups |= (1 << i);

	      lastgroup = group;
	    }

	  unique->pspecs[count] = NULL;	/* NULL terminated */
	}
      else unique = &no_unique_props;	/* indicate no unique properties */
      
      g_hash_table_insert (unique_prop_cache, GUINT_TO_POINTER (type), unique);
    }

  G_UNLOCK (unique_prop_cache);

  if (unique == &no_unique_props)	/* no unique properties? */
    unique = NULL;

  return (unique);
}

/* sort GParamSpec pointers by their unique group id (0 = no group) */
static gint
unique_group_list_sort_func (gconstpointer a, gconstpointer b)
{
  GParamSpec *aspec = (GParamSpec *)a, *bspec = (GParamSpec *)b;
  int agroup, bgroup;

  ipatch_param_get (aspec, "unique-group-id", &agroup, NULL);
  ipatch_param_get (bspec, "unique-group-id", &bgroup, NULL);

  return (agroup - bgroup);
}

/**
 * ipatch_item_set_atomic:
 * @item: (type IpatchItem): IpatchItem derived object to set properties of
 * @first_property_name: Name of first property
 * @...: Variable list of arguments that should start with the value to
 *   set @first_property_name to, followed by property name/value pairs. List is
 *   terminated with a %NULL property name.
 *
 * Sets properties on a patch item atomically (i.e. item is
 * multi-thread locked while all properties are set). This avoids
 * critical parameter sync problems when multiple threads are
 * accessing the same item. See g_object_set() for more information on
 * setting properties.
 */
void
ipatch_item_set_atomic (gpointer item, const char *first_property_name, ...)
{
  va_list args;

  g_return_if_fail (IPATCH_IS_ITEM (item));

  va_start (args, first_property_name);

  IPATCH_ITEM_WLOCK (item);
  g_object_set_valist (G_OBJECT (item), first_property_name, args);
  IPATCH_ITEM_WUNLOCK (item);

  va_end (args);
}

/**
 * ipatch_item_get_atomic:
 * @item: (type IpatchItem): IpatchItem derived object to get properties from
 * @first_property_name: Name of first property
 * @...: Variable list of arguments that should start with a
 *   pointer to store the value from @first_property_name, followed by
 *   property name/value pointer pairs. List is terminated with a %NULL
 *   property name.
 *
 * Gets properties from a patch item atomically (i.e. item is
 * multi-thread locked while all properties are retrieved). This
 * avoids critical parameter sync problems when multiple threads are
 * accessing the same item. See g_object_get() for more information on
 * getting properties.
 */
void
ipatch_item_get_atomic (gpointer item, const char *first_property_name, ...)
{
  va_list args;

  g_return_if_fail (IPATCH_IS_ITEM (item));

  va_start (args, first_property_name);

  IPATCH_ITEM_WLOCK (item);
  g_object_get_valist (G_OBJECT (item), first_property_name, args);
  IPATCH_ITEM_WUNLOCK (item);

  va_end (args);
}

/**
 * ipatch_item_first: (skip)
 * @iter: Patch item iterator containing #IpatchItem items
 *
 * Gets the first item in a patch item iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first item in @iter or %NULL if empty.
 */
IpatchItem *
ipatch_item_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_ITEM (obj));
  else return (NULL);
}

/**
 * ipatch_item_next: (skip)
 * @iter: Patch item iterator containing #IpatchItem items
 *
 * Gets the next item in a patch item iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next item in @iter or %NULL if no more items.
 */
IpatchItem *
ipatch_item_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_ITEM (obj));
  else return (NULL);
}
