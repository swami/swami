/*
 * SwamiControlQueue.h - Swami control event queue
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
#ifndef __SWAMI_CONTROL_QUEUE_H__
#define __SWAMI_CONTROL_QUEUE_H__

typedef struct _SwamiControlQueue SwamiControlQueue;
typedef struct _SwamiControlQueueClass SwamiControlQueueClass;

#include <glib.h>
#include <glib-object.h>
#include <libswami/SwamiLock.h>
#include <libswami/SwamiControl.h>
#include <libswami/SwamiControlEvent.h>

#define SWAMI_TYPE_CONTROL_QUEUE  (swami_control_queue_get_type ())
#define SWAMI_CONTROL_QUEUE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_CONTROL_QUEUE, \
   SwamiControlQueue))
#define SWAMI_CONTROL_QUEUE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_CONTROL_QUEUE, \
   SwamiControlQueueClass))
#define SWAMI_IS_CONTROL_QUEUE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_CONTROL_QUEUE))
#define SWAMI_IS_CONTROL_QUEUE_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE ((obj), SWAMI_TYPE_CONTROL_QUEUE))


/**
 * SwamiControlQueueTestFunc:
 * @queue: The queue object
 * @control: The control
 * @event: The control event
 *
 * A callback function type used to test if an event should be added to a queue.
 * An example of its usage would be a GUI queue which could test to see if the
 * event is being sent within the GUI thread or not.
 *
 * Returns: Function should return %TRUE if event should be queued, %FALSE to
 *   send event immediately.
 */
typedef gboolean (* SwamiControlQueueTestFunc)(SwamiControlQueue *queue,
					       SwamiControl *control,
					       SwamiControlEvent *event);

/* control event queue */
struct _SwamiControlQueue
{
  SwamiLock parent_instance;	/* derived from SwamiLock */

  SwamiControlQueueTestFunc test_func;

  GList *list; /* list of queued events (struct in SwamiControlQueue.c) */
  GList *tail;			/* tail of the list */
};

/* control value change queue class */
struct _SwamiControlQueueClass
{
  SwamiLockClass parent_class;
};

GType swami_control_queue_get_type (void);

SwamiControlQueue *swami_control_queue_new (void);
void swami_control_queue_add_event (SwamiControlQueue *queue,
				    SwamiControl *control,
				    SwamiControlEvent *event);
void swami_control_queue_run (SwamiControlQueue *queue);
void swami_control_queue_set_test_func (SwamiControlQueue *queue,
					SwamiControlQueueTestFunc test_func);

#endif
