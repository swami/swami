/*
 * SwamiControlValue.c - Swami GValue control object
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
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "SwamiControlValue.h"
#include "SwamiControl.h"
#include "SwamiLog.h"
#include "swami_priv.h"
#include "util.h"

static void swami_control_value_class_init (SwamiControlValueClass *klass);
static void swami_control_value_init (SwamiControlValue *ctrlvalue);
static void swami_control_value_finalize (GObject *object);
static GParamSpec *control_value_get_spec_method (SwamiControl *control);
static gboolean control_value_set_spec_method (SwamiControl *control,
					       GParamSpec *pspec);
static void control_value_get_value_method (SwamiControl *control,
					    GValue *value);
static void control_value_set_value_method (SwamiControl *control,
					    SwamiControlEvent *event,
					    const GValue *value);

static GObjectClass *parent_class = NULL;

GType
swami_control_value_get_type (void)
{
  static GType otype = 0;

  if (!otype)
    {
      static const GTypeInfo type_info =
	{
	  sizeof (SwamiControlValueClass), NULL, NULL,
	  (GClassInitFunc) swami_control_value_class_init,
	  (GClassFinalizeFunc) NULL, NULL,
	  sizeof (SwamiControlValue), 0,
	  (GInstanceInitFunc) swami_control_value_init
	};

      otype = g_type_register_static (SWAMI_TYPE_CONTROL, "SwamiControlValue",
				      &type_info, 0);
    }

  return (otype);
}

static void
swami_control_value_class_init (SwamiControlValueClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  SwamiControlClass *control_class = SWAMI_CONTROL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  obj_class->finalize = swami_control_value_finalize;

  control_class->get_spec = control_value_get_spec_method;
  control_class->set_spec = control_value_set_spec_method;
  control_class->get_value = control_value_get_value_method;
  control_class->set_value = control_value_set_value_method;
}

static void
swami_control_value_init (SwamiControlValue *ctrlvalue)
{
  swami_control_set_flags (SWAMI_CONTROL (ctrlvalue),
			   SWAMI_CONTROL_SENDRECV);
}

static void
swami_control_value_finalize (GObject *object)
{
  SwamiControlValue *ctrlvalue = SWAMI_CONTROL_VALUE (object);

  if (ctrlvalue->destroy && ctrlvalue->value)
    (*ctrlvalue->destroy)(ctrlvalue->value);

  if (ctrlvalue->pspec) g_param_spec_unref (ctrlvalue->pspec);

  parent_class->finalize (object);
}

/* control is locked by caller */
static GParamSpec *
control_value_get_spec_method (SwamiControl *control)
{
  SwamiControlValue *ctrlvalue = SWAMI_CONTROL_VALUE (control);
  return (ctrlvalue->pspec);
}

/* control is locked by caller */
static gboolean
control_value_set_spec_method (SwamiControl *control, GParamSpec *pspec)
{
  SwamiControlValue *ctrlvalue = SWAMI_CONTROL_VALUE (control);

  if (ctrlvalue->pspec) g_param_spec_unref (ctrlvalue->pspec);
  ctrlvalue->pspec = g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);

  return (TRUE);
}

static void
control_value_get_value_method (SwamiControl *control, GValue *value)
{
  SwamiControlValue *ctrlvalue = SWAMI_CONTROL_VALUE (control);
  g_value_copy (ctrlvalue->value, value);
}

static void
control_value_set_value_method (SwamiControl *control,
				SwamiControlEvent *event, const GValue *value)
{
  SwamiControlValue *ctrlvalue = SWAMI_CONTROL_VALUE (control);
  g_value_copy (value, ctrlvalue->value);
}

/**
 * swami_control_value_new:
 *
 * Create a new GValue control.
 *
 * Returns: New GValue control with a refcount of 1 which the caller owns.
 */
SwamiControlValue *
swami_control_value_new (void)
{
  return (SWAMI_CONTROL_VALUE (g_object_new (SWAMI_TYPE_CONTROL_VALUE, NULL)));
}

/**
 * swami_control_value_assign_value:
 * @ctrlvalue: Swami GValue control object
 * @value: Value to be controlled
 * @destroy: A function to destroy @value or %NULL
 *
 * Assigns a GValue to be controlled by a Swami GValue control object.
 * If @destroy is set it will be called on the @value when it is no longer
 * being used. The @ctrlvalue should already have a parameter specification.
 * If the @value type is the same as the GParamSpec value type it will be
 * used as is otherwise it will be initialized to the parameter spec type.
 */
void
swami_control_value_assign_value (SwamiControlValue *ctrlvalue,
				  GValue *value, GDestroyNotify destroy)
{
  GValue *destroy_value = NULL;
  GDestroyNotify destroy_func;

  g_return_if_fail (SWAMI_IS_CONTROL_VALUE (ctrlvalue));
  g_return_if_fail (value != NULL);

  SWAMI_LOCK_WRITE (ctrlvalue);

  if (swami_log_if_fail (ctrlvalue->pspec != NULL))
    {
      SWAMI_UNLOCK_WRITE (ctrlvalue);
      return;
    }

  if (ctrlvalue->destroy && ctrlvalue->value)
    {
      destroy_value = ctrlvalue->value;
      destroy_func = ctrlvalue->destroy;
    }

  ctrlvalue->value = value;
  ctrlvalue->destroy = destroy;

  /* ensure existing value is set to the type specified by spec */
  if (G_VALUE_TYPE (ctrlvalue->value)
      != G_PARAM_SPEC_VALUE_TYPE (ctrlvalue->pspec))
    {
      g_value_unset (ctrlvalue->value);
      g_value_init (ctrlvalue->value,
		    G_PARAM_SPEC_VALUE_TYPE (ctrlvalue->pspec));
    }

  SWAMI_UNLOCK_WRITE (ctrlvalue);

  /* destroy value outside of lock (if any) */
  if (destroy_value) (*destroy_func)(destroy_value);
}

/**
 * swami_control_value_alloc_value:
 * @ctrlvalue: Swami GValue control object
 *
 * Allocate a GValue and assign it to the @ctrlvalue object. See
 * swami_control_value_assign_value() to assign an existing GValue to
 * a Swami GValue control object. The @ctrlvalue should already have an
 * assigned parameter spec.
 */
void
swami_control_value_alloc_value (SwamiControlValue *ctrlvalue)
{
  GValue *destroy_value = NULL;
  GDestroyNotify destroy_func;
  GValue *value;
  
  g_return_if_fail (SWAMI_IS_CONTROL_VALUE (ctrlvalue));

  SWAMI_LOCK_WRITE (ctrlvalue);

  if (swami_log_if_fail (ctrlvalue->pspec != NULL))
    {
      SWAMI_UNLOCK_WRITE (ctrlvalue);
      return;
    }

  if (ctrlvalue->destroy && ctrlvalue->value)
    {
      destroy_value = ctrlvalue->value;
      destroy_func = ctrlvalue->destroy;
    }

  value = swami_util_new_value ();
  g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (ctrlvalue->pspec));
  ctrlvalue->value = value;
  ctrlvalue->destroy = (GDestroyNotify)swami_util_free_value;

  SWAMI_UNLOCK_WRITE (ctrlvalue);

  /* destroy value outside of lock (if any) */
  if (destroy_value) (*destroy_func)(destroy_value);
}
