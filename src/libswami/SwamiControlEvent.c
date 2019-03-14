/*
 * SwamiControlEvent.c - Swami event structure
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
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
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>

#ifdef WIN32
#include <time.h>
#include <windows.h>
#endif

#include "SwamiControlEvent.h"
#include "swami_priv.h"


GType
swami_control_event_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("SwamiControlEvent",
                                (GBoxedCopyFunc)swami_control_event_duplicate,
                                (GBoxedFreeFunc)swami_control_event_unref);
  return (type);
}

/**
 * swami_control_event_new:
 * @stamp: %TRUE to time stamp the new event (can be done later with
 *   swami_control_event_stamp().
 *
 * Create a new control event structure.
 *
 * Returns: New control event with a refcount of 1 which the caller owns.
 *   Keep in mind that a SwamiControlEvent is not a GObject, so it does its
 *   own refcounting with swami_control_event_ref() and
 *   swami_control_event_unref().
 */
SwamiControlEvent *
swami_control_event_new (gboolean stamp)
{
  SwamiControlEvent *event;

  event = g_slice_new0 (SwamiControlEvent);
  event->refcount = 1;

  if (stamp)
    {
#ifndef WIN32
      gettimeofday (&event->tick, NULL);
#else
      static DWORD tick_start ;
      tick_start = GetTickCount();
      event->tick.tv_sec = tick_start / 1000;
      event->tick.tv_usec = ( tick_start % 1000 ) * 1000;
#endif
    }

  return (event);
}

/**
 * swami_control_event_free:
 * @event: Swami control event structure to free
 *
 * Frees a Swami control event structure. Normally this function should
 * not be used, swami_control_event_unref() should be used instead to allow
 * for multiple users of a SwamiControlEvent. Calling this function bypasses
 * reference counting, so make sure you know what you are doing if you use
 * this.
 */
void
swami_control_event_free (SwamiControlEvent *event)
{
  g_return_if_fail (event != NULL);

  if (event->origin) swami_control_event_unref (event->origin);
  g_value_unset (&event->value);
  g_slice_free (SwamiControlEvent, event);
}

/**
 * swami_control_event_duplicate:
 * @event: Swami control event structure
 *
 * Duplicate a control event. The refcount and active count are not duplicated,
 * but the tick, origin and value are.
 *
 * Returns: New duplicate control event
 */
SwamiControlEvent *
swami_control_event_duplicate (const SwamiControlEvent *event)
{
  SwamiControlEvent *dup;

  g_return_val_if_fail (event != NULL, NULL);

  dup = g_slice_new0 (SwamiControlEvent);
  dup->refcount = 1;
  dup->tick = event->tick;
  if (event->origin) dup->origin = swami_control_event_ref (event->origin);
  g_value_copy (&event->value, &dup->value);

  return (dup);
}

/**
 * swami_control_event_transform:
 * @event: Swami control event structure
 * @valtype: The type for the new value (or 0 to use the @event value type)
 * @trans: Value transform function
 * @data: User data passed to transform function
 *
 * Like swami_control_event_duplicate() but transforms the event's value
 * using the @trans function to the type indicated by @valtype.
 *
 * Returns: New transformed control event (caller owns creator's reference)
 */
SwamiControlEvent *
swami_control_event_transform (SwamiControlEvent *event, GType valtype,
			       SwamiValueTransform trans, gpointer data)
{
  SwamiControlEvent *dup;

  g_return_val_if_fail (event != NULL, NULL);
  g_return_val_if_fail (trans != NULL, NULL);

  dup = g_slice_new0 (SwamiControlEvent);
  dup->refcount = 1;
  dup->tick = event->tick;

  dup->origin = event->origin ? event->origin : event;
  swami_control_event_ref (dup->origin);

  g_value_init (&dup->value, valtype ? valtype : G_VALUE_TYPE (&event->value));
  trans (&event->value, &dup->value, data);

  return (dup);
}

/**
 * swami_control_event_stamp:
 * @event: Event to stamp
 *
 * Stamps an event with the current tick count.
 */
void
swami_control_event_stamp (SwamiControlEvent *event)
{
  g_return_if_fail (event != NULL);

#ifndef WIN32
  gettimeofday (&event->tick, NULL);
#else
  {
    static DWORD tick_start ;
    tick_start = GetTickCount();
    event->tick.tv_sec = tick_start / 1000;
    event->tick.tv_usec = ( tick_start % 1000 ) * 1000;
  }
#endif
}

/**
 * swami_control_event_set_origin:
 * @event: Event structure
 * @origin: Origin event or %NULL if @event is its own origin
 *
 * Set the origin of an event. Should only be set once since its
 * not multi-thread locked.
 */
void
swami_control_event_set_origin (SwamiControlEvent *event,
				SwamiControlEvent *origin)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->origin == NULL);

  if (origin) swami_control_event_ref (origin);
  event->origin = origin;
}

/**
 * swami_control_event_ref:
 * @event: Event structure
 *
 * Increment the reference count of an event.
 *
 * Returns: The same referenced @event as a convenience.
 */
SwamiControlEvent *
swami_control_event_ref (SwamiControlEvent *event)
{
  g_return_val_if_fail (event != NULL, NULL);
  event->refcount++;

  return (event);
}

/**
 * swami_control_event_unref:
 * @event: Event structure
 *
 * Decrement the reference count of an event. If the reference count
 * reaches 0 the event will be freed.
 */
void
swami_control_event_unref (SwamiControlEvent *event)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->refcount > 0);

  if (--event->refcount == 0)
    swami_control_event_free (event);
}

/**
 * swami_control_event_active_ref:
 * @event: Event structure
 *
 * Increment the active propagation reference count.
 */
void
swami_control_event_active_ref (SwamiControlEvent *event)
{
  g_return_if_fail (event != NULL);
  event->active++;
}

/**
 * swami_control_event_active_unref:
 * @event: Event object
 *
 * Decrement the active propagation reference count.
 */
void
swami_control_event_active_unref (SwamiControlEvent *event)
{
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->active > 0);

  event->active--;
}
