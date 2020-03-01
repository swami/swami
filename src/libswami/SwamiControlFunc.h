/*
 * SwamiControlFunc.h - Header for Swami control function object
 * A convenient control type that uses user defined callback routines.
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
#ifndef __SWAMI_CONTROL_FUNC_H__
#define __SWAMI_CONTROL_FUNC_H__

#include <glib.h>
#include <glib-object.h>
#include <libswami/SwamiControl.h>

typedef struct _SwamiControlFunc SwamiControlFunc;
typedef struct _SwamiControlFuncClass SwamiControlFuncClass;

#define SWAMI_TYPE_CONTROL_FUNC   (swami_control_func_get_type ())
#define SWAMI_CONTROL_FUNC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_CONTROL_FUNC, \
   SwamiControlFunc))
#define SWAMI_CONTROL_FUNC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_CONTROL_FUNC, \
			    SwamiControlFuncClass))
#define SWAMI_IS_CONTROL_FUNC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_CONTROL_FUNC))
#define SWAMI_IS_CONTROL_FUNC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_CONTROL_FUNC))


/**
 * SwamiControlFuncDestroy:
 * @control: Function control object
 *
 * A function to call when the function control is destroyed or when the
 * function callbacks are changed. This function should handle all cleanup
 * for the callback functions. This function is called with @control
 * multi-thread write locked.
 */
typedef void (*SwamiControlFuncDestroy)(SwamiControlFunc *control);

/* function control object */
struct _SwamiControlFunc
{
    SwamiControl parent_instance;	/* derived from SwamiControl */
    SwamiControlGetValueFunc get_func; /* callback function to get value */
    SwamiControlSetValueFunc set_func; /* callback function to set value */
    SwamiControlFuncDestroy destroy_func;	/* destroy function */
    gpointer user_data;		/* user data for callback functions */
    GParamSpec *pspec; /* optional parameter specification for this control */
};

/* function control class */
struct _SwamiControlFuncClass
{
    SwamiControlClass parent_class;
};

/* macro to get user data field from a SwamiControlFunc */
#define SWAMI_CONTROL_FUNC_DATA(ctrl)  (SWAMI_CONTROL_FUNC (ctrl)->user_data)


GType swami_control_func_get_type(void);

SwamiControlFunc *swami_control_func_new(void);
void swami_control_func_assign_funcs(SwamiControlFunc *ctrlfunc,
                                     SwamiControlGetValueFunc get_func,
                                     SwamiControlSetValueFunc set_func,
                                     SwamiControlFuncDestroy destroy_func,
                                     gpointer user_data);
#endif
