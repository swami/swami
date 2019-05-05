/*
 * SwamiControlEvent.h - Swami control event structure (not an object)
 * A structure that defines a control event.
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
#ifndef __SWAMI_CONTROL_EVENT_H__
#define __SWAMI_CONTROL_EVENT_H__

#ifdef _WIN32
#include<Windows.h>
#else
#include <sys/time.h>
#endif
#include <glib.h>

#include <libinstpatch/IpatchUnit.h>
#include <libswami/SwamiParam.h>

typedef struct _SwamiControlEvent SwamiControlEvent;

/* a boxed type for SwamiControlEvent */
#define SWAMI_TYPE_CONTROL_EVENT (swami_control_event_get_type ())

struct _SwamiControlEvent
{
  struct timeval tick;		/* tick time */
  SwamiControlEvent *origin;	/* origin event or %NULL if is origin */
  GValue value;			/* value for this event */
  int active;			/* active propagation count */
  int refcount;			/* reference count */
};

/* an accessor macro for the value field of an event */
#define SWAMI_CONTROL_EVENT_VALUE(event) (&((event)->value))

#include <libswami/SwamiControl.h>

GType swami_control_event_get_type (void);
SwamiControlEvent *swami_control_event_new (gboolean stamp);
void swami_control_event_free (SwamiControlEvent *event);
SwamiControlEvent *
swami_control_event_duplicate (const SwamiControlEvent *event);
SwamiControlEvent *swami_control_event_transform
  (SwamiControlEvent *event, GType valtype, SwamiValueTransform trans,
   gpointer data);
void swami_control_event_stamp (SwamiControlEvent *event);
void swami_control_event_set_origin (SwamiControlEvent *event,
				     SwamiControlEvent *origin);
SwamiControlEvent *swami_control_event_ref (SwamiControlEvent *event);
void swami_control_event_unref (SwamiControlEvent *event);
void swami_control_event_active_ref (SwamiControlEvent *event);
void swami_control_event_active_unref (SwamiControlEvent *event);

#endif
