/*
 * SwamiEvent_ipatch.c - libInstPatch SwamiControl event types
 *
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "SwamiEvent_ipatch.h"
#include "swami_priv.h"


GType
swami_event_item_add_get_type (void)
{
  static GType item_type = 0;
 
  if (!item_type)
    {
      item_type = g_boxed_type_register_static
        ("SwamiEventItemAdd", (GBoxedCopyFunc) swami_event_item_add_copy,
         (GBoxedFreeFunc) swami_event_item_add_free);
    }
  return (item_type);
}

GType
swami_event_item_remove_get_type (void)
{
  static GType item_type = 0;
 
  if (!item_type)
    {
      item_type = g_boxed_type_register_static
        ("SwamiEventItemRemove", (GBoxedCopyFunc) swami_event_item_remove_copy,
         (GBoxedFreeFunc) swami_event_item_remove_free);
    }
  return (item_type);
}

GType
swami_event_prop_change_get_type (void)
{
  static GType item_type = 0;
 
  if (!item_type)
    {
      item_type = g_boxed_type_register_static
        ("SwamiEventPropChange", (GBoxedCopyFunc) swami_event_prop_change_copy,
         (GBoxedFreeFunc) swami_event_prop_change_free);
    }
  return (item_type);
}

/**
 * swami_event_item_add_copy:
 * @item_add: Patch item add event to copy
 * 
 * Copies a patch item add event (an IpatchItem pointer really).
 * 
 * Returns: New duplicated patch item add event.
 */
SwamiEventItemAdd *
swami_event_item_add_copy (SwamiEventItemAdd *item_add)
{
  return (g_object_ref (item_add));
}

/**
 * swami_event_item_add_free:
 * @item_add: Patch item add event to free
 * 
 * Free a patch item add event (an IpatchItem pointer really).
 */
void
swami_event_item_add_free (SwamiEventItemAdd *item_add)
{
  g_object_unref (item_add);
}

/**
 * swami_event_item_remove_new:
 * 
 * Allocate a new patch item remove event structure.
 * 
 * Returns: Newly allocated patch item remove event structure.
 */
SwamiEventItemRemove *
swami_event_item_remove_new (void)
{
  return (g_slice_new0 (SwamiEventItemRemove));
}

/**
 * swami_event_item_remove_copy:
 * @item_remove: Patch item remove event to copy
 * 
 * Copies a patch item remove event structure.
 * 
 * Returns: New duplicated patch item remove event structure.
 */
SwamiEventItemRemove *
swami_event_item_remove_copy (SwamiEventItemRemove *item_remove)
{
  SwamiEventItemRemove *new_event;

  new_event = g_slice_new (SwamiEventItemRemove);
  new_event->item = g_object_ref (item_remove->item);
  new_event->parent = g_object_ref (item_remove->parent);
  return (new_event);
}

/**
 * swami_event_item_remove_free:
 * @item_remove: Patch item remove event to free
 * 
 * Free a patch item remove event structure.
 */
void
swami_event_item_remove_free (SwamiEventItemRemove *item_remove)
{
  g_object_unref (item_remove->item);
  g_object_unref (item_remove->parent);
  g_slice_free (SwamiEventItemRemove, item_remove);
}

/**
 * swami_event_prop_change_new:
 * 
 * Allocate a new patch property change event structure.
 * 
 * Returns: Newly allocated patch property change event structure.
 */
SwamiEventPropChange *
swami_event_prop_change_new (void)
{
  return (g_slice_new0 (SwamiEventPropChange));
}

/**
 * swami_event_prop_change_copy:
 * @prop_change: Patch property change event to copy
 * 
 * Copies a patch property change event structure.
 * 
 * Returns: New duplicated patch property change event structure.
 */
SwamiEventPropChange *
swami_event_prop_change_copy (SwamiEventPropChange *prop_change)
{
  SwamiEventPropChange *new_event;

  new_event = g_slice_new (SwamiEventPropChange);
  new_event->object = g_object_ref (prop_change->object);
  new_event->pspec = g_param_spec_ref (prop_change->pspec);

  if (G_IS_VALUE (&prop_change->value))
    g_value_copy (&prop_change->value, &new_event->value);
  else memset (&prop_change->value, 0, sizeof (GValue));

  return (new_event);
}

/**
 * swami_event_prop_change_free:
 * @prop_change: Patch property change event to free
 * 
 * Free a patch property change event structure.
 */
void
swami_event_prop_change_free (SwamiEventPropChange *prop_change)
{
  g_object_unref (prop_change->object);
  g_param_spec_unref (prop_change->pspec);

  if (G_IS_VALUE (&prop_change->value))
    g_value_unset (&prop_change->value);

  g_slice_free (SwamiEventPropChange, prop_change);
}
