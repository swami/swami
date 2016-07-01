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
 * SECTION: IpatchTypeProp
 * @short_description: GObject style properties for GTypes
 * @see_also: 
 * @stability: Stable
 *
 * Provides a registry system for adding GObject style properties to GTypes.
 * This is used to describe certain properties of different objects, such as
 * "category".
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "IpatchTypeProp.h"
#include "IpatchVirtualContainer.h"
#include "builtin_enums.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* key used for hash of GType property values */
typedef struct
{
  GType type;		/* type this property is bound to */
  GParamSpec *spec;	/* the parameter spec of the property */
} TypePropValueKey;

typedef struct
{
  GValue value;			/* a static assigned value (or NULL) */
  IpatchTypePropGetFunc func;	/* a dynamic prop function (or NULL) */
  GDestroyNotify notify_func;
  gpointer user_data;
} TypePropValueVal;

static guint type_prop_value_GHashFunc (gconstpointer key);
static gboolean type_prop_value_GEqualFunc (gconstpointer a, gconstpointer b);
static void type_prop_value_destroy (gpointer user_data);

static void type_list_properties_GHFunc (gpointer key, gpointer value,
					 gpointer user_data);
static void
type_set_property (GType type, GParamSpec *prop_spec, const GValue *value,
		   IpatchTypePropGetFunc func, GDestroyNotify notify_func,
                   gpointer user_data);
static void type_get_property (GType type, GParamSpec *prop_spec,
			       GValue *value, GObject *object);


G_LOCK_DEFINE_STATIC (type_prop_hash);
G_LOCK_DEFINE_STATIC (type_prop_value_hash);

/* GType GParamSpec property hash (PropNameQuark -> GParamSpec) */
static GHashTable *type_prop_hash = NULL;

/* hash of all GType property values (GType:GParamSpec -> GValue) */
static GHashTable *type_prop_value_hash = NULL;

/**
 * _ipatch_type_prop_init: (skip)
 *
 * Initialize GType property system
 */
void
_ipatch_type_prop_init (void)
{
  type_prop_hash = g_hash_table_new (NULL, NULL);

  type_prop_value_hash =
    g_hash_table_new_full (type_prop_value_GHashFunc,
			   type_prop_value_GEqualFunc,
			   (GDestroyNotify)g_free,
			   (GDestroyNotify)type_prop_value_destroy);

  /* install some default type properties */

  /* a user friendly type name */
  ipatch_type_install_property
    (g_param_spec_string ("name", "Name", "Name",
			  NULL, G_PARAM_READWRITE));

  /* title of the object (usually dynamically created from obj instance) */
  ipatch_type_install_property
    (g_param_spec_string ("title", "Title", "Title",
			  NULL, G_PARAM_READWRITE));

  /* a user friendly type detail name */
  ipatch_type_install_property
    (g_param_spec_string ("blurb", "Blurb", "Blurb",
			  NULL, G_PARAM_READWRITE));

  /* type classes (see ipatch_type_prop_register_category) */
  ipatch_type_install_property
    (g_param_spec_int ("category", "Category", "Type category",
		       G_MININT, G_MAXINT, IPATCH_CATEGORY_NONE,
		       G_PARAM_READWRITE));

  /* virtual parent container type (defined for children of
   * IpatchVirtualContainer types) */
  ipatch_type_install_property
    (g_param_spec_gtype ("virtual-parent-type", "Virtual parent type",
			 "Virtual parent type", G_TYPE_NONE,
			 G_PARAM_READWRITE));

  /* virtual container child type (defined for IpatchVirtualContainer types) */
  ipatch_type_install_property
    (g_param_spec_gtype ("virtual-child-type", "Virtual child type",
			 "Virtual child type", G_TYPE_NONE,
			 G_PARAM_READWRITE));

  /* link item type (type of object referenced/linked by another) */
  ipatch_type_install_property
    (g_param_spec_gtype ("link-type", "Link type", "Link type", G_TYPE_NONE,
			 G_PARAM_READWRITE));

  /* virtual container conform function (function pointer used for making
   * a child object conform to the virtual container's criteria, the "percussion"
   * property for example) See IpatchVirtualContainerConformFunc. */
  ipatch_type_install_property (g_param_spec_pointer
    ("virtual-child-conform-func",
     "IpatchVirtualContainerConformFunc",
     "IpatchVirtualContainerConformFunc", G_PARAM_READWRITE));

  /* sort a container's children in user interfaces? */
  ipatch_type_install_property
    (g_param_spec_boolean ("sort-children", "Sort children",
			   "Sort children", FALSE, G_PARAM_READWRITE));

  /* splits type property (for note and velocity splits) */
  ipatch_type_install_property
    (g_param_spec_enum ("splits-type", "Splits type",
			"Splits type", IPATCH_TYPE_SPLITS_TYPE,
			IPATCH_SPLITS_NONE, G_PARAM_READWRITE));

  /* mime type for IpatchFile derived types */
  ipatch_type_install_property
    (g_param_spec_string ("mime-type", "Mime type", "Mime type",
			  NULL, G_PARAM_READWRITE));
}

/* hash function for GType property value hash */
static guint
type_prop_value_GHashFunc (gconstpointer key)
{
  TypePropValueKey *valkey = (TypePropValueKey *)key;
  return ((guint)(valkey->type) + G_PARAM_SPEC_TYPE (valkey->spec));
}

/* key equal function for GType property value hash */
static gboolean
type_prop_value_GEqualFunc (gconstpointer a, gconstpointer b)
{
  TypePropValueKey *akey = (TypePropValueKey *)a;
  TypePropValueKey *bkey = (TypePropValueKey *)b;
  return (akey->type == bkey->type && akey->spec == bkey->spec);
}

/* destroy notify for GType property values */
static void
type_prop_value_destroy (gpointer user_data)
{
  TypePropValueVal *val = (TypePropValueVal *)user_data;

  g_value_unset (&val->value);

  if (val->notify_func) val->notify_func (val->user_data);

  g_slice_free (TypePropValueVal, val);
}

/**
 * ipatch_type_install_property:
 * @prop_spec: (transfer full): Specification for the new #GType property.  Ownership
 *   of the GParamSpec is taken over by this function.  The name field of
 *   the GParamSpec should be a static string and is used as the property's
 *   ID.
 *
 * Install a new #GType property which can be used to associate arbitrary
 * information to GTypes.
 */
void
ipatch_type_install_property (GParamSpec *prop_spec)
{
  GQuark quark;

  g_return_if_fail (G_IS_PARAM_SPEC (prop_spec));
  g_return_if_fail (prop_spec->name != NULL);

  /* take ownership of the parameter spec */
  g_param_spec_ref (prop_spec);
  g_param_spec_sink (prop_spec);

  quark = g_quark_from_static_string (prop_spec->name);

  G_LOCK (type_prop_hash);
  g_hash_table_insert (type_prop_hash, GUINT_TO_POINTER (quark), prop_spec);
  G_UNLOCK (type_prop_hash);
}

/**
 * ipatch_type_find_property:
 * @name: Name of GType property
 * 
 * Lookup a GType property by name.
 * 
 * Returns: (transfer none): The matching GParamSpec or %NULL if not found.  The GParamSpec is
 *   internal and should NOT be modified or freed.
 */
GParamSpec *
ipatch_type_find_property (const char *name)
{
  GParamSpec *spec;
  GQuark quark;

  g_return_val_if_fail (name != NULL, NULL);

  quark = g_quark_try_string (name);
  if (!quark) return (NULL);

  G_LOCK (type_prop_hash);
  spec = g_hash_table_lookup (type_prop_hash, GUINT_TO_POINTER (quark));
  G_UNLOCK (type_prop_hash);

  return (spec);
}

/**
 * ipatch_type_list_properties:
 * @n_properties: (out): Length of returned array
 * 
 * Get a list of all registered GType properties.
 * 
 * Returns: (array length=n_properties) (transfer container): An array of GParamSpecs
 *   which should be freed when finished (only the array, not the GParamSpecs themselves)
 */
GParamSpec **
ipatch_type_list_properties (guint *n_properties)
{
  GParamSpec **specs, **specp;

  g_return_val_if_fail (n_properties != NULL, NULL);

  G_LOCK (type_prop_hash);
  specs = g_new (GParamSpec *, g_hash_table_size (type_prop_hash));
  specp = specs;
  g_hash_table_foreach (type_prop_hash, type_list_properties_GHFunc, &specp);
  G_UNLOCK (type_prop_hash);

  return (specs);
}

/* fills an array with GParamSpecs in the type_prop_hash */
static void
type_list_properties_GHFunc (gpointer key, gpointer value, gpointer user_data)
{
  GParamSpec ***specs = user_data;
  **specs = (GParamSpec *)value;
  *specs = *specs + 1;
}

/**
 * ipatch_type_find_types_with_property:
 * @name: Name of type property
 * @value: (nullable): Optional value to match to type property values
 *   (%NULL to match any value)
 * @n_types: (out) (optional): Location to store count of types in returned
 *   array or %NULL to ignore
 *
 * Get an array of types which have the given type property assigned and match
 * @value (optional, %NULL matches any value).
 *
 * Returns: (array length=n_types): Newly allocated 0 terminated array of types
 *   which have the named property set or %NULL if type property not found.
 */
GType *
ipatch_type_find_types_with_property (const char *name, const GValue *value,
                                      guint *n_types)
{
  TypePropValueKey *key;
  GParamSpec *pspec;
  GList *keys, *p, *temp;
  GType *types;
  int count = 0;
  int i;

  g_return_val_if_fail (name != NULL, NULL);

  pspec = ipatch_type_find_property (name);
  g_return_val_if_fail (pspec != NULL, NULL);

  G_LOCK (type_prop_value_hash);

  keys = g_hash_table_get_keys (type_prop_value_hash);  /* ++ alloc keys list */

  /* Convert keys list to list of matching GTypes */
  for (p = keys; p; )
  {
    key = p->data;

    if (key->spec == pspec)
    { /* Replace list data TypePropValueKey pointer to GType */
      p->data = GSIZE_TO_POINTER (key->type);
      p = p->next;
    }
    else        /* Doesn't match GParamSpec - remove from list */
    {
      if (p->prev) p->prev->next = p->next;
      else keys = p->next;

      if (p->next) p->next->prev = p->prev;

      temp = p;
      p = p->next;
      g_list_free1 (temp);
    }
  }

  G_UNLOCK (type_prop_value_hash);

  /* Compare values if @value was supplied */
  if (value)
  {
    GValue cmp_value = { 0 };
    GType type;

    for (p = keys; p; )
    {
      type = GPOINTER_TO_SIZE (p->data);
      g_value_init (&cmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      ipatch_type_get_property (type, pspec->name, &cmp_value);

      if (g_param_values_cmp (pspec, value, &cmp_value) != 0)
      {
        if (p->prev) p->prev->next = p->next;
        else keys = p->next;

        if (p->next) p->next->prev = p->prev;

        temp = p;
        p = p->next;
        g_list_free1 (temp);
      }
      else p = p->next;

      g_value_unset (&cmp_value);
    }
  }

  count = g_list_length (keys);
  types = g_new (GType, count + 1);     /* ++ alloc types */

  /* Copy GType list to type array and delete the list */
  for (p = keys, i = 0; p; p = g_list_delete_link (p, p), i++)  /* -- free keys list */
    types[i] = GPOINTER_TO_SIZE (p->data);

  types[i] = 0;

  if (n_types) *n_types = count;

  return (types);     /* !! caller takes over alloc */
}

/**
 * ipatch_type_set:
 * @type: GType to set properties of
 * @first_property_name: Name of first property to set
 * @...: Value of first property to set and optionally followed by more
 *   property name/value pairs, terminated with %NULL name.
 * 
 * Set GType properties.  GType properties are used to associate arbitrary
 * information with GTypes.
 */
void
ipatch_type_set (GType type, const char *first_property_name, ...)
{
  va_list args;

  va_start (args, first_property_name);
  ipatch_type_set_valist (type, first_property_name, args);
  va_end (args);
}

/**
 * ipatch_type_set_valist:
 * @type: GType to set properties of
 * @first_property_name: Name of first property to set
 * @args: Value of first property to set and optionally followed by more
 *   property name/value pairs, terminated with %NULL name.
 * 
 * Like ipatch_type_set() but uses a va_list.
 */
void
ipatch_type_set_valist (GType type, const char *first_property_name,
			va_list args)
{
  const char *name;
  GValue value = { 0 };
  gchar *error = NULL;
  GParamSpec *prop_spec;

  g_return_if_fail (type != 0);
  g_return_if_fail (first_property_name != NULL);

  name = first_property_name;
  while (name)
    {
      prop_spec = ipatch_type_find_property (name);

      if (!prop_spec)
	{
	  g_warning ("%s: no type property named `%s'",
		     G_STRLOC, name);
	  break;
	}

      if (!(prop_spec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: type property `%s' is not writable",
		     G_STRLOC, prop_spec->name);
	  break;
	}

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));
      G_VALUE_COLLECT (&value, args, 0, &error);

      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);

	  /* we purposely leak the GValue contents here, it might not be
	   * in a sane state if an error condition occured */
	  break;
	}

      type_set_property (type, prop_spec, &value, NULL, NULL, NULL);

      g_value_unset (&value);
      name = va_arg (args, char *);
    }
}

/**
 * ipatch_type_set_property:
 * @type: GType to set property of
 * @property_name: Name of property to set
 * @value: Value to set the the property to. The value's
 *   type must be the same as the GType property's type.
 *
 * Set a single property of a #GType.
 */
void
ipatch_type_set_property (GType type, const char *property_name,
			  const GValue *value)
{
  GParamSpec *prop_spec;

  g_return_if_fail (type != 0);
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  prop_spec = ipatch_type_find_property (property_name);

  if (!prop_spec)
    {
      g_warning ("%s: no type property named `%s'",
		  G_STRLOC, property_name);
      return;
    }

  if (!(prop_spec->flags & G_PARAM_WRITABLE))
    {
      g_warning ("%s: type property `%s' is not writable",
		 G_STRLOC, property_name);
      return;
    }

  if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (prop_spec))
    {
      g_warning ("%s: value type should be '%s' but is '%s'",
		 G_STRLOC, g_type_name (G_PARAM_SPEC_VALUE_TYPE (prop_spec)),
		 G_VALUE_TYPE_NAME (value));
      return;
    }

  type_set_property (type, prop_spec, value, NULL, NULL, NULL);
}

/* does the actual setting of a GType property, note that the value is
   is copied and not used directly */
static void
type_set_property (GType type, GParamSpec *prop_spec, const GValue *value,
		   IpatchTypePropGetFunc func, GDestroyNotify notify_func,
                   gpointer user_data)
{
  TypePropValueKey *key;
  TypePropValueVal *val;

  key = g_new (TypePropValueKey, 1);
  key->type = type;
  key->spec = prop_spec;

  val = g_slice_new0 (TypePropValueVal);

  if (value)
    {
      g_value_init (&val->value, G_VALUE_TYPE (value));
      g_value_copy (value, &val->value);
    }

  val->func = func;
  val->notify_func = notify_func;
  val->user_data = user_data;

  /* value is taken over by the hash table */
  G_LOCK (type_prop_value_hash);
  g_hash_table_insert (type_prop_value_hash, key, val);
  G_UNLOCK (type_prop_value_hash);
}

/**
 * ipatch_type_unset_property:
 * @type: GType to unset a property of
 * @property_name: The property to unset
 *
 * Unsets the value or dynamic function of a type property.
 *
 * Since: 1.1.0
 */
void
ipatch_type_unset_property (GType type, const char *property_name)
{
  GParamSpec *prop_spec;
  TypePropValueKey key;

  g_return_if_fail (type != 0);
  g_return_if_fail (property_name != NULL);

  prop_spec = ipatch_type_find_property (property_name);

  if (!prop_spec)
  {
    g_warning ("%s: no type property named `%s'",
               G_STRLOC, property_name);
    return;
  }

  if (!(prop_spec->flags & G_PARAM_WRITABLE))
  {
    g_warning ("%s: type property `%s' is not writable",
               G_STRLOC, property_name);
    return;
  }

  key.type = type;
  key.spec = prop_spec;

  G_LOCK (type_prop_value_hash);
  g_hash_table_remove (type_prop_value_hash, &key);
  G_UNLOCK (type_prop_value_hash);
}

/**
 * ipatch_type_get:
 * @type: GType to get properties from
 * @first_property_name: Name of first property to get
 * @...: Pointer to store first property value and optionally followed
 *   by more property name/value pairs, terminated with %NULL name.
 *
 * Get GType property values.
 */
void
ipatch_type_get (GType type, const char *first_property_name, ...)
{
  va_list args;
  
  va_start (args, first_property_name);
  ipatch_type_get_valist (type, first_property_name, args);
  va_end (args);
}

/**
 * ipatch_type_get_valist:
 * @type: GType to get properties from
 * @first_property_name: Name of first property to get
 * @args: Pointer to store first property value and optionally followed
 *   by more property name/value pairs, terminated with %NULL name.
 *
 * Like ipatch_type_get() but uses a va_list.
 */
void
ipatch_type_get_valist (GType type, const char *first_property_name,
			va_list args)
{
  const char *name;

  g_return_if_fail (type != 0);
  g_return_if_fail (first_property_name != NULL);

  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *prop_spec;
      char *error;

      prop_spec = ipatch_type_find_property (name);

      if (!prop_spec)
	{
	  g_warning ("%s: no type property named `%s'",
		     G_STRLOC, name);
	  break;
	}

      if (!(prop_spec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: type property `%s' is not readable",
		     G_STRLOC, prop_spec->name);
	  break;
	}

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));

      type_get_property (type, prop_spec, &value, NULL);
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
 * ipatch_type_get_property:
 * @type: GType to get property from
 * @property_name: Name of property to get
 * @value: (out): An initialized value to store the property value in. The value's
 *   type must be a type that the property can be transformed to.
 *
 * Get a single property from a #GType.
 */
void
ipatch_type_get_property (GType type, const char *property_name,
			  GValue *value)
{
  GParamSpec *prop_spec;

  g_return_if_fail (type != 0);
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  prop_spec = ipatch_type_find_property (property_name);

  if (!prop_spec) g_warning ("%s: no type property named `%s'",
			     G_STRLOC, property_name);
  else if (!(prop_spec->flags & G_PARAM_READABLE))
    g_warning ("%s: type property `%s' is not readable",
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
	  g_warning ("can't retrieve type property `%s' of type"
		     " `%s' as value of type `%s'", prop_spec->name,
		     g_type_name (G_PARAM_SPEC_VALUE_TYPE (prop_spec)),
		     G_VALUE_TYPE_NAME (value));
	  return;
	}
      else
	{
	  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));
	  prop_value = &tmp_value;
	}

      type_get_property (type, prop_spec, prop_value, NULL);

      if (prop_value != value)
	{
	  g_value_transform (prop_value, value);
	  g_value_unset (&tmp_value);
	}
    }
}

/* does the actual getting of a GType property */
static void
type_get_property (GType type, GParamSpec *prop_spec, GValue *value,
		   GObject *object)
{
  TypePropValueKey key;
  TypePropValueVal *val;

  key.type = type;
  key.spec = prop_spec;

  G_LOCK (type_prop_value_hash);
  val = g_hash_table_lookup (type_prop_value_hash, &key);
  G_UNLOCK (type_prop_value_hash);

  if (val)
    {
      if (val->func) val->func (type, prop_spec, value, object);
      else g_value_copy (&val->value, value);
    }
  else g_param_value_set_default (prop_spec, value);
}

/**
 * ipatch_type_object_get:
 * @object: Object instance to get type property from
 * @first_property_name: Name of first property to get
 * @...: Pointer to store first property value and optionally followed
 *   by more property name/value pairs, terminated with %NULL name.
 *
 * Get GType property values.  Like ipatch_type_get() but takes an object
 * instance which can be used by any registered dynamic type property
 * functions.
 */
void
ipatch_type_object_get (GObject *object, const char *first_property_name, ...)
{
  va_list args;
  
  va_start (args, first_property_name);
  ipatch_type_object_get_valist (object, first_property_name, args);
  va_end (args);
}

/*
 * ipatch_type_object_get_valist:
 * @object: Object instance to get type property from
 * @first_property_name: Name of first property to get
 * @args: Pointer to store first property value and optionally followed
 *   by more property name/value pairs, terminated with %NULL name.
 *
 * Like ipatch_type_object_get() but uses a va_list.
 */
void
ipatch_type_object_get_valist (GObject *object, const char *first_property_name,
			       va_list args)
{
  GType type;
  const char *name;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (first_property_name != NULL);

  type = G_OBJECT_TYPE (object);
  g_return_if_fail (type != 0);

  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *prop_spec;
      char *error;

      prop_spec = ipatch_type_find_property (name);

      if (!prop_spec)
	{
	  g_warning ("%s: no type property named `%s'",
		     G_STRLOC, name);
	  break;
	}

      if (!(prop_spec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: type property `%s' is not readable",
		     G_STRLOC, prop_spec->name);
	  break;
	}

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));

      type_get_property (type, prop_spec, &value, object);
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
 * ipatch_type_object_get_property:
 * @object: Object instance to get type property from
 * @property_name: Name of property to get
 * @value: (out): An initialized value to store the property value in. The value's
 *   type must be a type that the property can be transformed to.
 *
 * Get a single type property from an @object instance.
 */
void
ipatch_type_object_get_property (GObject *object, const char *property_name,
				 GValue *value)
{
  GParamSpec *prop_spec;
  GType type;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  type = G_OBJECT_TYPE (object);
  g_return_if_fail (type != 0);

  prop_spec = ipatch_type_find_property (property_name);

  if (!prop_spec) g_warning ("%s: no type property named `%s'",
			     G_STRLOC, property_name);
  else if (!(prop_spec->flags & G_PARAM_READABLE))
    g_warning ("%s: type property `%s' is not readable",
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
	  g_warning ("can't retrieve type property `%s' of type"
		     " `%s' as value of type `%s'", prop_spec->name,
		     g_type_name (G_PARAM_SPEC_VALUE_TYPE (prop_spec)),
		     G_VALUE_TYPE_NAME (value));
	  return;
	}
      else
	{
	  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));
	  prop_value = &tmp_value;
	}

      type_get_property (type, prop_spec, prop_value, object);

      if (prop_value != value)
	{
	  g_value_transform (prop_value, value);
	  g_value_unset (&tmp_value);
	}
    }
}

/**
 * ipatch_type_set_dynamic_func: (skip)
 * @type: GType of the type property
 * @property_name: Name of a previously registered type property
 * @func: Callback function used for getting values for this type property
 *
 * Registers a callback function for dynamically getting the value of a
 * type property.
 *
 * Example: A dynamic property is created for SoundFont presets to return a
 * different "virtual-parent-type" depending on if its a percussion or
 * melodic preset (determined from a preset's bank number).
 */
void
ipatch_type_set_dynamic_func (GType type, const char *property_name,
			      IpatchTypePropGetFunc func)
{
  ipatch_type_set_dynamic_func_full (type, property_name, func, NULL, NULL);
}

/**
 * ipatch_type_set_dynamic_func_full: (rename-to ipatch_type_set_dynamic_func)
 * @type: GType of the type property
 * @property_name: Name of a previously registered type property
 * @func: Callback function used for getting values for this type property
 * @notify_func: (nullable) (scope async) (closure user_data): Destroy function
 *   callback when @func is removed
 * @user_data: (nullable): Data passed to @notify_func
 *
 * Registers a callback function for dynamically getting the value of a
 * type property.  Like ipatch_type_set_dynamic_func() but more GObject Introspection
 * friendly.
 *
 * Example: A dynamic property is created for SoundFont presets to return a
 * different "virtual-parent-type" depending on if its a percussion or
 * melodic preset (determined from a preset's bank number).
 *
 * Since: 1.1.0
 */
void
ipatch_type_set_dynamic_func_full (GType type, const char *property_name,
			           IpatchTypePropGetFunc func,
                                   GDestroyNotify notify_func, gpointer user_data)
{
  GParamSpec *prop_spec;

  g_return_if_fail (type != 0);
  g_return_if_fail (property_name != NULL);

  prop_spec = ipatch_type_find_property (property_name);

  if (!prop_spec)
    {
      g_warning ("%s: no type property named `%s'",
		  G_STRLOC, property_name);
      return;
    }

  type_set_property (type, prop_spec, NULL, func, notify_func, user_data);
}

/**
 * ipatch_type_get_dynamic_func: (skip)
 * @type: GType of the type property
 * @property_name: Name of a previously registered type property
 *
 * Get a the dynamic function registered for a given @type and @property_name
 * with ipatch_type_set_dynamic_func().  Also can be used as an indicator of
 * whether a type property is dynamic or not.
 *
 * Returns: Pointer to the registered function or %NULL if no function
 *   registered (not a dynamic type property).
 */
IpatchTypePropGetFunc
ipatch_type_get_dynamic_func (GType type, const char *property_name)
{
  GParamSpec *type_prop_pspec;
  TypePropValueKey key;
  TypePropValueVal *val;

  type_prop_pspec = ipatch_type_find_property (property_name);
  g_return_val_if_fail (type_prop_pspec != NULL, NULL);

  key.type = type;
  key.spec = type_prop_pspec;

  G_LOCK (type_prop_value_hash);
  val = g_hash_table_lookup (type_prop_value_hash, &key);
  G_UNLOCK (type_prop_value_hash);

  return (val ? val->func : NULL);
}
