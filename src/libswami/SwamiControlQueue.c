/*
 * SwamiControlQueue.c - Swami control event queue
 * For queuing SwamiControl events
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
#include "SwamiControlQueue.h"
#include "swami_priv.h"

/* queue item bag */
typedef struct
{
  SwamiControl *control;
  SwamiControlEvent *event;
} QueueItem;

GType
swami_control_queue_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info =
      {
	sizeof (SwamiControlQueueClass), NULL, NULL,
	(GClassInitFunc) NULL, NULL, NULL,
	sizeof (SwamiControlQueue), 0,
	(GInstanceInitFunc) NULL
      };

    obj_type = g_type_register_static (SWAMI_TYPE_LOCK, "SwamiControlQueue",
				       &obj_info, 0);
  }

  return (obj_type);
}

/**
 * swami_control_queue_new:
 *
 * Create a new control queue object. These are used to queue control events
 * which can then be run at a later time (within a GUI thread for example).
 *
 * Returns: New control queue with a ref count of 1 that the caller owns.
 */
SwamiControlQueue *
swami_control_queue_new (void)
{
  return (SWAMI_CONTROL_QUEUE (g_object_new (SWAMI_TYPE_CONTROL_QUEUE, NULL)));
}

/**
 * swami_control_queue_add_event:
 * @queue: Swami control queue object
 * @control: Control to queue an event for
 * @event: Control event to queue
 *
 * Adds a control event to a queue. Does not run queue test function this is
 * the responsibility of the caller (for added performance).
 */
void
swami_control_queue_add_event (SwamiControlQueue *queue, SwamiControl *control,
			       SwamiControlEvent *event)
{
  QueueItem *item;

  g_return_if_fail (SWAMI_IS_CONTROL_QUEUE (queue));
  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (event != NULL);

  item = g_slice_new (QueueItem);
  item->control = g_object_ref (control); /* ++ ref control */
  item->event = swami_control_event_ref (event); /* ++ ref event */

  /* ++ increment active reference, gets removed in swami_control_queue_run */
  swami_control_event_active_ref (event);

  SWAMI_LOCK_WRITE (queue);
  queue->list = g_list_prepend (queue->list, item);
  if (!queue->tail) queue->tail = queue->list;
  SWAMI_UNLOCK_WRITE (queue);
}

/**
 * swami_control_queue_run:
 * @queue: Swami control queue object
 *
 * Process a control event queue by sending queued events to controls.
 */
void
swami_control_queue_run (SwamiControlQueue *queue)
{
  GList *list, *p, *temp;
  QueueItem *item;

  g_return_if_fail (SWAMI_IS_CONTROL_QUEUE (queue));

  SWAMI_LOCK_WRITE (queue);
  list = queue->tail;		/* take over the list */
  queue->list = NULL;
  queue->tail = NULL;
  SWAMI_UNLOCK_WRITE (queue);

  /* process queue in reverse (since we prepended) */
  p = list;
  while (p)
    {
      item = (QueueItem *)(p->data);
      swami_control_set_event_no_queue_loop (item->control, item->event);
      g_object_unref (item->control); /* -- unref control */
      swami_control_event_active_unref (item->event); /* -- unref active ref */
      swami_control_event_unref (item->event); /* -- unref event */

      g_slice_free (QueueItem, item);

      temp = p;
      p = p->prev;
      temp = g_list_delete_link (temp, temp);  /* assign to prevent gcc warning */
    }
}

/**
 * swami_control_queue_set_test_func:
 * @queue: Control queue object
 * @test_func: Test function callback (function should return %TRUE to queue
 *   an event or %FALSE to send immediately), can be %NULL in which case all
 *   events are queued (the default).
 * 
 * Set the queue test function which is called for each event added (and should
 * therefore be fast) to determine if the event should be queued or sent
 * immediately. Note that swami_control_queue_add_event() doesn't run the
 * test function, that is up to the caller (for increased performance).
 */
void
swami_control_queue_set_test_func (SwamiControlQueue *queue,
				   SwamiControlQueueTestFunc test_func)
{
  g_return_if_fail (SWAMI_IS_CONTROL_QUEUE (queue));
  queue->test_func = test_func;
}
