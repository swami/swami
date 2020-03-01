/*
 * SwamiEvent_ipatch.h - libInstPatch SwamiControl event types
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
#ifndef __SWAMI_CONTROL_IPATCH_H__
#define __SWAMI_CONTROL_IPATCH_H__

#include <libinstpatch/IpatchItem.h>

/* libInstPatch event box types */

#define SWAMI_TYPE_EVENT_ITEM_ADD   (swami_event_item_add_get_type ())
#define SWAMI_VALUE_HOLDS_EVENT_ITEM_ADD(value) \
  (G_TYPE_CHECK_VALUE_TYPE ((value), SWAMI_TYPE_EVENT_ITEM_ADD))

#define SWAMI_TYPE_EVENT_ITEM_REMOVE  (swami_event_item_remove_get_type ())
#define SWAMI_VALUE_HOLDS_EVENT_ITEM_REMOVE(value) \
  (G_TYPE_CHECK_VALUE_TYPE ((value), SWAMI_TYPE_EVENT_ITEM_REMOVE))

#define SWAMI_TYPE_EVENT_PROP_CHANGE  (swami_event_prop_change_get_type ())
#define SWAMI_VALUE_HOLDS_EVENT_PROP_CHANGE(value) \
  (G_TYPE_CHECK_VALUE_TYPE ((value), SWAMI_TYPE_EVENT_PROP_CHANGE))

/* item add just uses IpatchItem pointer */
typedef IpatchItem SwamiEventItemAdd;

typedef struct _SwamiEventItemRemove
{
    IpatchItem *item;		/* item removed or to be removed */
    IpatchItem *parent;		/* parent of item */
} SwamiEventItemRemove;

typedef struct _SwamiEventPropChange
{
    GObject *object;		/* object whose property changed */
    GParamSpec *pspec;		/* property parameter spec */
    GValue value;			/* new value */
} SwamiEventPropChange;


GType swami_event_item_add_get_type(void);
GType swami_event_item_remove_get_type(void);
GType swami_event_prop_change_get_type(void);

SwamiEventItemAdd *swami_event_item_add_copy(SwamiEventItemAdd *item_add);
void swami_event_item_add_free(SwamiEventItemAdd *item_add);
SwamiEventItemRemove *swami_event_item_remove_new(void);
SwamiEventItemRemove *
swami_event_item_remove_copy(SwamiEventItemRemove *item_remove);
void swami_event_item_remove_free(SwamiEventItemRemove *item_remove);
SwamiEventPropChange *swami_event_prop_change_new(void);
SwamiEventPropChange *
swami_event_prop_change_copy(SwamiEventPropChange *prop_change);
void swami_event_prop_change_free(SwamiEventPropChange *prop_change);


#endif
