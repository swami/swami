/*
 * SwamiParam.c - GParamSpec related functions
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
#include <stdarg.h>

#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchParamProp.h>
#include "SwamiParam.h"

/**
 * swami_param_get_limits:
 * @pspec: GParamSpec to get limits of
 * @min: Output: Minimum value of parameter specification
 * @max: Output: Maximum value of parameter specification
 * @def: Output: Default value of parameter specification
 * @integer: Output: %TRUE if integer parameter spec, %FALSE if floating point
 *
 * Get limits of a numeric parameter specification.
 *
 * Returns: %TRUE if @pspec is numeric, %FALSE otherwise (in which case
 *   output variables are undefined)
 */
gboolean
swami_param_get_limits(GParamSpec *pspec, gdouble *min, gdouble *max,
                       gdouble *def, gboolean *integer)
{
    GType type;
    gdouble rmin, rmax, rdef;
    gboolean isint;

    g_return_val_if_fail(G_IS_PARAM_SPEC(pspec), FALSE);

    type = G_PARAM_SPEC_TYPE(pspec);

    if(type == G_TYPE_PARAM_BOOLEAN)
    {
        GParamSpecBoolean *sp = (GParamSpecBoolean *)pspec;
        rmin = 0.0;
        rmax = 1.0;
        rdef = sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_CHAR)
    {
        GParamSpecChar *sp = (GParamSpecChar *)pspec;
        rmin = sp->minimum;
        rmax = sp->maximum;
        rdef = sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_UCHAR)
    {
        GParamSpecUChar *sp = (GParamSpecUChar *)pspec;
        rmin = sp->minimum;
        rmax = sp->maximum;
        rdef = sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_INT)
    {
        GParamSpecInt *sp = (GParamSpecInt *)pspec;
        rmin = sp->minimum;
        rmax = sp->maximum;
        rdef = sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_UINT)
    {
        GParamSpecUInt *sp = (GParamSpecUInt *)pspec;
        rmin = sp->minimum;
        rmax = sp->maximum;
        rdef = sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_LONG)
    {
        GParamSpecLong *sp = (GParamSpecLong *)pspec;
        rmin = sp->minimum;
        rmax = sp->maximum;
        rdef = sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_ULONG)
    {
        GParamSpecULong *sp = (GParamSpecULong *)pspec;
        rmin = sp->minimum;
        rmax = sp->maximum;
        rdef = sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_INT64)
    {
        GParamSpecInt64 *sp = (GParamSpecInt64 *)pspec;
        rmin = (gdouble)sp->minimum;
        rmax = (gdouble)sp->maximum;
        rdef = (gdouble)sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_UINT64)
    {
        GParamSpecUInt64 *sp = (GParamSpecUInt64 *)pspec;
        rmin = (gdouble)sp->minimum;
        rmax = (gdouble)sp->maximum;
        rdef = (gdouble)sp->default_value;
        isint = TRUE;
    }
    else if(type == G_TYPE_PARAM_FLOAT)
    {
        GParamSpecFloat *sp = (GParamSpecFloat *)pspec;
        rmin = sp->minimum;
        rmax = sp->maximum;
        rdef = sp->default_value;
        isint = FALSE;
    }
    else if(type == G_TYPE_PARAM_DOUBLE)
    {
        GParamSpecDouble *sp = (GParamSpecDouble *)pspec;
        rmin = sp->minimum;
        rmax = sp->maximum;
        rdef = sp->default_value;
        isint = FALSE;
    }
    else
    {
        return (FALSE);
    }

    if(min)
    {
        *min = rmin;
    }

    if(max)
    {
        *max = rmax;
    }

    if(def)
    {
        *def = rdef;
    }

    if(integer)
    {
        *integer = isint;
    }

    return (TRUE);
}

/**
 * swami_param_set_limits:
 * @pspec: GParamSpec to set limits of
 * @min: Minimum value of parameter specification
 * @max: Maximum value of parameter specification
 * @def: Default value of parameter specification
 *
 * Set limits of a numeric parameter specification.
 *
 * Returns: %TRUE if @pspec is numeric, %FALSE otherwise (in which case
 *   @pspec is unchanged)
 */
gboolean
swami_param_set_limits(GParamSpec *pspec, gdouble min, gdouble max,
                       gdouble def)
{
    GType type;

    g_return_val_if_fail(G_IS_PARAM_SPEC(pspec), FALSE);

    type = G_PARAM_SPEC_TYPE(pspec);

    if(type == G_TYPE_PARAM_BOOLEAN)
    {
        GParamSpecBoolean *sp = (GParamSpecBoolean *)pspec;
        sp->default_value = (def != 0.0);
    }
    else if(type == G_TYPE_PARAM_CHAR)
    {
        GParamSpecChar *sp = (GParamSpecChar *)pspec;
        sp->minimum = (gint8)min;
        sp->maximum = (gint8)max;
        sp->default_value = (gint8)def;
    }
    else if(type == G_TYPE_PARAM_UCHAR)
    {
        GParamSpecUChar *sp = (GParamSpecUChar *)pspec;
        sp->minimum = (guint8)min;
        sp->maximum = (guint8)max;
        sp->default_value = (guint8)def;
    }
    else if(type == G_TYPE_PARAM_INT)
    {
        GParamSpecInt *sp = (GParamSpecInt *)pspec;
        sp->minimum = (gint)min;
        sp->maximum = (gint)max;
        sp->default_value = (gint)def;
    }
    else if(type == G_TYPE_PARAM_UINT)
    {
        GParamSpecUInt *sp = (GParamSpecUInt *)pspec;
        sp->minimum = (guint)min;
        sp->maximum = (guint)max;
        sp->default_value = (guint)def;
    }
    else if(type == G_TYPE_PARAM_LONG)
    {
        GParamSpecLong *sp = (GParamSpecLong *)pspec;
        sp->minimum = (glong)min;
        sp->maximum = (glong)max;
        sp->default_value = (glong)def;
    }
    else if(type == G_TYPE_PARAM_ULONG)
    {
        GParamSpecULong *sp = (GParamSpecULong *)pspec;
        sp->minimum = (gulong)min;
        sp->maximum = (gulong)max;
        sp->default_value = (gulong)def;
    }
    else if(type == G_TYPE_PARAM_INT64)
    {
        GParamSpecInt64 *sp = (GParamSpecInt64 *)pspec;
        sp->minimum = (gint64)min;
        sp->maximum = (gint64)max;
        sp->default_value = (gint64)def;
    }
    else if(type == G_TYPE_PARAM_UINT64)
    {
        GParamSpecUInt64 *sp = (GParamSpecUInt64 *)pspec;
        sp->minimum = (guint64)min;
        sp->maximum = (guint64)max;
        sp->default_value = (guint64)def;
    }
    else if(type == G_TYPE_PARAM_FLOAT)
    {
        GParamSpecFloat *sp = (GParamSpecFloat *)pspec;
        sp->minimum = (gfloat)min;
        sp->maximum = (gfloat)max;
        sp->default_value = (gfloat)def;
    }
    else if(type == G_TYPE_PARAM_DOUBLE)
    {
        GParamSpecDouble *sp = (GParamSpecDouble *)pspec;
        sp->minimum = min;
        sp->maximum = max;
        sp->default_value = def;
    }
    else
    {
        return (FALSE);
    }

    return (TRUE);
}

/**
 * swami_param_type_has_limits:
 * @param_type: #GParamSpec type
 *
 * Check if a given #GParamSpec type can be used with swami_param_get_limits()
 * and swami_param_set_limits().
 *
 * Returns: %TRUE if can get/set limits, %FALSE otherwise
 */
gboolean
swami_param_type_has_limits(GType param_type)
{
    return (param_type == G_TYPE_PARAM_BOOLEAN
            || param_type == G_TYPE_PARAM_CHAR
            || param_type == G_TYPE_PARAM_UCHAR
            || param_type == G_TYPE_PARAM_INT
            || param_type == G_TYPE_PARAM_UINT
            || param_type == G_TYPE_PARAM_LONG
            || param_type == G_TYPE_PARAM_ULONG
            || param_type == G_TYPE_PARAM_INT64
            || param_type == G_TYPE_PARAM_UINT64
            || param_type == G_TYPE_PARAM_FLOAT
            || param_type == G_TYPE_PARAM_DOUBLE);
}

/**
 * swami_param_convert:
 * @src: Source param specification to get limits from
 * @dest: Destination param specification to set limits of
 *
 * Convert parameter limits between two numeric parameter specifications, also
 * copies the "unit-type" and "float-digit" parameter properties.  Sets
 * "float-digits" to 0 on the @dest parameter spec if the source is an integer
 * type.
 *
 * Returns: %TRUE if both parameter specs are numeric, %FALSE otherwise
 *   (in which case @dest will be unchanged).
 */
gboolean
swami_param_convert(GParamSpec *src, GParamSpec *dest)
{
    gdouble min, max, def;
    GValue value = { 0 };
    gboolean isint;

    g_return_val_if_fail(G_IS_PARAM_SPEC(dest), FALSE);
    g_return_val_if_fail(G_IS_PARAM_SPEC(src), FALSE);

    if(!swami_param_get_limits(src, &min, &max, &def, &isint))
    {
        return (FALSE);
    }

    if(!swami_param_set_limits(dest, min, max, def))
    {
        return (FALSE);
    }

    g_value_init(&value, G_TYPE_UINT);

    if(ipatch_param_get_property(src, "unit-type", &value))
    {
        ipatch_param_set_property(dest, "unit-type", &value);
    }

    if(isint)
    {
        g_value_set_uint(&value, 0);
    }

    if(isint || ipatch_param_get_property(src, "float-digits", &value))
    {
        ipatch_param_set_property(dest, "float-digits", &value);
    }

    return (TRUE);
}

/**
 * swami_param_convert_new:
 * @pspec: Source parameter spec to convert
 * @value_type: Value type of new parameter spec
 *
 * Create a new parameter spec using values of @value_type and convert
 * @pspec to the new parameter spec.
 *
 * Returns: New parameter spec that uses @value_type or %NULL on error
 *   (unable to convert @pspec to @value_type)
 */
GParamSpec *
swami_param_convert_new(GParamSpec *pspec, GType value_type)
{
    GParamSpec *newspec;
    GType newspec_type;

    g_return_val_if_fail(G_IS_PARAM_SPEC(pspec), NULL);

    newspec_type = swami_param_type_from_value_type(value_type);
    g_return_val_if_fail(newspec_type != 0, NULL);

    newspec = g_param_spec_internal(newspec_type, pspec->name, pspec->_nick,
                                    pspec->_blurb, pspec->flags);

    if(!swami_param_convert(pspec, newspec))
    {
        g_param_spec_ref(newspec);
        g_param_spec_sink(newspec);
        g_param_spec_unref(newspec);
        return (NULL);
    }

    return (newspec);
}

/**
 * swami_param_type_transformable:
 * @src_type: Source #GParamSpec type.
 * @dest_type: Destination #GParamSpec type.
 *
 * Check if a source #GParamSpec type is transformable to a destination type.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
swami_param_type_transformable(GType src_type, GType dest_type)
{
    return (swami_param_type_has_limits(src_type)
            && swami_param_type_has_limits(dest_type));
}

/**
 * swami_param_type_transformable_value:
 * @src_type: Source #GValue type.
 * @dest_type: Destination #GValue type.
 *
 * Check if a source #GParamSpec type is transformable to a destination type
 * by corresponding #GValue types.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
swami_param_type_transformable_value(GType src_valtype, GType dest_valtype)
{
    GType src_type, dest_type;

    src_type = swami_param_type_from_value_type(src_valtype);
    dest_type = swami_param_type_from_value_type(dest_valtype);

    return (swami_param_type_has_limits(src_type)
            && swami_param_type_has_limits(dest_type));
}

/**
 * swami_param_transform:
 * @src: Source param specification to get limits from
 * @dest: Destination param specification to set limits of
 * @trans: Transform function
 * @user_data: User defined data passed to transform function
 *
 * Convert parameter limits between two numeric parameter specifications using
 * a custom transform function.  The @trans parameter specifies the transform
 * function which is called for the min, max and default values of the @src
 * parameter spec, which are then assigned to @dest.  The source and destination
 * GValue parameters to the transform function are of #G_TYPE_DOUBLE.
 *
 * Returns: %TRUE if both parameter specs are numeric, %FALSE otherwise
 *   (in which case @dest will be unchanged).
 */
gboolean
swami_param_transform(GParamSpec *src, GParamSpec *dest,
                      SwamiValueTransform trans, gpointer user_data)
{
    gdouble min, max, def;
    gdouble tmin, tmax, tdef;
    GValue srcval = { 0 }, destval = { 0 };

    g_return_val_if_fail(G_IS_PARAM_SPEC(src), FALSE);
    g_return_val_if_fail(G_IS_PARAM_SPEC(dest), FALSE);
    g_return_val_if_fail(trans != NULL, FALSE);

    if(!swami_param_get_limits(src, &min, &max, &def, NULL))
    {
        return (FALSE);
    }

    g_value_init(&srcval, G_TYPE_DOUBLE);
    g_value_init(&destval, G_TYPE_DOUBLE);

    g_value_set_double(&srcval, min);
    trans(&srcval, &destval, user_data);
    tmin = g_value_get_double(&destval);

    g_value_reset(&srcval);
    g_value_reset(&destval);

    g_value_set_double(&srcval, max);
    trans(&srcval, &destval, user_data);
    tmax = g_value_get_double(&destval);

    g_value_reset(&srcval);
    g_value_reset(&destval);

    g_value_set_double(&srcval, def);
    trans(&srcval, &destval, user_data);
    tdef = g_value_get_double(&destval);

    g_value_unset(&srcval);
    g_value_unset(&destval);

    if(!swami_param_set_limits(dest, tmin, tmax, tdef))
    {
        return (FALSE);
    }

    return (TRUE);
}

/**
 * swami_param_transform_new:
 * @pspec: Source parameter spec to convert
 * @value_type: Value type of new parameter spec
 * @trans: Transform function
 * @user_data: User defined data passed to transform function
 *
 * Create a new parameter spec using values of @value_type and transform
 * @pspec to the new parameter spec using a custom transform function.
 * See swami_param_transform() for more details.
 *
 * Returns: New parameter spec that uses @value_type or %NULL on error
 *   (error message is then printed)
 */
GParamSpec *
swami_param_transform_new(GParamSpec *pspec, GType value_type,
                          SwamiValueTransform trans, gpointer user_data)
{
    GParamSpec *newspec;
    GType newspec_type;

    g_return_val_if_fail(G_IS_PARAM_SPEC(pspec), NULL);
    g_return_val_if_fail(trans != NULL, NULL);

    newspec_type = swami_param_type_from_value_type(value_type);
    g_return_val_if_fail(newspec_type != 0, NULL);

    newspec = g_param_spec_internal(newspec_type, pspec->name, pspec->_nick,
                                    pspec->_blurb, pspec->flags);

    if(!swami_param_transform(pspec, newspec, trans, user_data))
    {
        g_critical("%s: Failed to transform param spec of type '%s' to '%s'",
                   G_STRLOC,
                   G_PARAM_SPEC_TYPE_NAME(pspec),
                   G_PARAM_SPEC_TYPE_NAME(newspec));
        g_param_spec_ref(newspec);
        g_param_spec_sink(newspec);
        g_param_spec_unref(newspec);
        return (FALSE);
    }

    return (newspec);
}

/**
 * swami_param_type_from_value_type:
 * @value_type: A value type to get the parameter spec type that contains it.
 *
 * Get a parameter spec type for a given value type.
 *
 * Returns: Parameter spec type or 0 if no param spec type for @value_type.
 */
GType
swami_param_type_from_value_type(GType value_type)
{
    value_type = G_TYPE_FUNDAMENTAL(value_type);

    switch(value_type)
    {
    case G_TYPE_BOOLEAN:
        return G_TYPE_PARAM_BOOLEAN;

    case G_TYPE_CHAR:
        return G_TYPE_PARAM_CHAR;

    case G_TYPE_UCHAR:
        return G_TYPE_PARAM_UCHAR;

    case G_TYPE_INT:
        return G_TYPE_PARAM_INT;

    case G_TYPE_UINT:
        return G_TYPE_PARAM_UINT;

    case G_TYPE_LONG:
        return G_TYPE_PARAM_LONG;

    case G_TYPE_ULONG:
        return G_TYPE_PARAM_ULONG;

    case G_TYPE_INT64:
        return G_TYPE_PARAM_INT64;

    case G_TYPE_UINT64:
        return G_TYPE_PARAM_UINT64;

    case G_TYPE_ENUM:
        return G_TYPE_PARAM_ENUM;

    case G_TYPE_FLAGS:
        return G_TYPE_PARAM_FLAGS;

    case G_TYPE_FLOAT:
        return G_TYPE_PARAM_FLOAT;

    case G_TYPE_DOUBLE:
        return G_TYPE_PARAM_DOUBLE;

    case G_TYPE_STRING:
        return G_TYPE_PARAM_STRING;

    case G_TYPE_POINTER:
        return G_TYPE_PARAM_POINTER;

    case G_TYPE_BOXED:
        return G_TYPE_PARAM_BOXED;

    case G_TYPE_OBJECT:
        return G_TYPE_PARAM_OBJECT;

    default:
        return 0;
    }
}
