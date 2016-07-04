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
#ifndef __IPATCH_CONTAINER_H__
#define __IPATCH_CONTAINER_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

/* forward type declarations */

typedef struct _IpatchContainer IpatchContainer;
typedef struct _IpatchContainerClass IpatchContainerClass;

#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchIter.h>
#include <libinstpatch/IpatchList.h>

#define IPATCH_TYPE_CONTAINER   (ipatch_container_get_type ())
#define IPATCH_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_CONTAINER, IpatchContainer))
#define IPATCH_CONTAINER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_CONTAINER, \
  IpatchContainerClass))
#define IPATCH_IS_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_CONTAINER))
#define IPATCH_IS_CONTAINER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_CONTAINER))
#define IPATCH_CONTAINER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_CONTAINER, \
  IpatchContainerClass))

/**
 * IpatchContainerCallback:
 * @container: Container item
 * @item: Item that was added/removed to/from @container
 * @user_data: User defined pointer assigned when callback connected
 *
 * A function prototype callback which is called for container adds or removes
 * (after adds, before removes).
 */
typedef void (*IpatchContainerCallback)(IpatchContainer *container,
					IpatchItem *item,
					gpointer user_data);
/**
 * IpatchContainerDisconnect:
 * @container: Container item
 * @child: Match child item of original connect
 *   (ipatch_container_remove_connect() only, always NULL for
 *    ipatch_container_add_connect()).
 * @user_data: User defined pointer assigned when callback connected
 *
 * A function prototype which is called when a callback gets disconnected.
 */
typedef void (*IpatchContainerDisconnect)(IpatchContainer *container,
					  IpatchItem *child,
					  gpointer user_data);

/* Base patch container object */
struct _IpatchContainer
{
  IpatchItem parent_instance;
};

struct _IpatchContainerClass
{
  IpatchItemClass parent_class;

  /*< public >*/

  /* methods */
  const GType * (*child_types)(void);
  const GType * (*virtual_types)(void);
  gboolean (*init_iter)(IpatchContainer *container, IpatchIter *iter,
			GType type);
  void (*make_unique)(IpatchContainer *container, IpatchItem *item);
  gboolean (*get_dups)(IpatchContainer *container, IpatchItem *item,
		       IpatchList **list);
};

/* container uses no item flags */
/**
 * IPATCH_CONTAINER_UNUSED_FLAG_SHIFT: (skip)
 */
#define IPATCH_CONTAINER_UNUSED_FLAG_SHIFT  IPATCH_ITEM_UNUSED_FLAG_SHIFT

/**
 * IPATCH_CONTAINER_ERRMSG_INVALID_CHILD_2: (skip)
 */
#define IPATCH_CONTAINER_ERRMSG_INVALID_CHILD_2 \
    "Invalid child type '%s' for parent type '%s'"

GType ipatch_container_get_type (void);

IpatchList *ipatch_container_get_children (IpatchContainer *container,
					   GType type);
GList *ipatch_container_get_children_list (IpatchContainer *container);

GList *ipatch_container_get_children_by_type (IpatchContainer *container,
                                              GType type);
const GType *ipatch_container_get_child_types (IpatchContainer *container);
const GType *ipatch_container_get_virtual_types (IpatchContainer *container);
const GType *ipatch_container_type_get_child_types (GType container_type);
void ipatch_container_insert (IpatchContainer *container,
			      IpatchItem *item, int pos);
void ipatch_container_append (IpatchContainer *container, IpatchItem *item);
void ipatch_container_add (IpatchContainer *container, IpatchItem *item);
void ipatch_container_prepend (IpatchContainer *container, IpatchItem *item);
void ipatch_container_remove (IpatchContainer *container, IpatchItem *item);
void ipatch_container_remove_all (IpatchContainer *container);
guint ipatch_container_count (IpatchContainer *container, GType type);
void ipatch_container_make_unique (IpatchContainer *container,
				   IpatchItem *item);
void ipatch_container_add_unique (IpatchContainer *container,
				  IpatchItem *item);
gboolean ipatch_container_init_iter (IpatchContainer *container,
				     IpatchIter *iter, GType type);
void ipatch_container_insert_iter (IpatchContainer *container,
				   IpatchItem *item, IpatchIter *iter);
void ipatch_container_remove_iter (IpatchContainer *container,
				   IpatchIter *iter);

/* defined in IpatchContainer_notify.c */

void ipatch_container_add_notify (IpatchContainer *container, IpatchItem *child);
void ipatch_container_remove_notify (IpatchContainer *container, IpatchItem *child);
guint ipatch_container_add_connect (IpatchContainer *container,
				    IpatchContainerCallback callback,
				    IpatchContainerDisconnect disconnect,
				    gpointer user_data);
guint
ipatch_container_add_connect_notify (IpatchContainer *container,
			             IpatchContainerCallback callback,
			             GDestroyNotify notify_func,
			             gpointer user_data);
guint ipatch_container_remove_connect (IpatchContainer *container,
				       IpatchItem *child,
				       IpatchContainerCallback callback,
				       IpatchContainerDisconnect disconnect,
				       gpointer user_data);
guint
ipatch_container_remove_connect_notify (IpatchContainer *container,
                                        IpatchItem *child,
                                        IpatchContainerCallback callback,
                                        GDestroyNotify notify_func,
                                        gpointer user_data);
void ipatch_container_add_disconnect (guint handler_id);
void ipatch_container_add_disconnect_matched (IpatchContainer *container,
					      IpatchContainerCallback callback,
					      gpointer user_data);
void ipatch_container_remove_disconnect (guint handler_id);
void ipatch_container_remove_disconnect_matched (IpatchContainer *container,
						 IpatchItem *child,
						 IpatchContainerCallback callback,
						 gpointer user_data);
#endif
