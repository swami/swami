/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
/**
 * SECTION: IpatchParamProp
 * @short_description: GParamSpec extended properties
 * @see_also: 
 * @stability: Stable
 *
 * Extensions to standard GParamSpec include flags (for
 * compact single bit data extensions) and GValue based extensions.
 * An example of usage is the IPATCH_PARAM_UNIQUE flag which indicates
 * a parameter that should be unique amongst sibling items and the
 * "string-max-length" integer GValue which specifies a max
 * length of a GParamSpecString parameter.
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "IpatchParamProp.h"
#include "i18n.h"


static void param_list_properties_GHFunc (gpointer key, gpointer value,
					  gpointer user_data);
static void param_set_property (GParamSpec *spec, GParamSpec *prop_spec,
				const GValue *value);
static void param_value_destroy_notify (gpointer user_data);
static gboolean param_get_property (GParamSpec *spec, GParamSpec *prop_spec,
				    GValue *value);


G_LOCK_DEFINE_STATIC (param_prop_hash);

/* GParamSpec GValue property hash */
static GHashTable *param_prop_hash = NULL;


/**
 * _ipatch_param_init: (skip)
 *
 * Initialize parameter/unit system
 */
void
_ipatch_param_init (void)
{
  param_prop_hash = g_hash_table_new_full (NULL, NULL, NULL,
					   (GDestroyNotify)g_param_spec_unref);

  /* install string length property */
  ipatch_param_install_property
    (g_param_spec_uint ("string-max-length", _("Max Length"),
			_("Max string length (0=no limit)"),
			0, G_MAXUINT, 0, G_PARAM_READWRITE));

  /* install floating point digits property */
  ipatch_param_install_property
    (g_param_spec_uint ("float-digits", _("Float Digits"),
			_("Significant decimal digits"),
			0, 64, 2, G_PARAM_READWRITE));

  /* install unique group ID property */
  ipatch_param_install_property
    (g_param_spec_uint ("unique-group-id", _("Unique group ID"),
			_("For grouping multiple unique properties"),
			0, G_MAXUINT, 0, G_PARAM_READWRITE));

  /* install unit type property */
  ipatch_param_install_property
    (g_param_spec_uint ("unit-type", _("Units"),
     _("Type of units used"), 0, G_MAXUINT, 0, G_PARAM_READWRITE));
}

/**
 * ipatch_param_install_property:
 * @prop_spec: Specification for the new #GParamSpec property.  Ownership
 *   of the GParamSpec is taken over by this function.  The name field of
 *   the GParamSpec should be a static string and is used as the property's
 *   ID.
 * 
 * Install a new #GParamSpec property which can be used to extend existing
 * #GParamSpec types or define common parameters shared by all types. An
 * example property is the "string-max-length" property which defines a
 * max length for use with #GParamSpecString properties.
 */
void
ipatch_param_install_property (GParamSpec *prop_spec)
{
  GQuark quark;

  g_return_if_fail (G_IS_PARAM_SPEC (prop_spec));
  g_return_if_fail (prop_spec->name != NULL);

  /* take ownership of the parameter spec */
  g_param_spec_ref (prop_spec);
  g_param_spec_sink (prop_spec);

  quark = g_quark_from_static_string (prop_spec->name);

  G_LOCK (param_prop_hash);
  g_hash_table_insert (param_prop_hash, GUINT_TO_POINTER (quark), prop_spec);
  G_UNLOCK (param_prop_hash);
}

/**
 * ipatch_param_find_property:
 * @name: Name of GParamSpec property
 * 
 * Lookup a GParamSpec property by name.
 * 
 * Returns: (transfer none): The matching GParamSpec or %NULL if not found.
 *   The GParamSpec is internal and should NOT be modified or freed.
 */
GParamSpec *
ipatch_param_find_property (const char *name)
{
  GParamSpec *spec;
  GQuark quark;

  g_return_val_if_fail (name != NULL, NULL);

  quark = g_quark_try_string (name);
  if (!quark) return (NULL);

  G_LOCK (param_prop_hash);
  spec = g_hash_table_lookup (param_prop_hash, GUINT_TO_POINTER (quark));
  G_UNLOCK (param_prop_hash);

  return (spec);
}

/**
 * ipatch_param_list_properties:
 * @n_properties: (out): Length of returned array
 * 
 * Get a list of all registered GParamSpec properties.
 * 
 * Returns: (array length=n_properties) (transfer container): An array of
 *   GParamSpecs which should be freed when finished.
 */
GParamSpec **
ipatch_param_list_properties (guint *n_properties)
{
  GParamSpec **specs, **specp;

  g_return_val_if_fail (n_properties != NULL, NULL);

  G_LOCK (param_prop_hash);
  specs = g_new (GParamSpec *, g_hash_table_size (param_prop_hash));
  specp = specs;
  g_hash_table_foreach (param_prop_hash, param_list_properties_GHFunc, &specp);
  G_UNLOCK (param_prop_hash);

  return (specs);
}

static void
param_list_properties_GHFunc (gpointer key, gpointer value, gpointer user_data)
{
  GParamSpec ***specs = user_data;
  **specs = (GParamSpec *)value;
  *specs = *specs + 1;
}

/**
 * ipatch_param_set:
 * @spec: Parameter spec to set extended properties of
 * @first_property_name: Name of first property to set
 * @...: Value of first property to set and optionally followed by more
 *   property name/value pairs, terminated with %NULL name.
 * 
 * Set extended parameter properties.  Parameter properties are used to
 * extend existing #GParamSpec types.  "string-max-length" is an example of
 * an extended property, which is used for #GParamSpecString parameters to
 * define the max allowed length.
 *
 * Returns: (transfer none): The @spec pointer for convenience, makes it easy
 *   to create/install a parameter spec and set its properties at the same time.
 */
GParamSpec *
ipatch_param_set (GParamSpec *spec, const char *first_property_name, ...)
{
  va_list args;

  va_start (args, first_property_name);
  ipatch_param_set_valist (spec, first_property_name, args);
  va_end (args);

  return (spec);
}

/**
 * ipatch_param_set_valist:
 * @spec: Parameter spec to set extended properties of
 * @first_property_name: Name of first property to set
 * @args: Value of first property to set and optionally followed by more
 *   property name/value pairs, terminated with %NULL name.
 * 
 * Like ipatch_param_set() but uses a va_list.
 */
void
ipatch_param_set_valist (GParamSpec *spec, const char *first_property_name,
			 va_list args)
{
  const char *name;
  GValue value = { 0, };
  gchar *error = NULL;
  GParamSpec *prop_spec;

  g_return_if_fail (G_IS_PARAM_SPEC (spec));
  g_return_if_fail (first_property_name != NULL);

  name = first_property_name;
  while (name)
    {
      prop_spec = ipatch_param_find_property (name);

      if (!prop_spec)
	{
	  g_warning ("%s: no parameter property named `%s'",
		     G_STRLOC, name);
	  break;
	}

      if (!(prop_spec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: parameter property `%s' is not writable",
		     G_STRLOC, prop_spec->name);
	  break;
	}

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));
      G_VALUE_COLLECT (&value, args, 0, &error);

      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);

	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occured */
	  break;
	}

      param_set_property (spec, prop_spec, &value);

      g_value_unset (&value);
      name = va_arg (args, char *);
    }
}

/**
 * ipatch_param_set_property:
 * @spec: Parameter spec to set an extended property of
 * @property_name: Name of property to set
 * @value: Value to set the the property to. The value's
 *   type must be the same as the parameter property's type.
 *
 * Set a single extended parameter property of a #GParamSpec.
 */
void
ipatch_param_set_property (GParamSpec *spec, const char *property_name,
			   const GValue *value)
{
  GParamSpec *prop_spec;

  g_return_if_fail (G_IS_PARAM_SPEC (spec));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  prop_spec = ipatch_param_find_property (property_name);

  if (!prop_spec)
    {
      g_warning ("%s: no parameter property named `%s'",
		  G_STRLOC, property_name);
      return;
    }

  if (!(prop_spec->flags & G_PARAM_WRITABLE))
    {
      g_warning ("%s: parameter property `%s' is not writable",
		 G_STRLOC, property_name);
      return;
    }

  if (G_VALUE_TYPE (value) != G_PARAM_SPEC_VALUE_TYPE (prop_spec))
    {
      g_warning ("%s: value type should be '%s' but is '%s'",
		 G_STRLOC, g_type_name (G_PARAM_SPEC_VALUE_TYPE (prop_spec)),
		 G_VALUE_TYPE_NAME (value));
      return;
    }

  param_set_property (spec, prop_spec, value);
}

/* does the actual setting of a GParamSpec GValue property */
static void
param_set_property (GParamSpec *spec, GParamSpec *prop_spec,
		    const GValue *value)
{
  GQuark quark;
  GValue *newval;

  quark = g_quark_try_string (prop_spec->name);
  g_return_if_fail (quark != 0);

  /* duplicate the GValue */
  newval = g_new0 (GValue, 1);
  g_value_init (newval, G_VALUE_TYPE (value));
  g_value_copy (value, newval);

  /* set the GParamSpec property with a destroy notify to free the value
     if the property gets set again */
  g_param_spec_set_qdata_full (spec, quark, newval,
			       (GDestroyNotify)param_value_destroy_notify);
}

/* destroy notify for GParamSpec extended GValue properties */
static void
param_value_destroy_notify (gpointer user_data)
{
  GValue *value = (GValue *)user_data;

  g_value_unset (value);
  g_free (value);
}

/**
 * ipatch_param_get:
 * @spec: Parameter spec to get extended properties from
 * @first_property_name: Name of first property to get
 * @...: Pointer to store first property value and optionally followed
 *   by more property name/value pairs, terminated with %NULL name.
*
 * Get extended parameter properties.  Parameter properties are used to
 * extend existing #GParamSpec types.  "string-max-length" is an example of
 * an extended property, which is used for #GParamSpecString parameters to
 * define the max allowed length.
 */
void
ipatch_param_get (GParamSpec *spec, const char *first_property_name, ...)
{
  va_list args;
  
  va_start (args, first_property_name);
  ipatch_param_get_valist (spec, first_property_name, args);
  va_end (args);
}

/**
 * ipatch_param_get_valist:
 * @spec: Parameter spec to get extended properties from
 * @first_property_name: Name of first property to get
 * @args: Pointer to store first property value and optionally followed
 *   by more property name/value pairs, terminated with %NULL name.
 *
 * Like ipatch_param_get() but uses a va_list.
 */
void
ipatch_param_get_valist (GParamSpec *spec, const char *first_property_name,
			 va_list args)
{
  const char *name;

  g_return_if_fail (G_IS_PARAM_SPEC (spec));
  g_return_if_fail (first_property_name != NULL);

  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *prop_spec;
      char *error;

      prop_spec = ipatch_param_find_property (name);

      if (!prop_spec)
	{
	  g_warning ("%s: no parameter property named `%s'",
		     G_STRLOC, name);
	  break;
	}

      if (!(prop_spec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: parameter property `%s' is not readable",
		     G_STRLOC, prop_spec->name);
	  break;
	}

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));

      param_get_property (spec, prop_spec, &value);
      G_VALUE_LCOPY (&value, args, 0, &error);

      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  g_value_unset (&value);
	  break;
	}
      g_value_unset (&value);
      name = va_arg (args, char *);
    }
}

/**
 * ipatch_param_get_property:
 * @spec: Parameter spec to get an extended property from
 * @property_name: Name of property to get
 * @value: (out): An initialized value to store the property value in. The value's
 *   type must be a type that the property can be transformed to.
 *
 * Get a single extended parameter property from a #GParamSpec.
 *
 * Returns: %TRUE if the parameter property is set, %FALSE otherwise (in which
 *   case @value will contain the default value for this property).
 */
gboolean
ipatch_param_get_property (GParamSpec *spec, const char *property_name,
			   GValue *value)
{
  GParamSpec *prop_spec;
  gboolean retval = FALSE;

  g_return_val_if_fail (G_IS_PARAM_SPEC (spec), FALSE);
  g_return_val_if_fail (property_name != NULL, FALSE);
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);

  prop_spec = ipatch_param_find_property (property_name);

  if (!prop_spec) g_warning ("%s: no parameter property named `%s'",
			     G_STRLOC, property_name);
  else if (!(prop_spec->flags & G_PARAM_READABLE))
    g_warning ("%s: parameter property `%s' is not readable",
	       G_STRLOC, prop_spec->name);
  else
    {
      GValue *prop_value, tmp_value = { 0, };

      /* auto-conversion of the callers value type */
      if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (prop_spec))
	{
	  g_value_reset (value);
	  prop_value = value;
	}
      else if (!g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (prop_spec),
					    G_VALUE_TYPE (value)))
	{
	  g_warning ("can't retrieve parameter property `%s' of type"
		     " `%s' as value of type `%s'", prop_spec->name,
		     g_type_name (G_PARAM_SPEC_VALUE_TYPE (prop_spec)),
		     G_VALUE_TYPE_NAME (value));
	  return (FALSE);
	}
      else
	{
	  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));
	  prop_value = &tmp_value;
	}

      retval = param_get_property (spec, prop_spec, prop_value);

      if (prop_value != value)
	{
	  g_value_transform (prop_value, value);
	  g_value_unset (&tmp_value);
	}
    }

  return (retval);
}

/* does the actual getting of a GParamSpec GValue property */
static gboolean
param_get_property (GParamSpec *spec, GParamSpec *prop_spec,
		    GValue *value)
{
  GQuark quark;
  GValue *getval;

  quark = g_quark_try_string (prop_spec->name);
  g_return_val_if_fail (quark != 0, FALSE);

  getval = g_param_spec_get_qdata (spec, quark);

  if (!getval)
    g_param_value_set_default (prop_spec, value);
  else g_value_copy (getval, value);

  return (getval != NULL);
}
