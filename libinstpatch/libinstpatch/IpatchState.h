/*
 * IpatchState.h - Header for state history system (undo/redo)
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
#ifndef __IPATCH_STATE_H__
#define __IPATCH_STATE_H__

#include <glib.h>
#include <glib-object.h>

#include <libipatch/IpatchLock.h>
#include <libipatch/IpatchStateItem.h>
#include <libipatch/IpatchStateGroup.h>

typedef struct _IpatchState IpatchState;
typedef struct _IpatchStateClass IpatchStateClass;

#define IPATCH_TYPE_STATE   (ipatch_state_get_type ())
#define IPATCH_STATE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_STATE, IpatchState))
#define IPATCH_STATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_STATE, IpatchStateClass))
#define IPATCH_IS_STATE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_STATE))
#define IPATCH_IS_STATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_STATE))
#define IPATCH_STATE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_STATE, IpatchStateClass))

/* Ipatch State history object */
struct _IpatchState
{
  IpatchLock parent_instance;	/* derived from IpatchLock */

  GNode *root;			/* state tree (IpatchState) */
  GNode *position;		/* current position in state tree */

  GNode *group_root;		/* group tree (IpatchStateGroup) */

  GPrivate *active_group_key; /* per thread active group (IpatchStateGroup)*/

  IpatchStateItem *current_undo;	/* current undo item or NULL */
  GNode *redo_parent; /* current redo state parent (IpatchState GNode) */
};

/* Ipatch State history class */
struct _IpatchStateClass
{
  IpatchLockClass parent_class;
};

GType ipatch_state_get_type (void);
IpatchState *ipatch_state_new (void);
void ipatch_state_begin_group (IpatchState *state, const char *descr);
void ipatch_state_end_group (IpatchState *state);
void ipatch_state_set_active_group (IpatchState *state, IpatchStateGroup *group);
IpatchStateGroup *ipatch_state_get_active_group (IpatchState *state);
void ipatch_state_record_item (IpatchState *state, IpatchStateItem *item);
void ipatch_state_record (IpatchState *state, const char *type_name,
			  const char *first_property_name, ...);
void ipatch_state_retract (IpatchState *state);
/* GList *ipatch_state_get_undo_depends (IpatchState *state, const GList *items); */
void ipatch_state_undo (IpatchState *state, GList *items);
void ipatch_state_redo (IpatchState *state, GList *items);

#endif
