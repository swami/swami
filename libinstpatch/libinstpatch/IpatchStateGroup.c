/*
 * IpatchStateGroup.c - State (undo/redo) group object
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
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchStateGroup.h"
#include "ipatch_priv.h"

static void ipatch_state_group_class_init (IpatchStateGroupClass *klass);
static void ipatch_state_group_finalize (GObject *gobject);

gpointer group_parent_class = NULL;

GType
ipatch_state_group_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchStateGroupClass), NULL, NULL,
      (GClassInitFunc) ipatch_state_group_class_init, NULL, NULL,
      sizeof (IpatchStateGroup), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (IPATCH_TYPE_LOCK, "IpatchStateGroup",
					&item_info, 0);
  }

  return (item_type);
}

static void
ipatch_state_group_class_init (IpatchStateGroupClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  group_parent_class = g_type_class_peek_parent (klass);
  gobj_class->finalize = ipatch_state_group_finalize;
}

static void
ipatch_state_group_finalize (GObject *gobject)
{
  IpatchStateGroup *group = (IpatchStateGroup *)gobject;

  if (group->node)
    g_node_destroy (group->node); /* destroy group's tree node */

  if (G_OBJECT_CLASS (group_parent_class)->finalize)
    G_OBJECT_CLASS (group_parent_class)->finalize (gobject);
}
