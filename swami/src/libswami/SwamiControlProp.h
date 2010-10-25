/*
 * SwamiControlProp.h - Header for Swami GObject property control
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
#ifndef __SWAMI_CONTROL_PROP_H__
#define __SWAMI_CONTROL_PROP_H__

#include <glib.h>
#include <glib-object.h>
#include <libswami/SwamiControl.h>

typedef struct _SwamiControlProp SwamiControlProp;
typedef struct _SwamiControlPropClass SwamiControlPropClass;

#define SWAMI_TYPE_CONTROL_PROP   (swami_control_prop_get_type ())
#define SWAMI_CONTROL_PROP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_CONTROL_PROP, \
   SwamiControlProp))
#define SWAMI_CONTROL_PROP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_CONTROL_PROP, \
			    SwamiControlPropClass))
#define SWAMI_IS_CONTROL_PROP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_CONTROL_PROP))
#define SWAMI_IS_CONTROL_PROP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_CONTROL_PROP))

/* Property control object */
struct _SwamiControlProp
{
  SwamiControl parent_instance;	/* derived from SwamiControl */

  GObject *object;		/* object being controlled */
  GParamSpec *spec; /* parameter spec of the property being controlled */
  gulong notify_handler_id; /* ID of object "notify" signal handler */
  guint item_handler_id; /* IpatchItem property callback handler ID */
  gboolean send_events;	/* when TRUE control uses SwamiEventPropChange events */
};

/* Property control class */
struct _SwamiControlPropClass
{
  SwamiControlClass parent_class;
};

SwamiControl *swami_get_control_prop (GObject *object, GParamSpec *pspec);
SwamiControl *swami_get_control_prop_by_name (GObject *object,
						  const char *name);
void swami_control_prop_connect_objects (GObject *src, const char *propname1,
					 GObject *dest, const char *propname2,
					 guint flags);
void swami_control_prop_connect_to_control (GObject *src, const char *propname,
					    SwamiControl *dest, guint flags);
void swami_control_prop_connect_from_control (SwamiControl *src, GObject *dest,
					      const char *propname, guint flags);
GType swami_control_prop_get_type (void);

SwamiControlProp *swami_control_prop_new (GObject *object, GParamSpec *pspec);
void swami_control_prop_assign (SwamiControlProp *ctrlprop, GObject *object,
				GParamSpec *pspec, gboolean send_events);
void swami_control_prop_assign_by_name (SwamiControlProp *ctrlprop,
				        GObject *object, const char *prop_name);
#endif
