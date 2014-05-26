/*
 * IpatchState_types.h - Builtin IpatchState (undo/redo history) objects
 *
 * Ipatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
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
#ifndef __IPATCH_STATE_TYPES_H__
#define __IPATCH_STATE_TYPES_H__

#include <libinstpatch/IpatchItem.h>

typedef struct _IpatchStateItemAdd IpatchStateItemAdd;
typedef struct _IpatchStateItemAddClass IpatchStateItemAddClass;
typedef struct _IpatchStateItemRemove IpatchStateItemRemove;
typedef struct _IpatchStateItemRemoveClass IpatchStateItemRemoveClass;
typedef struct _IpatchStateItemChange IpatchStateItemChange;
typedef struct _IpatchStateItemChangeClass IpatchStateItemChangeClass;

#include <libipatch/IpatchState.h>

#define IPATCH_TYPE_STATE_ITEM_ADD   (ipatch_state_item_add_get_type ())
#define IPATCH_STATE_ITEM_ADD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_STATE_ITEM_ADD, \
   IpatchStateItemAdd))
#define IPATCH_STATE_ITEM_ADD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_STATE_ITEM_ADD, \
   IpatchStateItemAddClass))
#define IPATCH_IS_STATE_ITEM_ADD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_STATE_ITEM_ADD))
#define IPATCH_IS_STATE_ITEM_ADD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_STATE_ITEM_ADD))
#define IPATCH_STATE_ITEM_ADD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_STATE_ITEM_ADD, \
   IpatchStateItemAddClass))

#define IPATCH_TYPE_STATE_ITEM_REMOVE   (ipatch_state_item_remove_get_type ())
#define IPATCH_STATE_ITEM_REMOVE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_STATE_ITEM_REMOVE, \
   IpatchStateItemRemove))
#define IPATCH_STATE_ITEM_REMOVE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_STATE_ITEM_REMOVE, \
   IpatchStateItemRemoveClass))
#define IPATCH_IS_STATE_ITEM_REMOVE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_STATE_ITEM_REMOVE))
#define IPATCH_IS_STATE_ITEM_REMOVE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_STATE_ITEM_REMOVE))
#define IPATCH_STATE_ITEM_REMOVE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_STATE_ITEM_REMOVE, \
   IpatchStateItemRemoveClass))

#define IPATCH_TYPE_STATE_ITEM_CHANGE   (ipatch_state_item_change_get_type ())
#define IPATCH_STATE_ITEM_CHANGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_STATE_ITEM_CHANGE, \
   IpatchStateItemChange))
#define IPATCH_STATE_ITEM_CHANGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_STATE_ITEM_CHANGE, \
   IpatchStateItemChangeClass))
#define IPATCH_IS_STATE_ITEM_CHANGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_STATE_ITEM_CHANGE))
#define IPATCH_IS_STATE_ITEM_CHANGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_STATE_ITEM_CHANGE))
#define IPATCH_STATE_ITEM_CHANGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_STATE_ITEM_CHANGE, \
   IpatchStateItemChangeClass))


/* Add IpatchItem state object */
struct _IpatchStateItemAdd
{
  IpatchStateItem parent_instance; /* derived from IpatchStateItem */
  IpatchItem *item;		/* item that was added */
};

/* Add IpatchItem state class */
struct _IpatchStateItemAddClass
{
  IpatchStateItemClass parent_class; /* derived from IpatchStateItem */
};


/* Remove IpatchItem state object */
struct _IpatchStateItemRemove
{
  IpatchStateItem parent_instance; /* derived from IpatchStateItem */
  IpatchItem *item;		/* item that was removed */
  IpatchItem *parent;		/* parent of item that was removed */
};

/* Remove IpatchItem state class */
struct _IpatchStateItemRemoveClass
{
  IpatchStateItemClass parent_class; /* derived from IpatchStateItem */
};


/* IpatchItem property change state object */
struct _IpatchStateItemChange
{
  IpatchStateItem parent_instance; /* derived from IpatchStateItem */
  IpatchItem *item;		/* item that has changed */
  GParamSpec *pspec;		/* parameter spec of changed property */
  GValue value;			/* old property value */
};

/* IpatchItem property change state class */
struct _IpatchStateItemChangeClass
{
  IpatchStateItemClass parent_class; /* derived from IpatchStateItem */
};


GType ipatch_state_item_add_get_type (void);
GType ipatch_state_item_remove_get_type (void);
GType ipatch_state_item_change_get_type (void);

#endif
