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
#ifndef __IPATCH_ITEM_H__
#define __IPATCH_ITEM_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

typedef struct _IpatchItem IpatchItem;
typedef struct _IpatchItemClass IpatchItemClass;

#include <libinstpatch/IpatchIter.h>
#include <libinstpatch/IpatchList.h>

#define IPATCH_TYPE_ITEM   (ipatch_item_get_type ())
#define IPATCH_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_ITEM, IpatchItem))
#define IPATCH_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_ITEM, IpatchItemClass))
#define IPATCH_IS_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_ITEM))
#define IPATCH_IS_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_ITEM))
#define IPATCH_ITEM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_ITEM, IpatchItemClass))

/**
 * IpatchItemCopyLinkFunc:
 * @item: Item which is being linked (contains a reference to @link).
 * @link: The item being referenced (can be NULL).
 * @user_data: User data supplied to copy/duplicate function.
 *
 * A callback function called during item copy/duplicate operations for
 * any item link reference which needs to be resolved.  An example usage
 * is for deep duplicating an object (all references can also be duplicated).
 * Item copy methods should call this for any linked item references which are
 * not part of the new object.
 *
 * Returns: Pointer to item to use for link property (can be the @link item
 * if the duplicated/copied item is local to the same file).
 */
typedef IpatchItem * (*IpatchItemCopyLinkFunc)(IpatchItem *item,
					       IpatchItem *link,
					       gpointer user_data);

/* Base patch item object */
struct _IpatchItem
{
  GObject object;

  /*< private >*/

  int flags;		/* flag field (atomic int ops used) */
  IpatchItem *parent;	/* parent item or NULL if not parented or root */
  IpatchItem *base; 	/* base parent object or NULL */
  GStaticRecMutex *mutex;  /* pointer to mutex (could be a parent's mutex) */
};

/* Base patch item class */
struct _IpatchItemClass
{
  GObjectClass parent_class;

  /*< public >*/

  gboolean mutex_slave;	/* set to TRUE to use parent thread mutex */

  /* methods */
  void (*item_set_property)(GObject *object, guint property_id,
			    const GValue *value, GParamSpec *pspec);
  void (*copy)(IpatchItem *dest, IpatchItem *src,
	       IpatchItemCopyLinkFunc link_func, gpointer user_data);
  void (*remove)(IpatchItem *item);

/**
 * remove_full:
 * @item: Item to remove references to/from
 * @full: %TRUE removes all references to and from @item, %FALSE removes only references to @item
 *
 * Removes references to/from (depending on the value of @full) @item of other objects in the same
 * #IpatchBase object.  The remove method can be used instead for default behavior of removing item's
 * children if it is a container and @full is TRUE.
 *
 * Since: 1.1.0
 */
  void (*remove_full)(IpatchItem *item, gboolean full);
};

typedef enum
{
  IPATCH_ITEM_HOOKS_ACTIVE = 1 << 0,  /* hook callbacks active? */
  IPATCH_ITEM_FREE_MUTEX   = 1 << 1   /* TRUE if item should free its mutex */
} IpatchItemFlags;

/**
 * IPATCH_ITEM_UNUSED_FLAG_SHIFT: (skip)
 */
/* 2 flags + reserve 2 bits for future expansion */
#define IPATCH_ITEM_UNUSED_FLAG_SHIFT 4

/* Multi-thread locking macros. For now there is no distinction between
   write and read locking since GStaticRWLock is not recursive. */
#define IPATCH_ITEM_WLOCK(item)	\
    g_static_rec_mutex_lock (((IpatchItem *)(item))->mutex)
#define IPATCH_ITEM_WUNLOCK(item) \
    g_static_rec_mutex_unlock (((IpatchItem *)(item))->mutex)
#define IPATCH_ITEM_RLOCK(item)		IPATCH_ITEM_WLOCK(item)
#define IPATCH_ITEM_RUNLOCK(item)	IPATCH_ITEM_WUNLOCK(item)

/**
 * IpatchItemPropNotify:
 * @item: Item whose property changed
 * @pspec: Parameter spec of property which changed
 * @new_value: New value assigned to the property
 * @old_value: Old value that the property had (can be %NULL for read only
 *   properties)
 * @user_data: User defined pointer from when callback was added
 * @event_ptrs: Per event data defined by users of callback system
 *
 * Property notify information structure.
 */
typedef struct
{
  IpatchItem *item;		/* item whose property changed */
  GParamSpec *pspec;		/* property spec of the property that changed */
  const GValue *new_value;	/* new value of the property */
  const GValue *old_value;	/* old value of the property (can be NULL!) */
  gpointer user_data;		/* user defined data, defined on connect */

  /* per event data */
  struct
    {
      gpointer data;		/* implementation defined data per event */
      GDestroyNotify destroy;	/* function called to cleanup for @data */
    } eventdata[4];
} IpatchItemPropNotify;

/**
 * IPATCH_ITEM_PROP_NOTIFY_SET_EVENT:
 * @info: IpatchItemPropNotify pointer
 * @index: Pointer index (0-3)
 * @data: Data to assign to pointer field
 * @destroy: Callback function to cleanup @data when done (or %NULL)
 *
 * A macro for assigning per event pointers to #IpatchItemPropNotify.
 */
#define IPATCH_ITEM_PROP_NOTIFY_SET_EVENT(info, index, data, destroy) \
  (info)->eventdata[index].data = data; (info)->eventdata[index].destroy = destroy

/**
 * IpatchItemPropCallback:
 *
 * IpatchItem property change callback function prototype.
 */
typedef void (*IpatchItemPropCallback)(IpatchItemPropNotify *notify);

/**
 * IpatchItemPropDisconnect:
 * @item: Item of prop change match criteria
 * @pspec: Parameter spec of prop change match criteria
 * @user_data: User defined pointer from when callback was added
 *
 * Function prototype which gets called when a property notify callback gets
 * disconnected.
 */
typedef void (*IpatchItemPropDisconnect)(IpatchItem *item, GParamSpec *pspec,
					 gpointer user_data);

/* Convenience macro used by IpatchItem copy methods to call a copy link
   function, or use the link pointer directly if function is NULL */
#define IPATCH_ITEM_COPY_LINK_FUNC(item, link, func, userdata) \
  (func ? func (item, link, userdata) : link)

/* stored publicy for added convenience to derived types */
extern GParamSpec *ipatch_item_pspec_title;

GType ipatch_item_get_type (void);
int ipatch_item_get_flags (gpointer item);
void ipatch_item_set_flags (gpointer item, int flags);
void ipatch_item_clear_flags (gpointer item, int flags);
void ipatch_item_set_parent (IpatchItem *item, IpatchItem *parent);
void ipatch_item_unparent (IpatchItem *item);
IpatchItem *ipatch_item_get_parent (IpatchItem *item);
IpatchItem *ipatch_item_peek_parent (IpatchItem *item);
IpatchItem *ipatch_item_get_base (IpatchItem *item);
IpatchItem *ipatch_item_peek_base (IpatchItem *item);
IpatchItem *ipatch_item_get_ancestor_by_type (IpatchItem *item,
					      GType ancestor_type);
IpatchItem *ipatch_item_peek_ancestor_by_type (IpatchItem *item,
					       GType ancestor_type);
void ipatch_item_remove (IpatchItem *item);
void ipatch_item_remove_full (IpatchItem *item, gboolean full);
void ipatch_item_remove_recursive (IpatchItem *item, gboolean full);
void ipatch_item_changed (IpatchItem *item);
void ipatch_item_get_property_fast (IpatchItem *item, GParamSpec *pspec,
				    GValue *value);
void ipatch_item_copy (IpatchItem *dest, IpatchItem *src);
void ipatch_item_copy_link_func (IpatchItem *dest, IpatchItem *src,
				 IpatchItemCopyLinkFunc link_func,
				 gpointer user_data);
void ipatch_item_copy_replace (IpatchItem *dest, IpatchItem *src,
			       GHashTable *repl_hash);
IpatchItem *ipatch_item_duplicate (IpatchItem *item);
IpatchItem *ipatch_item_duplicate_link_func (IpatchItem *item,
					     IpatchItemCopyLinkFunc link_func,
					     gpointer user_data);
IpatchItem *ipatch_item_duplicate_replace (IpatchItem *item,
					   GHashTable *repl_hash);
IpatchList *ipatch_item_duplicate_deep (IpatchItem *item);
IpatchItem *ipatch_item_copy_link_func_deep (IpatchItem *item, IpatchItem *link,
					     gpointer user_data);
IpatchItem *ipatch_item_copy_link_func_hash (IpatchItem *item, IpatchItem *link,
					     gpointer user_data);
gboolean ipatch_item_type_can_conflict (GType item_type);
GParamSpec **ipatch_item_type_get_unique_specs (GType item_type,
						guint32 *groups);
GValueArray *ipatch_item_get_unique_props (IpatchItem *item);
guint ipatch_item_test_conflict (IpatchItem *item1, IpatchItem *item2);
void ipatch_item_set_atomic (gpointer item,
			     const char *first_property_name, ...);
void ipatch_item_get_atomic (gpointer item,
			     const char *first_property_name, ...);
IpatchItem *ipatch_item_first (IpatchIter *iter);
IpatchItem *ipatch_item_next (IpatchIter *iter);


/* in IpatchItemProp.c */


void ipatch_item_prop_notify (IpatchItem *item, GParamSpec *pspec,
			      const GValue *new_value, const GValue *old_value);
void ipatch_item_prop_notify_by_name (IpatchItem *item, const char *prop_name,
				      const GValue *new_value,
				      const GValue *old_value);
guint ipatch_item_prop_connect (IpatchItem *item, GParamSpec *pspec,
				IpatchItemPropCallback callback,
				IpatchItemPropDisconnect disconnect,
				gpointer user_data);
guint ipatch_item_prop_connect_by_name (IpatchItem *item,
					const char *prop_name,
					IpatchItemPropCallback callback,
					IpatchItemPropDisconnect disconnect,
					gpointer user_data);
void ipatch_item_prop_disconnect (guint handler_id);
void ipatch_item_prop_disconnect_matched (IpatchItem *item,
					  GParamSpec *pspec,
					  IpatchItemPropCallback callback,
					  gpointer user_data);
void ipatch_item_prop_disconnect_by_name (IpatchItem *item,
					  const char *prop_name,
					  IpatchItemPropCallback callback,
					  gpointer user_data);
#endif
