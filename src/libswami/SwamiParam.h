/*
 * SwamiParam.h - GParamSpec related functions
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
#ifndef __SWAMI_PARAM_H__
#define __SWAMI_PARAM_H__

#include <glib.h>
#include <glib-object.h>

/**
 * SwamiValueTransform:
 * @src: Source value to transform from
 * @dest: Destination value to store to
 * @user_data: User defined data (set when transform was connected for example)
 *
 * Prototype for value transform function.  The transform function should
 * handle any value conversions required.
 */
typedef void (*SwamiValueTransform)(const GValue *src, GValue *dest,
				    gpointer user_data);

gboolean swami_param_get_limits (GParamSpec *pspec, gdouble *min,
				 gdouble *max, gdouble *def,
				 gboolean *integer);
gboolean swami_param_set_limits (GParamSpec *pspec, gdouble min,
				 gdouble max, gdouble def);
gboolean swami_param_type_has_limits (GType param_type);
gboolean swami_param_convert (GParamSpec *dest, GParamSpec *src);
GParamSpec *swami_param_convert_new (GParamSpec *pspec, GType value_type);
gboolean swami_param_type_transformable (GType src_type, GType dest_type);
gboolean swami_param_type_transformable_value (GType src_valtype,
					       GType dest_valtype);
gboolean swami_param_transform (GParamSpec *dest, GParamSpec *src,
				SwamiValueTransform trans, gpointer user_data);
GParamSpec *swami_param_transform_new (GParamSpec *dest, GType value_type,
				       SwamiValueTransform trans,
				       gpointer user_data);
GType swami_param_type_from_value_type (GType value_type);

#endif
