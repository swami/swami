/*
 * IpatchStateGroup.h - State (undo/redo) group object
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
#ifndef __IPATCH_STATE_GROUP_H__
#define __IPATCH_STATE_GROUP_H__

#include <glib.h>
#include <glib-object.h>

#include <libipatch/IpatchLock.h>

typedef struct _IpatchStateGroup IpatchStateGroup;
typedef struct _IpatchStateGroupClass IpatchStateGroupClass;

#define IPATCH_TYPE_STATE_GROUP   (ipatch_state_group_get_type ())
#define IPATCH_STATE_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_STATE_GROUP, IpatchStateGroup))
#define IPATCH_STATE_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_STATE_GROUP, \
   IpatchStateGroupClass))
#define IPATCH_IS_STATE_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_STATE_GROUP))
#define IPATCH_IS_STATE_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_STATE_GROUP))
#define IPATCH_STATE_GROUP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_STATE_GROUP, \
   IpatchStateGroupClass))

/* state group structure */
struct _IpatchStateGroup
{
  IpatchLock parent_instance;	/* derived from IpatchLock */

  guint flags;			/* group flags */
  GNode *node;			/* node in the group tree or NULL */
  char *descr;		     /* description of this action, or NULL */
  GList *items;		/* list of IpatchStateItem objects (prepend) */
};

typedef enum
{
  IPATCH_STATE_GROUP_RETRACTED = 1 << 0,	/* group has been retracted */
  IPATCH_STATE_GROUP_PARTIAL   = 1 << 1 /* some item's missing from group */
} IpatchStateGroupFlags;

/* state group class */
struct _IpatchStateGroupClass
{
  IpatchLockClass parent_class;
};

GType ipatch_state_group_get_type (void);

#endif
