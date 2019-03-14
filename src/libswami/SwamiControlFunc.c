/*
 * SwamiControlFunc.c - Swami function control object
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

#include "SwamiControlFunc.h"
#include "SwamiControl.h"
#include "SwamiLog.h"
#include "swami_priv.h"
#include "util.h"

static void swami_control_func_class_init (SwamiControlFuncClass *klass);
static void swami_control_func_finalize (GObject *object);
static GParamSpec *control_func_get_spec_method (SwamiControl *control);
static gboolean control_func_set_spec_method (SwamiControl *control,
					      GParamSpec *spec);
static void control_func_get_value_method (SwamiControl *control,
					   GValue *value);
static void control_func_set_value_method (SwamiControl *control,
					   SwamiControlEvent *event,
					   const GValue *value);

static GObjectClass *parent_class = NULL;

GType
swami_control_func_get_type (void)
{
  static GType otype = 0;

  if (!otype)
    {
      static const GTypeInfo type_info =
	{
	  sizeof (SwamiControlFuncClass), NULL, NULL,
	  (GClassInitFunc) swami_control_func_class_init,
	  (GClassFinalizeFunc) NULL, NULL,
	  sizeof (SwamiControlFunc), 0,
	  (GInstanceInitFunc) NULL
	};

      otype = g_type_register_static (SWAMI_TYPE_CONTROL, "SwamiControlFunc",
				      &type_info, 0);
    }

  return (otype);
}

static void
swami_control_func_class_init (SwamiControlFuncClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  SwamiControlClass *control_class = SWAMI_CONTROL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  obj_class->finalize = swami_control_func_finalize;

  control_class->get_spec = control_func_get_spec_method;
  control_class->set_spec = control_func_set_spec_method;
  control_class->get_value = control_func_get_value_method;
  control_class->set_value = control_func_set_value_method;
}

static void
swami_control_func_finalize (GObject *object)
{
  SwamiControlFunc *ctrlfunc = SWAMI_CONTROL_FUNC (object);

  SWAMI_LOCK_WRITE (ctrlfunc);

  if (ctrlfunc->pspec) g_param_spec_unref (ctrlfunc->pspec);
  if (ctrlfunc->destroy_func) (*ctrlfunc->destroy_func)(ctrlfunc);

  ctrlfunc->get_func = NULL;
  ctrlfunc->set_func = NULL;
  ctrlfunc->destroy_func = NULL;

  SWAMI_UNLOCK_WRITE (ctrlfunc);

  parent_class->finalize (object);
}

/* control is locked by caller */
static GParamSpec *
control_func_get_spec_method (SwamiControl *control)
{
  SwamiControlFunc *ctrlfunc = SWAMI_CONTROL_FUNC (control);
  return (ctrlfunc->pspec);
}

/* control is locked by caller */
static gboolean
control_func_set_spec_method (SwamiControl *control, GParamSpec *spec)
{
  SwamiControlFunc *ctrlfunc = SWAMI_CONTROL_FUNC (control);
  if (ctrlfunc->pspec) g_param_spec_unref (ctrlfunc->pspec);
  ctrlfunc->pspec = g_param_spec_ref (spec);
  g_param_spec_sink (spec);	/* take ownership of the parameter spec */

  return (TRUE);
}

/* locking is up to user (not locked) */
static void
control_func_get_value_method (SwamiControl *control, GValue *value)
{
  SwamiControlGetValueFunc func = SWAMI_CONTROL_FUNC (control)->get_func;
  if (func) (*func)(control, value);
}

/* locking is up to user (not locked) */
static void
control_func_set_value_method (SwamiControl *control, SwamiControlEvent *event,
			       const GValue *value)
{
  SwamiControlSetValueFunc func = SWAMI_CONTROL_FUNC (control)->set_func;
  (*func)(control, event, value);
}

/**
 * swami_control_func_new:
 *
 * Create a new function callback control. Function controls are useful for
 * easily creating custom controls using callback functions.
 *
 * Returns: New function control with a refcount of 1 which the caller owns.
 */
SwamiControlFunc *
swami_control_func_new (void)
{
  return (SWAMI_CONTROL_FUNC (g_object_new (SWAMI_TYPE_CONTROL_FUNC, NULL)));
}

/**
 * swami_control_func_assign_funcs:
 * @ctrlfunc: Swami function control object
 * @get_func: Function to call to get control value or %NULL if not a readable
 *   value control (may still send events)
 * @set_func: Function to call to set control value or %NULL if not a writable
 *   value control
 * @destroy_func: Function to call when control is destroyed or function
 *   callbacks are changed or %NULL if no cleanup is needed.
 * @user_data: User defined pointer (set in @ctrlfunc instance).
 *
 * Assigns callback functions to a Swami function control object.
 * These callback functions should handle the getting and setting of the
 * control's value. The value passed to these callback functions is initialized
 * to the type of the control's parameter spec and this type should not be
 * changed. The @destroy_func callback is called when the control is destroyed
 * or when the callback functions are changed and should do any needed cleanup.
 * The control is not locked for any of these callbacks and so must be done
 * in the callback if there are any thread sensitive operations.
 */
void
swami_control_func_assign_funcs (SwamiControlFunc *ctrlfunc,
				 SwamiControlGetValueFunc get_func,
				 SwamiControlSetValueFunc set_func,
				 SwamiControlFuncDestroy destroy_func,
				 gpointer user_data)
{
  SwamiControl *control;

  g_return_if_fail (SWAMI_IS_CONTROL_FUNC (ctrlfunc));
  control = SWAMI_CONTROL (ctrlfunc);

  SWAMI_LOCK_WRITE (ctrlfunc);

  /* ensure input/output connections are still valid if changing functions */
  if (control->inputs && !set_func)
    {
      g_critical ("%s: Invalid writable function control function change",
		  G_STRLOC);
      SWAMI_UNLOCK_WRITE (ctrlfunc);
      return;
    }

  if (ctrlfunc->destroy_func) (*ctrlfunc->destroy_func)(ctrlfunc);

  control->flags = SWAMI_CONTROL_SENDS | (set_func ? SWAMI_CONTROL_RECVS : 0);

  ctrlfunc->get_func = get_func;
  ctrlfunc->set_func = set_func;
  ctrlfunc->destroy_func = destroy_func;
  ctrlfunc->user_data = user_data;

  SWAMI_UNLOCK_WRITE (ctrlfunc);
}
