/*
 * IpatchStateItem.h - Base class for state (undo/redo) items
 *
 * libInstPatch
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
#ifndef __IPATCH_STATE_ITEM_H__
#define __IPATCH_STATE_ITEM_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _IpatchStateItem IpatchStateItem;
typedef struct _IpatchStateItemClass IpatchStateItemClass;

#include <libipatch/IpatchLock.h>
#include <libipatch/IpatchStateGroup.h>

#define IPATCH_TYPE_STATE_ITEM   (ipatch_state_item_get_type ())
#define IPATCH_STATE_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_STATE_ITEM, IpatchStateItem))
#define IPATCH_STATE_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_STATE_ITEM, \
   IpatchStateItemClass))
#define IPATCH_IS_STATE_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_STATE_ITEM))
#define IPATCH_IS_STATE_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_STATE_ITEM))
#define IPATCH_STATE_ITEM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_STATE_ITEM, \
   IpatchStateItemClass))

/* state item */
struct _IpatchStateItem
{
  GObject parent_instance;	/* derived from GObject */

  guint flags;			/* flags for state items */
  GNode *node;			/* node in item tree or NULL */
  IpatchStateGroup *group;	/* group this item belongs to */
};

typedef enum
{
  IPATCH_STATE_ITEM_UNDO = 0,	/* state item holds undo data */
  IPATCH_STATE_ITEM_REDO = 1	/* state item holds redo data */
} IpatchStateItemType;

typedef enum
{
  IPATCH_STATE_ITEM_TYPE_MASK = 1 << 0, /* mask for IpatchStateItemType */
  IPATCH_STATE_ITEM_ACTIVE    = 1 << 1, /* a flag for item's in item tree */
  IPATCH_STATE_ITEM_DEPENDENT = 1 << 2 /* internal - to mark dependent items */
} IpatchStateItemFlags;

/* reserve 5 bits for future use */
#define IPATCH_STATE_ITEM_UNUSED_FLAG_SHIFT  8

/* state item class */
struct _IpatchStateItemClass
{
  GObjectClass parent_class;	/* derived from GObject */
  char *descr;			/* text description of this state class */

  /* methods */

  void (*restore)(const IpatchStateItem *item); /* restore state */
  gboolean (*depend)(const IpatchStateItem *item1, /* check dependency */
		     const IpatchStateItem *item2);
  gboolean (*conflict)(const IpatchStateItem *item1, /* check for conflicts */
		       const IpatchStateItem *item2);
  char * (*describe)(const IpatchStateItem *item); /* detailed description */
};

GType ipatch_state_item_get_type (void);

void ipatch_state_item_restore (const IpatchStateItem *item);
gboolean ipatch_state_item_depend (const IpatchStateItem *item1,
				  const IpatchStateItem *item2);
gboolean ipatch_state_item_conflict (const IpatchStateItem *item1,
				    const IpatchStateItem *item2);
char *ipatch_state_item_describe (const IpatchStateItem *item);

#endif
