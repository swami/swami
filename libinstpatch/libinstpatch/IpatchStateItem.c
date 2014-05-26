/*
 * IpatchStateItem.c - Base class for state (undo/redo) items
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

#include "IpatchStateItem.h"

static void ipatch_state_item_class_init (IpatchStateItemClass *klass);
static void ipatch_state_item_finalize (GObject *gobject);

gpointer item_parent_class = NULL;

GType
ipatch_state_item_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchStateItemClass), NULL, NULL,
      (GClassInitFunc) ipatch_state_item_class_init, NULL, NULL,
      sizeof (IpatchStateItem), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (G_TYPE_OBJECT, "IpatchStateItem",
					&item_info, G_TYPE_FLAG_ABSTRACT);
  }

  return (item_type);
}

static void
ipatch_state_item_class_init (IpatchStateItemClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  item_parent_class = g_type_class_peek_parent (klass);
  gobj_class->finalize = ipatch_state_item_finalize;
}

static void
ipatch_state_item_finalize (GObject *gobject)
{
  IpatchStateItem *item = IPATCH_STATE_ITEM (gobject);

  if (item->node) g_node_destroy (item->node); /* destroy item's tree node */
  if (item->group) g_object_unref (item->group); /* -- unref item's group */

  if (G_OBJECT_CLASS (item_parent_class)->finalize)
    G_OBJECT_CLASS (item_parent_class)->finalize (gobject);
}

/**
 * ipatch_state_item_restore:
 * @item: State item to restore
 *
 * Restore the state saved by @item.
 */
void
ipatch_state_item_restore (const IpatchStateItem *item)
{
  IpatchStateItemClass *klass;
  g_return_if_fail (IPATCH_IS_STATE_ITEM (item));

  klass = IPATCH_STATE_ITEM_GET_CLASS (item);
  g_return_if_fail (klass->restore != NULL);
  (*klass->restore)(item);
}

/**
 * ipatch_state_item_depend:
 * @item1: First item
 * @item2: Second item
 *
 * Check if @item1 is dependent on @item2.
 *
 * Returns: %TRUE if @item1 is dependent on @item2, %FALSE otherwise.
 */
gboolean
ipatch_state_item_depend (const IpatchStateItem *item1,
			 const IpatchStateItem *item2)
{
  IpatchStateItemClass *klass;
  g_return_val_if_fail (IPATCH_IS_STATE_ITEM (item1), FALSE);
  g_return_val_if_fail (IPATCH_IS_STATE_ITEM (item2), FALSE);

  klass = IPATCH_STATE_ITEM_GET_CLASS (item1);
  return (!klass->depend || (*klass->depend)(item1, item2));
}

/**
 * ipatch_state_item_conflict:
 * @item1: First item
 * @item2: Second item
 *
 * Check if @item1 conflicts with @item2.
 *
 * Returns: %TRUE if @item1 conflicts with @item2, %FALSE otherwise.
 */
gboolean
ipatch_state_item_conflict (const IpatchStateItem *item1,
			   const IpatchStateItem *item2)
{
  IpatchStateItemClass *klass;
  g_return_val_if_fail (IPATCH_IS_STATE_ITEM (item1), FALSE);
  g_return_val_if_fail (IPATCH_IS_STATE_ITEM (item2), FALSE);

  klass = IPATCH_STATE_ITEM_GET_CLASS (item1);
  return (!klass->conflict || (*klass->conflict)(item1, item2));
}

/**
 * ipatch_state_item_describe:
 * @item: Item to describe
 *
 * Get a detailed description of the action that created the @item state.
 *
 * Returns: Newly allocated description of the @item action or %NULL if
 * no description available. String should be freed when no longer needed.
 */
char *
ipatch_state_item_describe (const IpatchStateItem *item)
{
  IpatchStateItemClass *klass;
  g_return_val_if_fail (IPATCH_IS_STATE_ITEM (item), NULL);

  klass = IPATCH_STATE_ITEM_GET_CLASS (item);
  if (klass->describe) return ((*klass->describe)(item));
  else return (NULL);
}
