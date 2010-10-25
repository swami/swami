/*
 * SwamiControlValue.h - Header for Swami GValue control
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
#ifndef __SWAMI_CONTROL_VALUE_H__
#define __SWAMI_CONTROL_VALUE_H__

#include <glib.h>
#include <glib-object.h>
#include <libswami/SwamiControl.h>

typedef struct _SwamiControlValue SwamiControlValue;
typedef struct _SwamiControlValueClass SwamiControlValueClass;

#define SWAMI_TYPE_CONTROL_VALUE   (swami_control_value_get_type ())
#define SWAMI_CONTROL_VALUE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_CONTROL_VALUE, \
   SwamiControlValue))
#define SWAMI_CONTROL_VALUE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_CONTROL_VALUE, \
			    SwamiControlValueClass))
#define SWAMI_IS_CONTROL_VALUE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_CONTROL_VALUE))
#define SWAMI_IS_CONTROL_VALUE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_CONTROL_VALUE))

/* Value control object */
struct _SwamiControlValue
{
  SwamiControl parent_instance;	/* derived from SwamiControl */

  GValue *value;		/* value being controlled */
  GDestroyNotify destroy; /* function to call to destroy value or NULL */
  GParamSpec *pspec;	 /* created param spec for controlled value */
};

/* Value control class */
struct _SwamiControlValueClass
{
  SwamiControlClass parent_class;
};

GType swami_control_value_get_type (void);

SwamiControlValue *swami_control_value_new (void);
void swami_control_value_assign_value (SwamiControlValue *ctrlvalue,
				       GValue *value, GDestroyNotify destroy);
void swami_control_value_alloc_value (SwamiControlValue *ctrlvalue);

#endif
